//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  NetBSD port:  
//  Copyright (c) 1995 Brian Grayson(bgrayson@pine.ece.utexas.edu)
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
  Host *_thisHost;
  Timer _timer;
  //  NetBSD:  Use long long, so we won't run into problems after 4 GB
  //  has been transferred over the net!
  long long _lastBytesIn, _lastBytesOut;

  void adjust(void);
};

#endif
