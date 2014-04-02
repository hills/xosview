//  
//  Initial port performed by Greg Onufer (exodus@cheers.bungi.com)
//
#ifndef _CPUMETER_H_
#define _CPUMETER_H_

#include "fieldmetergraph.h"
#include "xosview.h"
#include "kstats.h"
#include <kstat.h>
#include <sys/sysinfo.h>


class CPUMeter : public FieldMeterGraph {
 public:
	CPUMeter(XOSView *parent, kstat_ctl_t *kcp, int cpuid = 0);
	~CPUMeter(void);

	const char *name(void) const { return "CPUMeter"; }
	void checkevent(void);
	void checkResources(void);
	static const char *cpuStr(int num);

 protected:
	float cputime_[2][CPU_STATES];
	int cpuindex_;

	void getcputime(void);

 private:
	KStatList *cpustats;
	bool aggregate;
	kstat_ctl_t *kc;
	kstat_t *ksp;
};

#endif
