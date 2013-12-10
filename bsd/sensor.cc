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


BSDSensor::BSDSensor( XOSView *parent, const char *name, const char *high,
                      const char *low, const char *label, const char *caption, int nbr )
	: FieldMeter( parent, 3, label, caption, 1, 1, 0 ) {
	metric_ = true;
	hashigh_ = false;
	nbr_ = nbr;
	high_ = low_ = 0.0;
	highname_[0] = highval_[0] = '\0';
	lowname_[0] = lowval_[0] = '\0';
	unit_[0] = '\0';
	std::string n(name), tmp;
	tmp = n.substr( 0, n.find_first_of('.') );
	strncpy(name_, tmp.c_str(), NAMESIZE);
	tmp = n.substr( n.find_first_of('.') + 1 );
	strncpy(val_, tmp.c_str(), NAMESIZE);
	if (high) {
		hashigh_ = true;
		if ('0' <= high[0] && high[0] <= '9')  // high given as number
			high_ = atof(high);
		else {
			n = high;
			tmp = n.substr( 0, n.find_first_of('.') );
			strncpy(highname_, tmp.c_str(), NAMESIZE);
			tmp = n.substr( n.find_first_of('.') + 1 );
			strncpy(highval_, tmp.c_str(), NAMESIZE);
		}
	}
	if (low) {
		if ('0' <= low[0] && low[0] <= '9')  // low given as number
			low_ = atof(low);
		else {
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

void BSDSensor::updateLegend( void ) {
	char l[32];
	if (hashigh_) {
		if (total_ < 10) {
			if ( strlen(unit_) )
				snprintf(l, 32, "ACT(%s)/%.1f/%.1f", unit_, high_, total_);
			else
				snprintf(l, 32, "ACT/%.1f/%.1f", high_, total_);
		}
		else {
			if ( strlen(unit_) )
				snprintf(l, 32, "ACT(%s)/%d/%d", unit_, (int)high_, (int)total_);
			else
				snprintf(l, 32, "ACT/%d/%d", (int)high_, (int)total_);
		}
	}
	else {
		if (total_ < 10) {
			if ( strlen(unit_) )
				snprintf(l, 32, "ACT(%s)/HIGH/%.1f", unit_, total_);
			else
				snprintf(l, 32, "ACT/HIGH/%.1f", total_);
		}
		else {
			if ( strlen(unit_) )
				snprintf(l, 32, "ACT(%s)/HIGH/%d", unit_, (int)total_);
			else
				snprintf(l, 32, "ACT/HIGH/%d", (int)total_);
		}
	}
	legend(l);
}

void BSDSensor::checkResources( void ) {
	FieldMeter::checkResources();

	actcolor_  = parent_->allocColor( parent_->getResource( "bsdsensorActColor" ) );
	highcolor_ = parent_->allocColor( parent_->getResource( "bsdsensorHighColor" ) );
	lowcolor_  = parent_->allocColor( parent_->getResource( "bsdsensorLowColor" ) );
	setfieldcolor( 0, actcolor_  );
	setfieldcolor( 1, parent_->getResource( "bsdsensorIdleColor" ) );
	setfieldcolor( 2, highcolor_ );
	priority_ = atoi( parent_->getResource( "bsdsensorPriority" ) );

	char s[32];
	const char *tmp = parent_->getResourceOrUseDefault( "bsdsensorHighest", "100" );
	snprintf(s, 32, "bsdsensorHighest%d", nbr_);
	total_ = atof( parent_->getResourceOrUseDefault(s, tmp) );
	snprintf(s, 32, "bsdsensorUsedFormat%d", nbr_);
	const char *f = parent_->getResourceOrUseDefault(s, NULL);
	SetUsedFormat( f ? f : parent_->getResource( "bsdsensorUsedFormat" ) );

	if (!hashigh_)
		high_ = total_;

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
	float value;
	BSDGetSensor(name_, val_, &value);
	if ( strlen(highname_) ) {
		float high;
		BSDGetSensor(highname_, highval_, &high);
		if (high != high_) {
			high_ = high;
			if (high_ > total_)
				total_ = 10 * (int)((high_ * 1.25) / 10);
			updateLegend();
			drawlegend();
		}
	}
	if ( strlen(lowname_) )
		BSDGetSensor(lowname_, lowval_, &low_);

	fields_[0] = value;
	fields_[1] = high_ - fields_[0];
	if (fields_[1] < 0.0) { // alarm: T > max
		fields_[1] = 0.0;
		if (colors_[0] != highcolor_) {
			setfieldcolor( 0, highcolor_ );
			drawlegend();
		}
	}
	else if (fields_[0] < low_) { // alarm: T < min
		if (colors_[0] != lowcolor_) {
			setfieldcolor(0, lowcolor_);
			drawlegend();
		}
	}
	else {
		if (colors_[0] != actcolor_) {
			setfieldcolor(0, actcolor_);
			drawlegend();
		}
	}

	setUsed(fields_[0], total_);
	fields_[2] = total_ - fields_[1] - fields_[0];
	if (fields_[0] > total_)
		fields_[0] = total_;
	if (fields_[2] < 0.0)
		fields_[2] = 0.0;
}
