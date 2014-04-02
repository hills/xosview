//
//  Copyright (c) 2014 by Tomi Tapper <tomi.o.tapper@jyu.fi>
//
//  File based on bsd/intratemeter.* by
//  Copyright (c) 1999 by Brian Grayson (bgrayson@netbsd.org)
//
//  This file may be distributed under terms of the GPL
//
//

#ifndef _IRQRATEMETER_H_
#define _IRQRATEMETER_H_

#include "fieldmetergraph.h"
#include "xosview.h"
#include "kstats.h"
#include <kstat.h>


class IrqRateMeter : public FieldMeterGraph {
public:
	IrqRateMeter(XOSView *parent, kstat_ctl_t *kc);
	~IrqRateMeter(void);

	const char *name(void) const { return "IrqRateMeter"; }
	void checkevent(void);
	void checkResources(void);

protected:
	void getinfo(void);

private:
	uint64_t _lastirqcount;
	kstat_ctl_t *_kc;
	KStatList *_cpus;
};


#endif
