//
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//  Copyright (c) 1995, 1996, 1997-2002 by Brian Grayson (bgrayson@netbsd.org)
//
//  Most of this code was written by Werner Fink <werner@suse.de>.
//  Only small changes were made on my part (M.R.)
//  And the near-trivial port to NetBSD was done by Brian Grayson
//
//  This file may be distributed under terms of the GPL or of the BSD
//    license, whichever you choose.  The full license notices are
//    contained in the files COPYING.GPL and COPYING.BSD, which you
//    should have received.  If not, contact one of the xosview
//    authors for a copy.
//

#include "loadmeter.h"
#include "kernel.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>


LoadMeter::LoadMeter( XOSView *parent )
	: FieldMeterGraph( parent, 2, "LOAD", "PROCS/MIN", 1, 1, 0 ) {
	total_ = -1.0;
}

LoadMeter::~LoadMeter( void ) {
}

void LoadMeter::checkResources( void ) {
	FieldMeterGraph::checkResources();

	procloadcol_ = parent_->allocColor( parent_->getResource("loadProcColor") );
	warnloadcol_ = parent_->allocColor( parent_->getResource("loadWarnColor") );
	critloadcol_ = parent_->allocColor( parent_->getResource("loadCritColor") );

	setfieldcolor( 0, procloadcol_ );
	setfieldcolor( 1, parent_->getResource("loadIdleColor") );
	priority_ = atoi( parent_->getResource("loadPriority") );
	dodecay_ = parent_->isResourceTrue("loadDecay");
	useGraph_ = parent_->isResourceTrue("loadGraph");
	SetUsedFormat( parent_->getResource("loadUsedFormat") );
	do_cpu_speed_ = parent_->isResourceTrue("loadCpuSpeed");

	const char *warn = parent_->getResource("loadWarnThreshold");
	if (strncmp(warn, "auto", 2) == 0)
		warnThreshold_ = BSDCountCpus();
	else
		warnThreshold_ = atoi(warn);

	const char *crit = parent_->getResource("loadCritThreshold");
	if (strncmp(crit, "auto", 2) == 0)
		critThreshold_ = warnThreshold_ * 4;
	else
		critThreshold_ = atoi(crit);

	alarmstate_ = lastalarmstate_ = 0;

	if (dodecay_) {
		//  Warning:  Since the loadmeter changes scale occasionally, old
		//  decay values need to be rescaled.  However, if they are rescaled,
		//  they could go off the edge of the screen.  Thus, for now, to
		//  prevent this whole problem, the load meter can not be a decay
		//  meter.  The load is a decaying average kind of thing anyway,
		//  so having a decaying load average is redundant.
		std::cerr << "Warning:  The loadmeter can not be configured as a decay\n"
		          << "  meter.  See the source code (" << __FILE__ << ") for further\n"
		          << "  details.\n";
		dodecay_ = 0;
	}
}

void LoadMeter::checkevent( void ) {
	getloadinfo();

	if (do_cpu_speed_) {
		old_cpu_speed_ = cur_cpu_speed_;
		cur_cpu_speed_ = BSDGetCPUSpeed();

		if (old_cpu_speed_ != cur_cpu_speed_) {
			char l[25];
			snprintf(l, 25, "PROCS/MIN %d MHz", cur_cpu_speed_);
			legend(l);
			drawlegend();
		}
	}
	drawfields();
}

void LoadMeter::getloadinfo( void ) {
	double oneMinLoad;

	getloadavg(&oneMinLoad, 1);  //  Only get the 1-minute-average sample.
	fields_[0] = oneMinLoad;

	if (fields_[0] <  warnThreshold_)
		alarmstate_ = 0;
	else if (fields_[0] >= critThreshold_)
		alarmstate_ = 2;
	else
		alarmstate_ = 1;

	if (alarmstate_ != lastalarmstate_) {
		if (alarmstate_ == 0)
			setfieldcolor(0, procloadcol_);
		else if (alarmstate_ == 1)
			setfieldcolor(0, warnloadcol_);
		else
			setfieldcolor(0, critloadcol_);
		drawlegend();
		lastalarmstate_ = alarmstate_;
	}

	// Adjust total to next power-of-two of the current load.
	if ( (fields_[0]*5.0 < total_ && total_ > 1.0) || fields_[0] > total_ ) {
		unsigned int i = fields_[0];
		i |= i >> 1; i |= i >> 2; i |= i >> 4; i |= i >> 8; i |= i >> 16;  // i = 2^n - 1
		total_ = i + 1;
	}

	fields_[1] = total_ - fields_[0];
	setUsed(fields_[0], total_);
}
