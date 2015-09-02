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
#include "xosview.h"
#include "defines.h"

// for CPUSTATES
#if defined(XOSVIEW_NETBSD) || defined(XOSVIEW_OPENBSD)
#include <sys/sched.h>
#else
#include <sys/resource.h>
#endif


class CPUMeter : public FieldMeterGraph {
public:
	CPUMeter( XOSView *parent, unsigned int nbr );
	~CPUMeter( void );

	const char *name( void ) const { return "CPUMeter"; }
	void checkevent( void );
	void checkResources( void );

protected:
	void getcputime( void );

private:
	uint64_t cputime_[2][CPUSTATES];
	unsigned int cpuindex_, nbr_;
};


#endif
