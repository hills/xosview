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

#include "swapmeter.h"
#include "kernel.h"
#include <stdlib.h>


SwapMeter::SwapMeter( XOSView *parent )
	: FieldMeterGraph( parent, 2, "SWAP", "USED/FREE" ) {
	BSDSwapInit();
}

SwapMeter::~SwapMeter( void ) {
}

void SwapMeter::checkResources( void ) {
	FieldMeterGraph::checkResources();

	setfieldcolor( 0, parent_->getResource("swapUsedColor") );
	setfieldcolor( 1, parent_->getResource("swapFreeColor") );
	priority_ = atoi( parent_->getResource("swapPriority") );
	dodecay_ = parent_->isResourceTrue("swapDecay");
	useGraph_ = parent_->isResourceTrue("swapGraph");
	SetUsedFormat( parent_->getResource("swapUsedFormat") );
}

void SwapMeter::checkevent( void ) {
	getswapinfo();
	drawfields();
}

void SwapMeter::getswapinfo( void ) {
	uint64_t total = 0, used = 0;

	BSDGetSwapInfo(&total, &used);

	total_ = (double)total;
	if ( total_ == 0.0 )
		total_ = 1.0;  /* We don't want any division by zero, now, do we?  :) */
	fields_[0] = (double)used;
	fields_[1] = total_;

	setUsed(fields_[0], total_);
}
