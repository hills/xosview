//
//  Copyright (c) 2014 by Tomi Tapper (tomi.o.tapper@jyu.fi)
//
//  Based on bsd/intratemeter.* by
//    Copyright (c) 1999 by Brian Grayson (bgrayson@netbsd.org)
//  and on linux/intmeter.* by
//    Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "intratemeter.h"
#include "cpumeter.h"
#include <stdlib.h>
#include <fstream>
#include <string>
#include <iostream>

static const char *INTFILE = "/proc/interrupts";


IrqRateMeter::IrqRateMeter( XOSView *parent )
  : FieldMeterGraph( parent, 2, "IRQs", "IRQs per sec/IDLE", 1, 1, 0 ) {
  _lastirqs = 0;
  _cpucount = CPUMeter::countCPUs();
}

IrqRateMeter::~IrqRateMeter( void ) {
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
}

void IrqRateMeter::checkevent( void ) {
  getinfo();
  drawfields();
}

void IrqRateMeter::getinfo( void ) {
  std::ifstream intfile(INTFILE);
  std::string line;
  unsigned long long count = 0;
  unsigned long tmp;
  const char *cur = NULL;
  char *end = NULL;

  if (!intfile) {
    std::cerr << "Can not open file : " << INTFILE << std::endl;
    parent_->done(1);
    return;
  }

  IntervalTimerStop();
  intfile.ignore(1024, '\n');

  // sum all interrupts on all cpus
  while ( !intfile.eof() ) {
    unsigned int i = 0;
    std::getline(intfile, line);
    if ( line.find_first_of("0123456789") > line.find_first_of(':') )
      break;  // reached non-numeric interrupts
    tmp = strtoul(line.c_str(), &end, 10);
    cur = end + 1;
    while (*cur && i++ < _cpucount) {
      tmp = strtoul(cur, &end, 10);
      count += tmp;
      cur = end;
    }
  }
  if (_lastirqs == 0)  // first run
    _lastirqs = count;
  fields_[0] = (count - _lastirqs) / IntervalTimeInSecs();
  IntervalTimerStart();
  _lastirqs = count;

  // Bump total, if needed.
  if (fields_[0] > total_)
    total_ = fields_[0];

  setUsed(fields_[0], total_);
}
