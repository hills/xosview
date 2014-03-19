//
//  NetBSD port:
//  Copyright (c) 1995,1996,1997 Brian Grayson(bgrayson@netbsd.org)
//
//  This file was written by Brian Grayson for the NetBSD and xosview
//    projects.
//  This file may be distributed under terms of the GPL or of the BSD
//    license, whichever you choose.  The full license notices are
//    contained in the files COPYING.GPL and COPYING.BSD, which you
//    should have received.  If not, contact one of the xosview
//    authors for a copy.
//

#ifndef _DISKMETER_H_
#define _DISKMETER_H_

#include "fieldmetergraph.h"
#include "xosview.h"
#include "defines.h"


class DiskMeter : public FieldMeterGraph {
public:
	DiskMeter( XOSView *parent, double max );
	~DiskMeter( void );

	const char *name( void ) const { return "DiskMeter"; }
	void checkevent( void );
	void checkResources( void );

protected:
	void getstats( void );

private:
#ifndef HAVE_DEVSTAT
	uint64_t prevreads_, prevwrites_;
#endif
	double maxBandwidth_;
};


#endif
