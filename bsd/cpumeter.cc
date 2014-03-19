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

#include "cpumeter.h"
#include "kernel.h"
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>


CPUMeter::CPUMeter( XOSView *parent, unsigned int nbr )
	: FieldMeterGraph( parent, 5, "CPU", "USR/NICE/SYS/INT/FREE" ) {
	nbr_ = nbr;
	cpuindex_ = 0;
	bzero(cputime_, 10 * sizeof(cputime_[0][0]));
	BSDCPUInit();

	char t[8] = "CPU";
	if (nbr_ > 0)
		snprintf(t, 8, "CPU%d", nbr_ - 1);
	title(t);
}

CPUMeter::~CPUMeter( void ) {
}

void CPUMeter::checkResources( void ) {
	FieldMeterGraph::checkResources();

	setfieldcolor( 0, parent_->getResource("cpuUserColor") );
	setfieldcolor( 1, parent_->getResource("cpuNiceColor") );
	setfieldcolor( 2, parent_->getResource("cpuSystemColor") );
	setfieldcolor( 3, parent_->getResource("cpuInterruptColor") );
	setfieldcolor( 4, parent_->getResource("cpuFreeColor") );
	priority_ = atoi( parent_->getResource("cpuPriority") );
	dodecay_ = parent_->isResourceTrue("cpuDecay");
	useGraph_ = parent_->isResourceTrue("cpuGraph");
	SetUsedFormat( parent_->getResource("cpuUsedFormat") );
}

void CPUMeter::checkevent( void ) {
	getcputime();
	drawfields();
}

void CPUMeter::getcputime( void ) {
	uint64_t tempCPU[CPUSTATES];
	total_ = 0;

	BSDGetCPUTimes(tempCPU, nbr_);

	int oldindex = (cpuindex_ + 1) % 2;
	for (int i = 0; i < CPUSTATES; i++) {
		cputime_[cpuindex_][i] = tempCPU[i];
		fields_[i] = cputime_[cpuindex_][i] - cputime_[oldindex][i];
		total_ += fields_[i];
	}
	if (total_) {
		setUsed(total_ - fields_[4], total_);
		cpuindex_ = (cpuindex_ + 1) % 2;
	}
}
