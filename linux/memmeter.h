//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//
// $Id$
//
#ifndef _MEMMETER_H_
#define _MEMMETER_H_

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
  int _shAdj;

  struct MemStat {
    unsigned long total;
    unsigned long used;
    unsigned long shared;
    unsigned long buff;
    unsigned long cache;
    unsigned long free;
  };

  void getmemstat(MemStat *mstat);
};


#endif
