//
//  Copyright (c) 2022 by Tvrtko Ursulin <tvrtko.ursulin@intel.com>
//
//  This file may be distributed under terms of the GPL
//

#ifndef _GPUMETER_H_
#define _GPUMETER_H_

#include "fieldmetergraph.h"
#include "xosview.h"

#include <map>
#include <string>
#include <vector>

struct pmu_pair {
	uint64_t cur;
	uint64_t prev;
};

class GPUEngine {
public:
	GPUEngine() { };
	~GPUEngine() { };

	std::string name;
	uint64_t pmu_config;
	unsigned int engine_class;
	unsigned int engine_instance;

	struct pmu_pair busy;
};

class GPU {
public:
	GPU(unsigned int idx, unsigned long type, std::string root);
	~GPU();

	bool isInitialized() const {  return pmu_fd >= 0; }
	const char *getName() const { return meter_name; }
	const char *getLegend() const { return meter_legend.c_str(); }
	unsigned int numEngines() const { return num_classes; }
	double getBusy(double *busy);

private:
	unsigned int idx;
	unsigned long type;
	std::string root;

	std::vector<GPUEngine *> engines;
	unsigned int max_class;
	unsigned int num_classes;
	unsigned int *engines_per_class;

	char meter_name[64];
	std::string meter_legend;

	int pmu_fd;

	struct pmu_pair ts;
};

class GPUList {
public:
	GPUList();
	~GPUList();

	unsigned int getCount() const { return gpus.size(); }
	GPU *getGPU(unsigned int idx) { return gpus[idx]; }

private:
	std::map<unsigned int, GPU *> gpus;
};

class GPUMeter : public FieldMeterGraph {
public:
	GPUMeter(XOSView *parent, GPU *gpu);
	~GPUMeter(void);

	const char *name(void) const { return "GPUMeter"; }
	void checkevent(void);

	void checkResources(void);

private:
	GPU *gpu;

	void getgputime( void );
};

#endif
