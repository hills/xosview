//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//
// $Id$
//
#ifndef _BITMETER_H_
#define _BITMETER_H_

#define BITMETER_H_CVSID "$Id$"

#include "meter.h"

class XOSView;

class BitMeter : public Meter {
public:
  BitMeter( XOSView *parent,  
	    const char *title = "", const char *legend ="",
	    int numBits = 1, int docaptions = 0, int dolegends = 0, int dousedlegends = 0 );
  virtual ~BitMeter( void );

  void checkevent( void );
  void draw( void );
  void disableMeter ( void );

  int numBits(void) const { return numbits_; }
  void setNumBits(int n);
  
  void checkResources( void );
protected:
  unsigned long onColor_, offColor_;
  char *bits_, *lastbits_;
  int numbits_;
  bool disabled_;

  void setBits(int startbit, unsigned char values);

  void drawBits( int manditory = 0 );
private:
};

#endif
