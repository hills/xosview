//
//  Copyright (c) 2008 by Tomi Tapper <tomi.o.tapper@jyu.fi>
//
//  Read coretemp reading with sysctl and display actual temperature.
//  If actual >= high, actual temp changes color to indicate alarm.
//
//  File based on linux/lmstemp.* by
//  Copyright (c) 2000, 2006 by Leopold Toetsch <lt@toetsch.at>
//
//  This file may be distributed under terms of the GPL
//
//
//

#include "coretemp.h"
#include "kernel.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>


CoreTemp::CoreTemp( XOSView *parent, const char *label, const char *caption, int cpu)
	: FieldMeter( parent, 3, label, caption, 1, 1, 1 ) {
	metric_ = true;
	cpu_ = cpu;
	cpucount_ = countCpus();
	temps_ = (float *)calloc(cpucount_, sizeof(float));
}

CoreTemp::~CoreTemp( void ) {
	free(temps_);
}

void CoreTemp::checkResources( void ) {
	FieldMeter::checkResources();

	actcolor_  = parent_->allocColor( parent_->getResource( "coretempActColor" ) );
	highcolor_ = parent_->allocColor( parent_->getResource( "coretempHighColor" ) );
	setfieldcolor( 0, actcolor_ );
	setfieldcolor( 1, parent_->getResource( "coretempIdleColor") );
	setfieldcolor( 2, highcolor_ );

	priority_ = atoi( parent_->getResource( "coretempPriority" ) );
	const char *highest = parent_->getResourceOrUseDefault( "coretempHighest", "100" );
	total_ = atoi( highest );
	const char *high = parent_->getResourceOrUseDefault( "coretempHigh", NULL );
	SetUsedFormat( parent_->getResource( "coretempUsedFormat" ) );

	// Get tjMax here and use as total.
	float total = -300.0;
	float *tjmax = (float *)calloc(cpucount_, sizeof(float));
	BSDGetCPUTemperature(temps_, tjmax);
	for (int i = 0; i < cpucount_; i++) {
		if (tjmax[i] > total)
			total = tjmax[i];
	}
	free(tjmax);
	if (total > 0.0)
		total_ = total;

	char l[32];
	if (!high) {
		high_ = total_;
		snprintf(l, 32, "ACT(\260C)/HIGH/%d", (int)total_);
	}
	else {
		high_ = atoi( high );
		snprintf(l, 32, "ACT(\260C)/%d/%d", (int)high_, (int)total_);
	}
	legend(l);
}

unsigned int CoreTemp::countCpus( void ) {
	return BSDGetCPUTemperature(NULL, NULL);
}

void CoreTemp::checkevent( void ) {
	getcoretemp();
	drawfields();
}

void CoreTemp::getcoretemp( void ) {
	BSDGetCPUTemperature(temps_, NULL);

	fields_[0] = 0.0;
	if ( cpu_ >= 0 && cpu_ < cpucount_ ) {  // one core
		fields_[0] = temps_[cpu_];
	}
	else if ( cpu_ == -1 ) {  // average
		float tempval = 0.0;
		for (int i = 0; i < cpucount_; i++)
			tempval += temps_[i];
		fields_[0] = tempval / (float)cpucount_;
	}
	else if ( cpu_ == -2 ) {  // maximum
		float tempval = -300.0;
		for (int i = 0; i < cpucount_; i++) {
			if (temps_[i] > tempval)
				tempval = temps_[i];
		}
		fields_[0] = tempval;
	}
	else {    // should not happen
		std::cerr << "Unknown CPU core number in coretemp." << std::endl;
		parent_->done(1);
		return;
	}

	setUsed(fields_[0], total_);
	if (fields_[0] < 0)
		fields_[0] = 0;
	fields_[1] = high_ - fields_[0];
	fields_[2] = total_ - fields_[1] - fields_[0];
	if (fields_[0] > total_)
		fields_[0] = total_;
	if (fields_[2] < 0)
		fields_[2] = 0;

	if (fields_[1] < 0) { // alarm: T > high
		fields_[1] = 0;
		if (colors_[0] != highcolor_) {
			setfieldcolor( 0, highcolor_ );
			drawlegend();
		}
	}
	else {
		if (colors_[0] != actcolor_) {
			setfieldcolor( 0, actcolor_ );
			drawlegend();
		}
	}
}
