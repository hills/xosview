//  
// $Id$
//
#include "cpumeter.h"
#include "xosview.h"
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <strstream.h>

CPUMeter::CPUMeter(XOSView *parent, kstat_ctl_t *_kc, int cpuid)
	: FieldMeterDecay(parent, CPU_STATES, toUpper(cpuStr(cpuid)),
	    "USER/SYS/WAIT/IDLE")
{
	kc = _kc;
	for (int i = 0 ; i < 2 ; i++)
		for (int j = 0 ; j < CPU_STATES ; j++)
			cputime_[i][j] = 0;
	cpuindex_ = 0;

	int j = 0;
	for (ksp = kc->kc_chain; ksp != NULL; ksp = ksp->ks_next) {
		if (strncmp(ksp->ks_name, "cpu_stat", 8) == 0) {
			j++;
			if (j == cpuid)
				break;
		}
	}
}

CPUMeter::~CPUMeter(void)
{
}

void CPUMeter::checkResources(void)
{
	FieldMeterDecay::checkResources();

	setfieldcolor(0, parent_->getResource("cpuUserColor"));
	setfieldcolor(1, parent_->getResource("cpuSystemColor"));
	setfieldcolor(2, parent_->getResource("cpuInterruptColor"));
	setfieldcolor(3, parent_->getResource("cpuFreeColor"));
	priority_ = atoi(parent_->getResource("cpuPriority"));
	dodecay_ = !strcmp(parent_->getResource("cpuDecay"), "True");
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

	if (kstat_read(kc, ksp, &cs) == -1) {
		parent_->done(1);
		return;
	}
	cputime_[cpuindex_][0] = cs.cpu_sysinfo.cpu[CPU_USER];
	cputime_[cpuindex_][1] = cs.cpu_sysinfo.cpu[CPU_KERNEL];
	cputime_[cpuindex_][2] = cs.cpu_sysinfo.cpu[CPU_WAIT];
	cputime_[cpuindex_][3] = cs.cpu_sysinfo.cpu[CPU_IDLE];

	int oldindex = (cpuindex_ + 1) % 2;
	for (int i = 0 ; i < CPU_STATES ; i++) {
		fields_[i] = cputime_[cpuindex_][i] - cputime_[oldindex][i];
		total_ += fields_[i];
	}
	cpuindex_ = (cpuindex_ + 1) % 2;

	if (total_)
		setUsed(total_ - fields_[3], total_);
}

const char *CPUMeter::toUpper(const char *str)
{
	static char buffer[256];
	strcpy(buffer, str);
	for (char *tmp = buffer ; *tmp != '\0' ; tmp++)
		*tmp = toupper(*tmp);

	return buffer;
}

const char *CPUMeter::cpuStr(int num)
{
	static char buffer[32];
	ostrstream str(buffer, 32);

	str << "cpu";
	if (num != 0)
		str << (num - 1);
	str << ends;

	return buffer;
}

int CPUMeter::countCPUs(kstat_ctl_t *kc)
{
	kstat_t *ksp;
	int i = 0;

        for (ksp = kc->kc_chain; ksp != NULL; ksp = ksp->ks_next) {
                if (strncmp(ksp->ks_name, "cpu_stat", 8) == 0)
                        i++;
        }
	return (i);
}
