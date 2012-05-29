//
//  Copyright (c) 1997 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//  Ported to NetBSD by David W. Talmage
//  (talmage@jefferson.cmf.nrl.navy.mil)
//

#ifndef _BTRYMETER_H_
#define _BTRYMETER_H_

#include "defines.h"
#include "fieldmeter.h"

#ifdef HAVE_BATTERY_METER

class BtryMeter : public FieldMeter {
public:
  BtryMeter( XOSView *parent );
  ~BtryMeter( void );

  const char *name( void ) const { return "BtryMeter"; }
  void checkevent( void );

  void checkResources( void );
protected:

  void getpwrinfo( void );
private:
  int alarmThreshold;
};

#endif  // HAVE_BATTERY_METER

#endif
