//
//  Copyright (c) 1999 by Brian Grayson (bgrayson@netbsd.org)
//
//  This file may be distributed under terms of the GPL or of the BSD
//    license, whichever you choose.  The full license notices are
//    contained in the files COPYING.GPL and COPYING.BSD, which you
//    should have received.  If not, contact one of the xosview
//    authors for a copy.
//

#include "intratemeter.h"
#include "kernel.h"
#include <stdlib.h>
#include <strings.h>
#include <err.h>


IrqRateMeter::IrqRateMeter( XOSView *parent )
	: FieldMeterGraph( parent, 2, "IRQs", "IRQs per sec/IDLE", 1, 1, 0 ) {
	if (!BSDIntrInit()) {
		warnx("The kernel does not seem to have the symbols needed for the IrqRateMeter.");
		warnx("The IrqRateMeter has been disabled.");
		disableMeter();
	}
	irqcount_ = BSDNumInts();
	irqs_ = (uint64_t *)calloc(irqcount_ + 1, sizeof(uint64_t));
	lastirqs_ = (uint64_t *)calloc(irqcount_ + 1, sizeof(uint64_t));
}

IrqRateMeter::~IrqRateMeter( void ) {
	free(irqs_);
	free(lastirqs_);
}

void IrqRateMeter::checkResources( void ) {
	FieldMeterGraph::checkResources();
	setfieldcolor( 0, parent_->getResource("irqrateUsedColor") );
	setfieldcolor( 1, parent_->getResource("irqrateIdleColor") );
	priority_ = atoi( parent_->getResource("irqratePriority") );
	dodecay_ = parent_->isResourceTrue("irqrateDecay");
	useGraph_ = parent_->isResourceTrue("irqrateGraph");
	SetUsedFormat( parent_->getResource("irqrateUsedFormat") );
	total_ = 2000;

	BSDGetIntrStats(lastirqs_, NULL);
}

void IrqRateMeter::checkevent( void ) {
	getinfo();
	drawfields();
}

void IrqRateMeter::getinfo( void ) {
	int delta = 0;

	IntervalTimerStop();
	BSDGetIntrStats(irqs_, NULL);

	for (uint i = 0; i <= irqcount_; i++) {
		delta += irqs_[i] - lastirqs_[i];
		lastirqs_[i] = irqs_[i];
	}
	bzero(irqs_, (irqcount_ + 1) * sizeof(irqs_[0]));

	/*  Scale delta by the priority.  */
	fields_[0] = delta / IntervalTimeInSecs();

	//  Bump total_, if needed.
	if (fields_[0] > total_)
		total_ = fields_[0];

	setUsed(fields_[0], total_);
	IntervalTimerStart();
}
