//
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//  Copyright (c) 1995, 1996, 1997-2002 by Brian Grayson (bgrayson@netbsd.org)
//
//  Most of this code was written by Werner Fink <werner@suse.de>
//  Only small changes were made on my part (M.R.)
//  And the near-trivial port to NetBSD was by bgrayson.
//
//  This file may be distributed under terms of the GPL or of the BSD
//    license, whichever you choose.  The full license notices are
//    contained in the files COPYING.GPL and COPYING.BSD, which you
//    should have received.  If not, contact one of the xosview
//    authors for a copy.
//

#ifndef _LOADMETER_H_
#define _LOADMETER_H_

#include "fieldmetergraph.h"
#include "xosview.h"


class LoadMeter : public FieldMeterGraph {
public:
	LoadMeter( XOSView *parent );
	~LoadMeter( void );

	const char *name( void ) const { return "LoadMeter"; }
	void checkevent( void );
	void checkResources( void );

protected:
	void getloadinfo( void );

private:
	unsigned long procloadcol_, warnloadcol_, critloadcol_;
	int warnThreshold_, critThreshold_, alarmstate_, lastalarmstate_;
	int old_cpu_speed_, cur_cpu_speed_;
	bool do_cpu_speed_;
};


#endif
