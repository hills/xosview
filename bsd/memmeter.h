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

#ifndef _MEMMETER_H_
#define _MEMMETER_H_

#include "fieldmetergraph.h"
#include "xosview.h"


class MemMeter : public FieldMeterGraph {
public:
	MemMeter( XOSView *parent );
	~MemMeter( void );

	const char *name( void ) const { return "MemMeter"; }
	void checkevent( void );
	void checkResources( void );

protected:
	void getmeminfo( void );

private:
	uint64_t meminfo_[5];
};


#endif
