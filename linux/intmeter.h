//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _INTMETER_H_
#define _INTMETER_H_

#include "bitmeter.h"


class IntMeter : public BitMeter {
public:
  IntMeter( XOSView *parent,
	    const char *title = "", const char *legend ="",
	    int dolegends = 0, int dousedlegends = 0 );
  ~IntMeter( void );

  void checkevent( void );

  void checkResources( void );
protected:
  unsigned long irqs_[16], lastirqs_[16];

  void getirqs( void );
};

#endif
