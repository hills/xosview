//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//
// $Id$
//
#ifndef _CPUMETER_H_
#define _CPUMETER_H_

#include "fieldmeterdecay.h"

class CPUMeter : public FieldMeterDecay {
public:
  CPUMeter(XOSView *parent, const char *cpuID = "cpu");
  ~CPUMeter(void);

  const char *name(void) const { return "CPUMeter"; }
  void checkevent(void);

  void checkResources(void);

  static int countCPUs(void);
  static const char *cpuStr(int num);
protected:
  int _lineNum;
  float cputime_[2][4];
  int cpuindex_;

  void getcputime(void);
  int findLine(const char *cpuID);
  const char *toUpper(const char *str);
private:
};

#endif
