//  
// $Id$
//  Initial port performed by Stefan Eilemann (eile@sgi.com)
//
#ifndef _GFXMETER_H_
#define _GFXMETER_H_

#include "sarmeter.h"

#include <rpcsvc/rstat.h>

class GfxMeter : public FieldMeterGraph {
 public:
	GfxMeter(XOSView *parent, int max);
	~GfxMeter(void);

	const char *name(void) const { return "GfxMeter"; }  
	void checkevent(void);

	void checkResources(void);

protected:
    void getgfxinfo(void);
    
    unsigned long swapgfxcol_, warngfxcol_, critgfxcol_;
private:
    int warnThreshold, critThreshold, alarmstate, lastalarmstate;
    int nPipes;

    unsigned int lastSwapBuf;
};

#endif
