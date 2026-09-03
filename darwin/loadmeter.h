//
//  Copyright (c) 1994, 1995 Mike Romberg
//  Copyright (c) 2026 Jonathan Snow (Darwin support)
//
//  This file may be distributed under terms of the GPL
//

#ifndef _LOADMETER_H_
#define _LOADMETER_H_

#include "fieldmetergraph.h"

class LoadMeter : public FieldMeterGraph {
public:
  LoadMeter(XOSView *parent);
  ~LoadMeter(void);

  const char *name(void) const { return "LoadMeter"; }
  void checkevent(void);
  void checkResources(void);

protected:
  void getloadinfo(void);
  unsigned long procloadcol_, warnloadcol_, critloadcol_;

private:
  float warnThreshold_, critThreshold_;
  int alarmstate_, lastalarmstate_;
  bool unavailable_;
};

#endif
