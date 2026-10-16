//
//  Copyright (c) 2026 Jonathan Snow
//
//  This file may be distributed under terms of the GPL.
//

#ifndef _DISKMETER_H_
#define _DISKMETER_H_

#include "fieldmetergraph.h"

#include <cstdint>

class DiskMeter : public FieldMeterGraph {
public:
  DiskMeter(XOSView *parent, double max);
  ~DiskMeter(void);

  const char *name(void) const { return "DiskMeter"; }
  void checkevent(void);
  void checkResources(void);

protected:
  bool getstats(uint64_t &bytesRead, uint64_t &bytesWritten);

private:
  uint64_t lastBytesRead_, lastBytesWritten_;
  double maxBandwidth_;
  bool haveBaseline_, unavailable_;
};

#endif
