//  
//  Initial port performed by Greg Onufer (exodus@cheers.bungi.com)
//
#ifndef _LOADMETER_H_
#define _LOADMETER_H_

#include "fieldmetergraph.h"
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

 private:
	unsigned long procloadcol, warnloadcol, critloadcol;
	unsigned int warnThreshold, critThreshold;
	int lastalarmstate;
	kstat_ctl_t *kc;
	kstat_t *ksp;
};

#endif
