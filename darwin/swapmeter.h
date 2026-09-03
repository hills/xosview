//
//  Copyright (c) 2026 Jonathan Snow
//
//  This file may be distributed under terms of the GPL.
//

#ifndef _SWAPMETER_H_
#define _SWAPMETER_H_

#include "fieldmetergraph.h"

class SwapMeter : public FieldMeterGraph {
public:
  SwapMeter(XOSView *parent);
  ~SwapMeter(void);

  const char *name(void) const { return "SwapMeter"; }
  void checkevent(void);
  void checkResources(void);

protected:
  void getswapinfo(void);

private:
  bool unavailable_;
};

#endif
