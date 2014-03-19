//
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  NetBSD port:
//  Copyright (c) 1995, 1996, 1997-2002 by Brian Grayson (bgrayson@netbsd.org)
//
//  This file was originally written by Brian Grayson for the NetBSD and
//    xosview projects.
//
//  This file may be distributed under terms of the GPL or of the BSD
//    license, whichever you choose.  The full license notices are
//    contained in the files COPYING.GPL and COPYING.BSD, which you
//    should have received.  If not, contact one of the xosview
//    authors for a copy.
//

#include "pagemeter.h"
#include "kernel.h"
#include <stdlib.h>


PageMeter::PageMeter( XOSView *parent, double total )
	: FieldMeterGraph( parent, 3, "PAGE", "IN/OUT/IDLE" ) {
	total_ = total;
	BSDPageInit();
	BSDGetPageStats(NULL, previnfo_);
}

PageMeter::~PageMeter( void ) {
}

void PageMeter::checkResources( void ) {
	FieldMeterGraph::checkResources();

	setfieldcolor( 0, parent_->getResource("pageInColor") );
	setfieldcolor( 1, parent_->getResource("pageOutColor") );
	setfieldcolor( 2, parent_->getResource("pageIdleColor") );
	priority_ = atoi( parent_->getResource("pagePriority") );
	dodecay_ = parent_->isResourceTrue("pageDecay");
	useGraph_ = parent_->isResourceTrue("pageGraph");
	SetUsedFormat( parent_->getResource("pageUsedFormat") );
}

void PageMeter::checkevent( void ) {
	getpageinfo();
	drawfields();
}

void PageMeter::getpageinfo( void ) {
	uint64_t info[2];
	BSDGetPageStats(NULL, info);

	fields_[0] = info[0] - previnfo_[0];
	fields_[1] = info[1] - previnfo_[1];
	previnfo_[0] = info[0];
	previnfo_[1] = info[1];

	if (total_ < fields_[0] + fields_[1])
		total_ = fields_[0] + fields_[1];

	fields_[2] = total_ - fields_[0] - fields_[1];
	setUsed(total_ - fields_[2], total_);
}
