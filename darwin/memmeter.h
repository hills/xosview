//
//  Copyright (c) 2026 Jonathan Snow
//
//  This file may be distributed under terms of the GPL.
//

#ifndef _MEMMETER_H_
#define _MEMMETER_H_

#include "fieldmetergraph.h"

class MemMeter : public FieldMeterGraph {
public:
  MemMeter(XOSView *parent);
  ~MemMeter(void);

  const char *name(void) const { return "MemMeter"; }
  void checkevent(void);
  void checkResources(void);

protected:
  void getmeminfo(void);

private:
  bool unavailable_;
};

#endif
