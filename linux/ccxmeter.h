//
//  Copyright (c) 1994, 1995, 2004, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _CCXMETER_H_
#define _CCXMETER_H_

#include "fieldmetergraph.h"
#include "xosview.h"


class CCXMeter : public FieldMeterGraph {
public:
  CCXMeter(XOSView *parent, const int ccxId);
  ~CCXMeter(void);

  const char *name(void) const { return "CCXMeter"; }
  void checkevent(void);

  void checkResources(void);

  static int countCCXs(void);
  static int getKernelVersion(void);

protected:
  static const int smtCount = 2; // two threads per core
  static const int ccxSize = 4 * smtCount; // 4 cores per ccx
  int _ccxId;
  int _lineNumStart;
  unsigned long long cputime_[2][10];
  int cpuindex_;
  int kernel_;
  int statfields_;

  std::string to_string(int);
  void getcputime(void);
  int findLine(const int ccxId);
private:
};

#endif
