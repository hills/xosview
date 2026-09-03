//
//  Copyright (c) 2026 Jonathan Snow
//
//  This file may be distributed under terms of the GPL.
//

#ifndef _NETMETER_H_
#define _NETMETER_H_

#include "fieldmetergraph.h"

#include <cstdint>
#include <string>

class NetMeter : public FieldMeterGraph {
public:
  NetMeter(XOSView *parent, double max);
  ~NetMeter(void);

  const char *name(void) const { return "NetMeter"; }
  void checkevent(void);
  void checkResources(void);

protected:
  bool getstats(uint64_t &bytesIn, uint64_t &bytesOut);

private:
  uint64_t lastBytesIn_, lastBytesOut_;
  double netBandwidth_;
  std::string netIface_;
  bool ignored_, haveBaseline_, unavailable_;
};

#endif
