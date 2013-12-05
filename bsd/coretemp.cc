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

#include <stdlib.h>
#include <stdio.h>
#include "kernel.h"
#include "coretemp.h"


CoreTemp::CoreTemp( XOSView *parent, const char *label, const char *caption, int cpu)
	: FieldMeter( parent, 2, label, caption, 1, 1, 1 ) {
	metric_ = true;
	cpu_ = cpu;
	checkResources();
}

CoreTemp::~CoreTemp( void ) {
}

void CoreTemp::checkResources( void ) {
	FieldMeter::checkResources();

	setfieldcolor( 0, parent_->getResource( "coretempActColor" ) );
	setfieldcolor( 1, parent_->getResource( "coretempIdleColor") );

	priority_ = atoi( parent_->getResource( "coretempPriority" ) );
	oldtotal_ = total_ = atoi( parent_->getResourceOrUseDefault( "coretempHighest", "100" ) );
	SetUsedFormat( parent_->getResource( "coretempUsedFormat" ) );
}

void CoreTemp::checkevent( void ) {
	getcoretemp();
	drawfields();
}

void CoreTemp::getcoretemp( void ) {
	int cpus = BSDCountCpus();
	float *temps = (float *)calloc(cpus, sizeof(float));
	float *tjmax = (float *)calloc(cpus, sizeof(float));

	BSDGetCPUTemperature(temps, tjmax);

	fields_[0] = 0.0;
	if ( cpu_ >= 0 && cpu_ < cpus ) {  // one core
		fields_[0] = temps[cpu_];
		total_ = ( tjmax[cpu_] > 0.0 ? tjmax[cpu_] : oldtotal_ );
	}
	else if ( cpu_ == -1 ) {  // average
		float tempval = 0.0, temphigh = 0.0;
		for (int i = 0; i < cpus; i++) {
			tempval += temps[i];
			temphigh += ( tjmax[i] > 0.0 ? tjmax[i] : oldtotal_ );
		}
		fields_[0] = tempval / (float)cpus;
		total_ = temphigh / (float)cpus;
	}
	else if ( cpu_ == -2 ) {  // maximum
		float tempval = -300.0, temphigh = -300.0;
		for (int i = 0; i < cpus; i++) {
			if (temps[i] > tempval)
				tempval = temps[i];
			if (tjmax[i] > temphigh)
				temphigh = ( tjmax[i] > 0.0 ? tjmax[i] : oldtotal_ );
		}
		fields_[0] = tempval;
		total_ = temphigh;
	}
	else {    // should not happen
		std::cerr << "Unknown CPU core number in coretemp." << std::endl;
		parent_->done(1);
		return;
	}

	free(temps);
	free(tjmax);

	fields_[1] = total_ - fields_[0];
	if (fields_[1] < 0.0) {   // alarm
		fields_[1] = 0.0;
		setfieldcolor( 0, parent_->getResource( "coretempHighColor" ) );
	}
	else
		setfieldcolor( 0, parent_->getResource( "coretempActColor" ) );

	setUsed(fields_[0], total_);

	if (total_ != oldtotal_) {
		char l[25];
		snprintf(l, 25, "TEMPERATURE (C)/%d", (int)total_);
		legend(l);
		drawlegend();
		oldtotal_ = total_;
	}
}
