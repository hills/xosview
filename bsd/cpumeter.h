//
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  NetBSD port:
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

#ifndef _CPUMETER_H_
#define _CPUMETER_H_

#include "fieldmetergraph.h"

#if defined(XOSVIEW_DFBSD)
#define CPUSTATES 5
#else
#include <sys/dkstat.h>
#endif


class CPUMeter : public FieldMeterGraph {
public:
	CPUMeter( XOSView *parent );
	~CPUMeter( void );

	const char *name( void ) const { return "CPUMeter"; }
	void checkevent( void );
	void checkResources( void );

protected:
	void getcputime( void );

private:
	long cputime_[2][CPUSTATES];
	int cpuindex_;
};


#endif
