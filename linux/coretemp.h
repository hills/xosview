//
//  Copyright (c) 2008 by Tomi Tapper <tomi.o.tapper@jyu.fi>
//
//  File based on linux/lmstemp.* by
//  Copyright (c) 2000, 2006 by Leopold Toetsch <lt@toetsch.at>
//
//  This file may be distributed under terms of the GPL
//
//
//
#ifndef _CORETEMP_H_
#define _CORETEMP_H_

#include "cpumeter.h"
#include "fieldmeter.h"
#include <vector>

class CoreTemp : public FieldMeter {
public:
  CoreTemp( XOSView *parent, const char *label, const char *caption, int pkg, int cpu);
  ~CoreTemp( void );

  const char *name( void ) const { return "CoreTemp"; }
  void checkevent( void );
  void checkResources( void );

  static unsigned int countCpus(int pkg);

protected:
  void getcoretemp( void );

private:
  int   _pkg;
  int   _cpu;
  int   _high;
  std::vector<int> _cpus;   // sysfs indices (tempn_input) for max and avg
};


#endif
