//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _NETMETER_H_
#define _NETMETER_H_

#include "fieldmeterdecay.h"
#include "timer.h"

class Host;

class NetMeter : public FieldMeterDecay {
public:
  NetMeter(XOSView *parent, float max);
  ~NetMeter( void );

  const char *name( void ) const { return "NetMeter"; }  
  void checkevent( void );

  void checkResources( void );
protected:
  float maxpackets_;

private:
  int _ipsock;
  Timer _timer;
  unsigned long _lastBytesIn, _lastBytesOut;

  void adjust(void);
};

#endif
