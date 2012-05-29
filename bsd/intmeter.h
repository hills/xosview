//
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _INTMETER_H_
#define _INTMETER_H_

#include "bitmeter.h"
#include "kernel.h"

#include <map>

class IntMeter : public BitMeter {
public:
  IntMeter( XOSView *parent,
	    const char *title = "", const char *legend ="",
	    int dolegends = 0, int dousedlegends = 0 );
  ~IntMeter( void );

  void checkevent( void );

  void checkResources( void );
private:
  unsigned long *irqs_, *lastirqs_;
  unsigned int *inbrs_;
  unsigned int irqcount_;
  std::map<int,int> realintnum;
protected:
  void getirqs( void );
  void updateirqcount( bool init = false );
};

#endif
