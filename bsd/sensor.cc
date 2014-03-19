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

#include "sensor.h"
#include "kernel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <string>


BSDSensor::BSDSensor( XOSView *parent, const char *name, const char *high,
                      const char *low, const char *label, const char *caption, int nbr )
	: SensorFieldMeter( parent, label, caption, 1, 1, 0 ) {
	nbr_ = nbr;
	highname_[0] = highval_[0] = '\0';
	lowname_[0] = lowval_[0] = '\0';
	std::string n(name), tmp;
	tmp = n.substr( 0, n.find_first_of('.') );
	strncpy(name_, tmp.c_str(), NAMESIZE);
	tmp = n.substr( n.find_first_of('.') + 1 );
	strncpy(val_, tmp.c_str(), NAMESIZE);
	if (high) {
		has_high_ = true;
		if ( sscanf(high, "%lf", &high_) == 0 ) {  // high given as number?
			n = high;
			tmp = n.substr( 0, n.find_first_of('.') );
			strncpy(highname_, tmp.c_str(), NAMESIZE);
			tmp = n.substr( n.find_first_of('.') + 1 );
			strncpy(highval_, tmp.c_str(), NAMESIZE);
		}
	}
	if (low) {
		has_low_ = true;
		if ( sscanf(low, "%lf", &low_) == 0 ) {  // low given as number?
			n = low;
			tmp = n.substr( 0, n.find_first_of('.') );
			strncpy(lowname_, tmp.c_str(), NAMESIZE);
			tmp = n.substr( n.find_first_of('.') + 1 );
			strncpy(lowval_, tmp.c_str(), NAMESIZE);
		}
	}
}

BSDSensor::~BSDSensor( void ) {
}

void BSDSensor::checkResources( void ) {
	SensorFieldMeter::checkResources();

	actcolor_  = parent_->allocColor( parent_->getResource( "bsdsensorActColor" ) );
	highcolor_ = parent_->allocColor( parent_->getResource( "bsdsensorHighColor" ) );
	lowcolor_  = parent_->allocColor( parent_->getResource( "bsdsensorLowColor" ) );
	setfieldcolor( 0, actcolor_  );
	setfieldcolor( 1, parent_->getResource( "bsdsensorIdleColor" ) );
	setfieldcolor( 2, highcolor_ );
	priority_ = atoi( parent_->getResource( "bsdsensorPriority" ) );

	char s[32];
	const char *tmp = parent_->getResourceOrUseDefault( "bsdsensorHighest", "0" );
	snprintf(s, 32, "bsdsensorHighest%d", nbr_);
	total_ = fabs( atof( parent_->getResourceOrUseDefault(s, tmp) ) );
	snprintf(s, 32, "bsdsensorUsedFormat%d", nbr_);
	const char *f = parent_->getResourceOrUseDefault(s, NULL);
	SetUsedFormat( f ? f : parent_->getResource( "bsdsensorUsedFormat" ) );

	if (!has_high_)
		high_ = total_;
	if (!has_low_)
		low_ = 0;

	// Get the unit.
	float dummy;
	BSDGetSensor(name_, val_, &dummy, unit_);
	updateLegend();
}

void BSDSensor::checkevent( void ) {
	getsensor();
	drawfields();
}

void BSDSensor::getsensor( void ) {
	float value, high = high_, low = low_;
	BSDGetSensor(name_, val_, &value);
	if ( strlen(highname_) )
		BSDGetSensor(highname_, highval_, &high);
	if ( strlen(lowname_) )
		BSDGetSensor(lowname_, lowval_, &low);

	fields_[0] = value;
	checkFields(low, high);
}
