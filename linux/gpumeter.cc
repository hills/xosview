//
//  Copyright (c) 2022 by Tvrtko Ursulin <tvrtko.ursulin@intel.com>
//
//  This file may be distributed under terms of the GPL
//

#include "gpumeter.h"

#include <algorithm>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/utsname.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <sys/sysinfo.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <drm/i915_drm.h>

static uint64_t get_pmu_config(std::string fname)
{
	uint64_t counter = -1;
	std::stringstream ss;
	std::ifstream file;
	std::string str;
	size_t pos;

	file.open(fname);
	if (file.fail())
		goto out;

	file >> str;

	pos = str.find("0x");
	if (pos == std::string::npos)
		goto out;

	ss << std::hex << str.substr(pos + 2);
	ss >> counter;

out:
	return counter;
}

static bool cmpEngine(GPUEngine *a, GPUEngine *b)
{
	if (a->engine_class == b->engine_class)
		return a->engine_instance < b->engine_instance;
	else
		return a->engine_class < b->engine_class;
}

static int
perf_event_open(struct perf_event_attr *attr,
		pid_t pid,
		int cpu,
		int group_fd,
		unsigned long flags)
{
#ifndef __NR_perf_event_open
#if defined(__i386__)
#define __NR_perf_event_open 336
#elif defined(__x86_64__)
#define __NR_perf_event_open 298
#else
#define __NR_perf_event_open 0
#endif
#endif
	attr->size = sizeof(*attr);

	return syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

static int open_pmu(uint64_t type, uint64_t config, int group)
{
	static int nr_cpus = get_nprocs_conf();
	struct perf_event_attr attr = { };
	int cpu = 0, ret;
	uint64_t format;

	attr.type = type;

	format = PERF_FORMAT_TOTAL_TIME_ENABLED;
	if (group == -1)
		format |= PERF_FORMAT_GROUP;

	attr.read_format = format;
	attr.config = config;
	attr.use_clockid = 1;
	attr.clockid = CLOCK_MONOTONIC;

	do {
		ret = perf_event_open(&attr, -1, cpu++, group, 0);
	} while ((ret < 0 && errno == EINVAL) && (cpu < nr_cpus));

	return ret;
}

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))

GPU::GPU(unsigned int _idx, unsigned long _type, std::string _root)
 : num_classes(0),
   pmu_fd(-1)
{
	static const char *class_names[] = {
		"RENDER",
		"COPY",
		"VIDEO",
		"ENHANCE",
	};
	std::string events_root = _root + "/events/";
	struct dirent *dent;
	DIR *d;

	idx = _idx;
	type = _type;
	root = _root;

	snprintf(meter_name, sizeof(meter_name), "GPU%u", idx);

	d = opendir(events_root.c_str());
	if (!d)
		return;

	while ((dent = readdir(d)) != NULL) {
		const std::string suffix = "-busy";
		GPUEngine engine;
		std::string str;

		if (dent->d_type != DT_REG)
			continue;

		str = dent->d_name;

                /* xxxN-busy */
		if (str.length() < (suffix.length() + 4))
			continue;
		if (str.compare(str.length() - suffix.length(),
				suffix.size(), suffix) != 0)
			continue;

		engine.name = str.substr(0, str.length() - suffix.length());

		engine.pmu_config = get_pmu_config(events_root + str);
		if (engine.pmu_config == (uint64_t)-1)
			continue;

		/* Double check config is an engine config. */
		if (engine.pmu_config >= __I915_PMU_OTHER(0))
			continue;

		engine.engine_class =
			(engine.pmu_config & (__I915_PMU_OTHER(0) - 1)) >>
			I915_PMU_CLASS_SHIFT;
		engine.engine_instance =
			(engine.pmu_config >> I915_PMU_SAMPLE_BITS) &
			((1 << I915_PMU_SAMPLE_INSTANCE_BITS) - 1);

		engines.push_back(new GPUEngine(engine));
	}
	closedir(d);

	std::sort(engines.begin(), engines.end(), cmpEngine);

	max_class = 0;
	for (std::vector<GPUEngine *>::iterator it = engines.begin();
	    it != engines.end();
	    it++) {
		unsigned int engine_class = (*it)->engine_class;

		if (engine_class > max_class)
			max_class = engine_class;
	}

	engines_per_class = new unsigned int[max_class + 1];
	memset(engines_per_class, 0, sizeof(unsigned int) * (max_class + 1));

	for (std::vector<GPUEngine *>::iterator it = engines.begin();
	    it != engines.end();
	    it++)
		engines_per_class[(*it)->engine_class]++;

	for (unsigned int i = 0; i <= max_class; i++) {
		if (engines_per_class)
			num_classes++;

		if (meter_legend.length())
			meter_legend += "/";
		if (i < ARRAY_SIZE(class_names))
			meter_legend += class_names[i];
		else
			meter_legend += "UNKNOWN";
	}

	for (std::vector<GPUEngine *>::iterator it = engines.begin();
	    it != engines.end();
	    it++) {
		int fd = open_pmu(type, (*it)->pmu_config, pmu_fd);

		if (fd < 0) {
			pmu_fd = -1;
			break;
		} else if (pmu_fd == -1) {
			pmu_fd = fd;
		}
	}
}

