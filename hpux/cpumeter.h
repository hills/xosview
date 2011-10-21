//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _CPUMETER_H_
#define _CPUMETER_H_

#include "fieldmetergraph.h"

class CPUMeter : public FieldMeterGraph {
public:
  CPUMeter( XOSView *parent );
  ~CPUMeter( void );

  const char *name( void ) const { return "CPUMeter"; }
  void checkevent( void );

  void checkResources(void);
protected:
  float cputime_[2][5];
  int cpuindex_;

  void getcputime( void );
private:
};

#endif
