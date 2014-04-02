//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _NETMETER_H_
#define _NETMETER_H_

#include "fieldmetergraph.h"
#include "xosview.h"
#include "kstats.h"
#include <kstat.h>
#include <net/if.h>
#include <string>


class NetMeter : public FieldMeterGraph {
public:
  NetMeter( XOSView *parent, kstat_ctl_t *kc, float max );
  ~NetMeter( void );

  const char *name( void ) const { return "NetMeter"; }
  void checkevent( void );
  void checkResources( void );

protected:
  void getnetstats( void );

private:
  float _maxpackets;
  uint64_t _lastBytesIn, _lastBytesOut;
  kstat_ctl_t *_kc;
  KStatList *_nets;
  std::string _netIface;
  bool _ignored;
  struct lifreq _lfr;
  int _socket;
};

#endif
