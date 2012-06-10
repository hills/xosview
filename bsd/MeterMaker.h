//
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//  Copyright (c) 1995, 1996, 1997-2002 by Brian Grayson (bgrayson@netbsd.org)
//
//  This file was written by Brian Grayson for the NetBSD and xosview
//    projects.
//  This file may be distributed under terms of the GPL or of the BSD
//    license, whichever you choose.  The full license notices are
//    contained in the files COPYING.GPL and COPYING.BSD, which you
//    should have received.  If not, contact one of the xosview
//    authors for a copy.
//

#ifndef _MeterMaker_h
#define _MeterMaker_h

#include "pllist.h"
#include "meter.h"
#include "xosview.h"


class MeterMaker : public PLList<Meter *> {
public:
	MeterMaker(XOSView *xos);
	void makeMeters(void);

private:
	XOSView *_xos;
};


#endif
