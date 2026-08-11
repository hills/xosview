//
//  Copyright (c) 2026 by Antoni Sawicki ( as@tenoware.com )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _CPUMETER_H_
#define _CPUMETER_H_

#include "fieldmetergraph.h"
#include "xosview.h"


class CPUMeter : public FieldMeterGraph {
public:
  CPUMeter( XOSView *parent );
  ~CPUMeter( void );

  const char *name( void ) const { return "CPUMeter"; }
  void checkevent( void );

  void checkResources( void );

  static int countCPUs( void );

protected:
  double cputime_[2][4];
  int cpuindex_;
  bool ok_;

  void getcputime( void );
};

#endif
