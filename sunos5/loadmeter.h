//  
// $Id$
//  Initial port performed by Greg Onufer (exodus@cheers.bungi.com)
//
#ifndef _LOADMETER_H_
#define _LOADMETER_H_

#include "fieldmeterdecay.h"
#include <kstat.h>

class LoadMeter : public FieldMeterDecay {
 public:
	LoadMeter(XOSView *parent, kstat_ctl_t *kcp);
	~LoadMeter(void);

	const char *name(void) const { return "LoadMeter"; }  
	void checkevent(void);

	void checkResources(void);

 protected:
	void getloadinfo(void);
	unsigned long procloadcol_;
	unsigned long warnloadcol_;

 private:
	int alarmThreshold;
	kstat_ctl_t *kc;
	kstat_t *ksp;
};

#endif
