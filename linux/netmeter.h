//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _NETMETER_H_
#define _NETMETER_H_

#include "fieldmetergraph.h"
#include "xosview.h"
#include <string>

#define NETFILENAME "/proc/net/dev"


class NetMeter : public FieldMeterGraph {
public:
  NetMeter(XOSView *parent, float max);
  ~NetMeter( void );

  const char *name( void ) const { return "NetMeter"; }
  void checkevent( void );

  void checkResources( void );

private:
  float _maxpackets;
  std::string _netIface;
  bool _ignored;
  unsigned long long _lastBytesIn, _lastBytesOut;
};


#endif
