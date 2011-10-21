//  
//  Initial port performed by Stefan Eilemann (eilemann@gmail.com)
//
#ifndef _LOADMETER_H_
#define _LOADMETER_H_

#include "fieldmetergraph.h"
#include <rpcsvc/rstat.h>

class LoadMeter : public FieldMeterGraph {
 public:
	LoadMeter(XOSView *parent);
	~LoadMeter(void);

	const char *name(void) const { return "LoadMeter"; }  
	void checkevent(void);

	void checkResources(void);

protected:
    void getloadinfo(void);
    
    unsigned long procloadcol_, warnloadcol_, critloadcol_;
private:
    int warnThreshold, critThreshold, alarmstate, lastalarmstate;
    char hostname[256];
    struct statstime res;
};

#endif
