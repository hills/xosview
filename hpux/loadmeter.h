//  
//  Copyright (c) 1994, 1995, 1997 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _LOADMETER_H_
#define _LOADMETER_H_


#include "fieldmeterdecay.h"

class LoadMeter : public FieldMeterDecay {
public:
  LoadMeter( XOSView *parent );
  ~LoadMeter( void );

  const char *name( void ) const { return "LoadMeter"; }  
  void checkevent( void );

  void checkResources( void );
protected:

  void getloadinfo( void );
  unsigned long procloadcol_, warnloadcol_;
private:
  int alarmThreshold;
};


#endif
