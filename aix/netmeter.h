//
//  Copyright (c) 2026 by Antoni Sawicki ( as@tenoware.com )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _NETMETER_H_
#define _NETMETER_H_

#include "fieldmetergraph.h"
#include "xosview.h"
#include <string>


class NetMeter : public FieldMeterGraph {
public:
  NetMeter( XOSView *parent, float max );
  ~NetMeter( void );

  const char *name( void ) const { return "NetMeter"; }
  void checkevent( void );

  void checkResources( void );

protected:
  void getnetstats( void );

private:
  float maxpackets_;
  bool ok_;
  double lastBytesIn_, lastBytesOut_;
  bool first_;
  std::string netIface_;
  bool ignored_;
};

#endif
