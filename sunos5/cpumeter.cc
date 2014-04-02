//
//  Initial port performed by Greg Onufer (exodus@cheers.bungi.com)
//

#include "cpumeter.h"
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <sys/sysinfo.h>


CPUMeter::CPUMeter(XOSView *parent, kstat_ctl_t *_kc, int cpuid)
	: FieldMeterGraph(parent, CPU_STATES, cpuStr(cpuid), "USER/SYS/WAIT/IDLE")
{
	kc = _kc;
	aggregate = ( cpuid < 0 );
	for (int i = 0 ; i < 2 ; i++)
		for (int j = 0 ; j < CPU_STATES ; j++)
			cputime_[i][j] = 0;
	cpuindex_ = 0;
	cpustats = KStatList::getList(kc, KStatList::CPU_STAT);

	if (!aggregate)
		for (unsigned int i = 0; i < cpustats->count(); i++)
			if ((*cpustats)[i]->ks_instance == cpuid)
				ksp = (*cpustats)[i];
}

CPUMeter::~CPUMeter(void)
{
}

void CPUMeter::checkResources(void)
{
	FieldMeterGraph::checkResources();

	setfieldcolor(0, parent_->getResource("cpuUserColor"));
	setfieldcolor(1, parent_->getResource("cpuSystemColor"));
	setfieldcolor(2, parent_->getResource("cpuInterruptColor"));
	setfieldcolor(3, parent_->getResource("cpuFreeColor"));
	priority_ = atoi(parent_->getResource("cpuPriority"));
	dodecay_ = parent_->isResourceTrue("cpuDecay");
	useGraph_ = parent_->isResourceTrue("cpuGraph");
	SetUsedFormat(parent_->getResource("cpuUsedFormat"));
}

void CPUMeter::checkevent(void)
{
	getcputime();
	drawfields();
}

void CPUMeter::getcputime(void)
{
	total_ = 0;
	cpu_stat_t cs;

	if (aggregate) {
		cpustats->update(kc);
		bzero(cputime_[cpuindex_], CPU_STATES * sizeof(cputime_[cpuindex_][0]));
		for (unsigned int i = 0; i < cpustats->count(); i++) {
			if (kstat_read(kc, (*cpustats)[i], &cs) == -1) {
				parent_->done(1);
				return;
			}
			cputime_[cpuindex_][0] += cs.cpu_sysinfo.cpu[CPU_USER];
			cputime_[cpuindex_][1] += cs.cpu_sysinfo.cpu[CPU_KERNEL];
			cputime_[cpuindex_][2] += cs.cpu_sysinfo.cpu[CPU_WAIT];
			cputime_[cpuindex_][3] += cs.cpu_sysinfo.cpu[CPU_IDLE];
		}
	}
	else {
		if (kstat_read(kc, ksp, &cs) == -1) {
			parent_->done(1);
			return;
		}
		cputime_[cpuindex_][0] = cs.cpu_sysinfo.cpu[CPU_USER];
		cputime_[cpuindex_][1] = cs.cpu_sysinfo.cpu[CPU_KERNEL];
		cputime_[cpuindex_][2] = cs.cpu_sysinfo.cpu[CPU_WAIT];
		cputime_[cpuindex_][3] = cs.cpu_sysinfo.cpu[CPU_IDLE];
	}

	int oldindex = (cpuindex_ + 1) % 2;
	for (int i = 0 ; i < CPU_STATES ; i++) {
		fields_[i] = cputime_[cpuindex_][i] - cputime_[oldindex][i];
		total_ += fields_[i];
	}
	cpuindex_ = (cpuindex_ + 1) % 2;

	if (total_)
		setUsed(total_ - fields_[3], total_);
}

const char *CPUMeter::cpuStr(int num)
{
	static char buffer[8] = "CPU";
	if (num >= 0)
		snprintf(buffer + 3, 4, "%d", num);
	buffer[7] = '\0';
	return buffer;
}