GPU::~GPU()
{

}

static uint64_t pmu_read_multi(int fd, unsigned int num, uint64_t *val)
{
	uint64_t buf[2 + num];
	unsigned int i;
	ssize_t len;

	memset(buf, 0, sizeof(buf));

	len = read(fd, buf, sizeof(buf));

	if (len == (ssize_t)sizeof(buf)) {
		for (i = 0; i < num; i++)
			val[i] = buf[2 + i];
	}

	return buf[1];
}

double GPU::getBusy(double *busy)
{
	const int num = engines.size();
	uint64_t val[num];
	unsigned long t;

	ts.prev = ts.cur;
	ts.cur = pmu_read_multi(pmu_fd, num, val);
	t = ts.cur - ts.prev;

	for (unsigned int i = 0; i <= max_class; i++)
		busy[i] = 0;

	for (int i = 0; i < num; i++) {
		engines[i]->busy.prev = engines[i]->busy.cur;
		engines[i]->busy.cur = val[i];
		busy[engines[i]->engine_class] +=
			engines[i]->busy.cur - engines[i]->busy.prev;
	}

	for (unsigned int i = 0; i <= max_class; i++) {
		if (engines_per_class[i] > 1)
			busy[i] /= engines_per_class[i];
	}

	return t;
}

GPUList::GPUList()
{
	const char *sysfs_root = "/sys/devices/";
	unsigned int count = 0;
	struct dirent *dent;
	DIR *d;

	d = opendir(sysfs_root);
	if (!d)
		return;

	while ((dent = readdir(d)) != NULL) {
		const std::string prefix = "i915";
		unsigned long type;
		std::ifstream file;
		std::string str;

		if (dent->d_type != DT_DIR)
			continue;

		str = dent->d_name;
		if (str.compare(0, prefix.size(), prefix) != 0)
			continue;

		str = sysfs_root + str;
		file.open(str + "/type");
		if (file.fail())
			continue;

		file >> type;

		GPU *gpu = new GPU(count, type, str);
		gpus[count++] = gpu;
	}
	closedir(d);
}

GPUList::~GPUList()
{

}

GPUMeter::GPUMeter(XOSView *parent, GPU *gpu_)
: FieldMeterGraph(parent, 10, gpu_->getName(), gpu_->getLegend()),
  gpu(gpu_)
{
}

GPUMeter::~GPUMeter(void)
{
}

void GPUMeter::checkResources(void)
{
	FieldMeterGraph::checkResources();

	dodecay_ = 0;
	useGraph_ = 1;
	lineGraph_ = 1;
	stackGraph_ = 0;
	SetUsedFormat("percent");

	priority_ = atoi(parent_->getResource("gpuPriority"));

	setNumFields(gpu->numEngines() + (stackGraph_ ? 1 : 0));

	for (unsigned int i = 0; i < gpu->numEngines(); i++) {
		char buf[64];

		snprintf(buf, sizeof(buf), "gpuColor%u", i + 1);
		setfieldcolor(i, parent_->allocColor(parent_->getResource(buf)));
	}

	if (stackGraph_)
		setfieldcolor(numfields_ - 1,
			      parent_->allocColor(parent_->getResource("gpuIdleColor")));

	if (lineGraph_)
		backgroundColor_ =
			parent_->allocColor(parent_->getResource("gpuIdleColor"));

	getgputime();
}

void GPUMeter::checkevent(void)
{
	getgputime();
	drawfields();
}

void GPUMeter::getgputime(void)
{
	total_ = 0;
	memset(fields_, 0, numfields_ * sizeof(fields_[0]));

	if (!gpu->isInitialized())
		return;

	total_ = gpu->getBusy(fields_);
	if (total_) {
		double max = 0.0;

		for (unsigned int i = 0; i < gpu->numEngines(); i++) {
			if (fields_[i] > max)
				max = fields_[i];
		}
		if (stackGraph_)
			fields_[numfields_ - 1] = total_ - max;
		setUsed(max, total_);
	}
}
