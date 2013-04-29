//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _INTMETER_H_
#define _INTMETER_H_

#include "bitmeter.h"
#include "cpumeter.h"

class IntMeter : public BitMeter {
public:
  IntMeter( XOSView *parent, int cpu = 0);
  ~IntMeter( void );

  void checkevent( void );

  void checkResources( void );

  static int countCPUs(void) { return CPUMeter::countCPUs(); }

protected:
  unsigned long *irqs_, *lastirqs_;

  int _cpu;
  bool _old;
  bool separate_;
  const int max;

  void getirqs( void );
  void updateirqcount( int n, bool init );
  void initirqcount( void );
};

#endif
