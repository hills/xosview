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

#include "MeterMaker.h"
#include "defines.h"
#include "kernel.h"
#include "loadmeter.h"
#include "cpumeter.h"
#include "memmeter.h"
#include "swapmeter.h"
#include "pagemeter.h"
#include "netmeter.h"
#include "diskmeter.h"
#include "intmeter.h"
#include "intratemeter.h"
#include "btrymeter.h"
#if defined(__i386__) || defined(__x86_64__)
#include "coretemp.h"
#endif
#include "sensor.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>


MeterMaker::MeterMaker(XOSView *xos) {
	_xos = xos;
}

void MeterMaker::makeMeters(void) {
	// Standard meters (usually added, but users could turn them off)
	if ( _xos->isResourceTrue("load") )
		push(new LoadMeter(_xos));

	if ( _xos->isResourceTrue("cpu") ) {
		bool single, both, all;
		unsigned int cpuCount = BSDCountCpus();

		single = ( strncmp(_xos->getResource("cpuFormat"), "single", 2) == 0 );
		both = ( strncmp(_xos->getResource("cpuFormat"), "both", 2) == 0 );
		all = ( strncmp(_xos->getResource("cpuFormat"), "all", 2) == 0 );

		if ( strncmp(_xos->getResource("cpuFormat"), "auto", 2) == 0 ) {
			if (cpuCount == 1 || cpuCount > 4)
				single = true;
			else
				all = true;
		}

		if (single || both)
			push(new CPUMeter(_xos, 0));

		if (all || both) {
			for (unsigned int i = 1; i <= cpuCount; i++)
				push(new CPUMeter(_xos, i));
		}
	}

	if ( _xos->isResourceTrue("mem") )
		push(new MemMeter(_xos));

	if ( _xos->isResourceTrue("swap") )
		push(new SwapMeter(_xos));

	if ( _xos->isResourceTrue("page") )
		push(new PageMeter(_xos, atof(_xos->getResource("pageBandwidth"))));

	if ( _xos->isResourceTrue("net") )
		push(new NetMeter(_xos, atof(_xos->getResource("netBandwidth"))));

	if ( _xos->isResourceTrue("disk") )
		push(new DiskMeter(_xos, atof(_xos->getResource("diskBandwidth"))));

	if ( _xos->isResourceTrue("interrupts") )
		push(new IntMeter(_xos));

	if ( _xos->isResourceTrue("irqrate") )
		push(new IrqRateMeter(_xos));

	if ( _xos->isResourceTrue("battery") && BSDHasBattery() )
		push(new BtryMeter(_xos));

#if defined(__i386__) || defined(__x86_64__)
	if ( _xos->isResourceTrue("coretemp") && CoreTemp::countCpus() > 0 ) {
		char caption[32];
		snprintf(caption, 32, "ACT(\260C)/HIGH/%s",
		         _xos->getResourceOrUseDefault( "coretempHighest", "100" ) );
		const char *displayType = _xos->getResourceOrUseDefault("coretempDisplayType", "separate");
		if (strncmp(displayType, "separate", 1) == 0) {
			char name[5];
			for (uint i = 0; i < CoreTemp::countCpus(); i++) {
				snprintf(name, 5, "CPU%d", i);
				push(new CoreTemp(_xos, name, caption, i));
			}
		}
		else if (strncmp(displayType, "average", 1) == 0)
			push(new CoreTemp(_xos, "CPU", caption, -1));
		else if (strncmp(displayType, "maximum", 1) == 0)
			push(new CoreTemp(_xos, "CPU", caption, -2));
		else {
			std::cerr << "Unknown value of coretempDisplayType: " << displayType << std::endl;
			_xos->done(1);
		}
	}
#endif

	if ( _xos->isResourceTrue("bsdsensor") ) {
		char caption[16], l[8], s[16];
		for (int i = 1 ; ; i++) {
			snprintf(s, 16, "bsdsensorHighest%d", i);
			float highest = atof( _xos->getResourceOrUseDefault(s, "100") );
			snprintf(caption, 16, "ACT/HIGH/%f", highest);
			snprintf(s, 16, "bsdsensor%d", i);
			const char *name = _xos->getResourceOrUseDefault(s, NULL);
			if (!name || !*name)
				break;
			snprintf(s, 16, "bsdsensorHigh%d", i);
			const char *high = _xos->getResourceOrUseDefault(s, NULL);
			snprintf(s, 16, "bsdsensorLow%d", i);
			const char *low = _xos->getResourceOrUseDefault(s, NULL);
			snprintf(s, 16, "bsdsensorLabel%d", i);
			snprintf(l, 8, "SEN%d", i);
			const char *label = _xos->getResourceOrUseDefault(s, l);
			push(new BSDSensor(_xos, name, high, low, label, caption, i));
		}
	}
}
