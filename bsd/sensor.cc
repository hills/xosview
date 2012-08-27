//
//  Copyright (c) 2012 by Tomi Tapper <tomi.o.tapper@jyu.fi>
//
//  File based on linux/lmstemp.* by
//  Copyright (c) 2000, 2006 by Leopold Toetsch <lt@toetsch.at>
//
//  This file may be distributed under terms of the GPL
//
//
//

#include <stdlib.h>
#include "kernel.h"
#include "sensor.h"


BSDSensor::BSDSensor( XOSView *parent, const char *name, const char *high, const char *label, const char *caption, int nbr )
	: FieldMeter( parent, 3, label, caption, 1, 1, 1 ) {
	name_ = name;
	high_ = high;
	nbr_ = nbr;
}

BSDSensor::~BSDSensor( void ) {
}

void BSDSensor::checkResources( void ) {
	FieldMeter::checkResources();

	setfieldcolor( 0, parent_->getResource( "bsdsensorActColor" ) );
	setfieldcolor( 1, parent_->getResource( "bsdsensorIdleColor") );
	setfieldcolor( 2, parent_->getResource( "bsdsensorHighColor" ) );

	priority_ = atoi( parent_->getResource( "bsdsensorPriority" ) );
	if ('0' < high_[0] && high_[0] <= '9') {  // high given as number
		total_ = atoi( high_.c_str() );
		high_.erase();
	}
	else
		total_ = 100;  // guess something and adjust later

	char s[30];
	snprintf(s, 30, "bsdsensorUsedFormat%d", nbr_);
	const char *f = parent_->getResourceOrUseDefault(s, NULL);
	SetUsedFormat( f ? f : parent_->getResource( "bsdsensorUsedFormat" ) );
}

void BSDSensor::checkevent( void ) {
	getsensor();
	drawfields();
}

void BSDSensor::getsensor( void ) {
	float value;
	BSDGetSensor( name_.substr(0, name_.find_first_of('.')).c_str(),
	              name_.substr(name_.find_first_of('.') + 1).c_str(), &value );
	if ( !high_.empty() ) {
		float high;
		BSDGetSensor( high_.substr(0, high_.find_first_of('.')).c_str(),
		              high_.substr(high_.find_first_of('.') + 1).c_str(), &high );
		if (high != total_) {
			char l[20];
			snprintf(l, 20, "ACT/HIGH/%d", (int)high);
			legend(l);
			drawlegend();
			total_ = high;
		}
	}
	fields_[0] = (double)value;
	fields_[1] = total_ - fields_[0];
	if (fields_[1] < 0.0) {
		fields_[1] = 0.0;
		setfieldcolor( 0, parent_->getResource( "bsdsensorHighColor" ) );
	}
	else
		setfieldcolor( 0, parent_->getResource( "bsdsensorActColor" ) );
	setUsed(fields_[0], total_);
}
