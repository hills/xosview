//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  NetBSD port:  
//  Copyright (c) 1995 Brian Grayson(bgrayson@pine.ece.utexas.edu)
//
//  This file may be distributed under terms of the GPL
//
//
// $Id$
//
#ifndef _MEMMETER_H_
#define _MEMMETER_H_

#define MEMMETER_H_CVSID "$Id: "

#include "fieldmeterdecay.h"

class MemMeter : public FieldMeterDecay {
public:
  MemMeter( XOSView *parent );
  ~MemMeter( void );

  const char *name( void ) const { return "MemMeter"; }  
  void checkevent( void );

  void checkResources( void );
protected:

  void getmeminfo( void );
private:
};


#endif
