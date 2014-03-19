//
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "intmeter.h"
#include "kernel.h"
#include <stdlib.h>
#include <strings.h>
#include <sstream>


IntMeter::IntMeter( XOSView *parent, const char *, const char *, int dolegends, int dousedlegends )
	: BitMeter( parent, "INTS", "IRQs", 1, dolegends, dousedlegends ) {
	if (!BSDIntrInit())
		disableMeter();
	irqcount_ = BSDNumInts();
	irqs_ = (uint64_t *)calloc(irqcount_ + 1, sizeof(uint64_t));
	lastirqs_ = (uint64_t *)calloc(irqcount_ + 1, sizeof(uint64_t));
	inbrs_ = (unsigned int *)calloc(irqcount_ + 1, sizeof(int));
	updateirqcount(true);
}

IntMeter::~IntMeter( void ) {
	free(irqs_);
	free(lastirqs_);
	free(inbrs_);
}

void IntMeter::checkevent( void ) {
	getirqs();

	for (uint i = 0 ; i <= irqcount_ ; i++) {
		if (inbrs_[i] != 0) {
			if (realintnum_.find(i) == realintnum_.end()) {   // new interrupt number
				updateirqcount();
				return;
			}
			bits_[realintnum_[i]] = ((irqs_[i] - lastirqs_[i]) != 0);
			lastirqs_[i] = irqs_[i];
		}
	}
	bzero(inbrs_, (irqcount_ + 1) * sizeof(inbrs_[0]));
	bzero(irqs_, (irqcount_ + 1) * sizeof(irqs_[0]));

	BitMeter::checkevent();
}

void IntMeter::checkResources( void ) {
	BitMeter::checkResources();
	onColor_  = parent_->allocColor( parent_->getResource( "intOnColor" ) );
	offColor_ = parent_->allocColor( parent_->getResource( "intOffColor" ) );
	priority_ = atoi( parent_->getResource( "intPriority" ) );
}

void IntMeter::getirqs( void ) {
	BSDGetIntrStats(irqs_, inbrs_);
}

void IntMeter::updateirqcount( bool init ) {
	int count = 16;

	if (init) {
		getirqs();
		for (int i = 0; i < 16; i++)
			realintnum_[i] = i;
	}
	for (uint i = 16; i <= irqcount_; i++) {
		if (inbrs_[i] != 0) {
			realintnum_[i] = count++;
			inbrs_[i] = 0;
		}
	}
	setNumBits(count);

	// Build the legend.
	std::ostringstream os;
	os << "0";
	if (realintnum_.upper_bound(15) == realintnum_.end()) // only 16 ints
		os << "-15";
	else {
		int prev = 15, prev2 = 14;
		for (std::map<int,int>::const_iterator it = realintnum_.upper_bound(15),
		     end = realintnum_.end(); it != end; ++it) {
			if ( &*it == &*realintnum_.rbegin() ) { // last element
				if (it->first == prev + 1)
					os << "-" ;
				else {
					if (prev == prev2 + 1)
						os << "-" << prev;
					os << "," ;
				}
				os << it->first;
			}
			else {
				if (it->first != prev + 1) {
					if (prev == prev2 + 1)
						os << "-" << prev;
					os << "," << it->first ;
				}
			}
			prev2 = prev;
			prev = it->first;
		}
		os << std::ends;
	}
	legend(os.str().c_str());
}
