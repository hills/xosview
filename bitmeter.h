//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _BITMETER_H_
#define _BITMETER_H_

#include "meter.h"

class XOSView;

class BitMeter : public Meter {
public:
  BitMeter( XOSView *parent,  
	    const char *title = "", const char *legend ="",
	    int numBits = 1, int dolegends = 0, int dousedlegends = 0 );
  virtual ~BitMeter( void );

  void checkevent( void );
  void draw( void );
  
  void checkResources( void );
protected:
  unsigned long onColor_, offColor_;
  char *bits_, *lastbits_;
  int numbits_;

  void setBits(int startbit, unsigned char values);

  void drawBits( int manditory = 0 );
private:
};

#endif
