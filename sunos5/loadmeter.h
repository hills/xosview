//  
//  Initial port performed by Greg Onufer (exodus@cheers.bungi.com)
//
#ifndef _LOADMETER_H_
#define _LOADMETER_H_

#include "fieldmetergraph.h"
#include "xosview.h"
#include "kstats.h"
#include <kstat.h>


class LoadMeter : public FieldMeterGraph {
 public:
	LoadMeter(XOSView *parent, kstat_ctl_t *kcp);
	~LoadMeter(void);

	const char *name(void) const { return "LoadMeter"; }
	void checkevent(void);
	void checkResources(void);

 protected:
	void getloadinfo(void);
	void getspeedinfo(void);

 private:
	unsigned long procloadcol, warnloadcol, critloadcol;
	unsigned int warnThreshold, critThreshold;
	unsigned int old_cpu_speed, cur_cpu_speed;
	int lastalarmstate;
	bool do_cpu_speed;
	KStatList *cpulist;
	kstat_ctl_t *kc;
#ifdef NO_GETLOADAVG
	kstat_t *ksp;
#endif
};

#endif
