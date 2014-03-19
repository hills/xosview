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

#ifndef _NETMETER_H_
#define _NETMETER_H_

#include "fieldmetergraph.h"
#include "xosview.h"
#include <string>


class NetMeter : public FieldMeterGraph {
public:
	NetMeter( XOSView *parent, double max );
	~NetMeter( void );

	const char *name( void ) const { return "NetMeter"; }
	void checkevent( void );
	void checkResources( void );

protected:
	void getstats(void);

private:
	uint64_t lastBytesIn_, lastBytesOut_;
	double netBandwidth_;
	std::string netIface_;
	bool ignored_;
};


#endif
