//
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  NetBSD port:
//  Copyright (c) 1995, 1996, 1997-2002 by Brian Grayson (bgrayson@netbsd.org)
//
//  This file was originally written by Brian Grayson for the NetBSD and
//    xosview projects.
//  The NetBSD memmeter was improved by Tom Pavel (pavel@slac.stanford.edu)
//    to provide active and inactive values, rather than just "used."
//
//  This file may be distributed under terms of the GPL or of the BSD
//    license, whichever you choose.  The full license notices are
//    contained in the files COPYING.GPL and COPYING.BSD, which you
//    should have received.  If not, contact one of the xosview
//    authors for a copy.
//

#include <stdlib.h>
#include "kernel.h"
#include "memmeter.h"


MemMeter::MemMeter( XOSView *parent )
	: FieldMeterGraph( parent, 5, "MEM", "ACT/INACT/WRD/CA/FRE" ) {
	BSDPageInit();
}

MemMeter::~MemMeter( void ) {

}

void MemMeter::checkResources( void ) {
	FieldMeterGraph::checkResources();

	setfieldcolor( 0, parent_->getResource("memActiveColor") );
	setfieldcolor( 1, parent_->getResource("memInactiveColor") );
	setfieldcolor( 2, parent_->getResource("memCacheColor") );
	setfieldcolor( 3, parent_->getResource("memBufferColor") );
	setfieldcolor( 4, parent_->getResource("memFreeColor") );
	priority_ = atoi( parent_->getResource("memPriority") );
	dodecay_ = parent_->isResourceTrue("memDecay");
	useGraph_ = parent_->isResourceTrue("memGraph");
	SetUsedFormat( parent_->getResource("memUsedFormat") );
}

void MemMeter::checkevent( void ) {
	getmeminfo();
	drawfields();
}

void MemMeter::getmeminfo( void ) {
	BSDGetPageStats(meminfo_, NULL);
	for (int i = 0; i < 5; i++)
		fields_[i] = (double)meminfo_[i];

	total_ = fields_[0] + fields_[1] + fields_[2] + fields_[3] + fields_[4];
	setUsed(total_ - fields_[4], total_);
}
