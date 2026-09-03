//
//  Copyright (c) 1994, 1995 Mike Romberg
//  Copyright (c) 2026 Jonathan Snow (Darwin support)
//
//  This file may be distributed under terms of the GPL
//

#ifndef _CPUMETER_H_
#define _CPUMETER_H_

#include "fieldmetergraph.h"

#include <array>
#include <mach/processor_info.h>
#include <vector>

class CPUMeter : public FieldMeterGraph {
public:
  CPUMeter(XOSView *parent, unsigned int cpu = 0);
  ~CPUMeter(void);

  const char *name(void) const { return "CPUMeter"; }
  void checkevent(void);
  void checkResources(void);

  static unsigned int countCPUs(void);

private:
  typedef std::array<natural_t, CPU_STATE_MAX> TickSet;

  unsigned int cpu_;
  bool initialized_;
  bool unavailable_;
  std::vector<TickSet> previous_;

  bool getcputime(void);
};

#endif
