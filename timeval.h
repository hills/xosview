//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _TIMEVAL_H_
#define _TIMEVAL_H_

#include <sys/time.h>
#include <iostream.h>

class TimeVal {
public:
  TimeVal(unsigned long sec = 0, unsigned long usec = 0) {
    _val.tv_sec = sec;
    _val.tv_usec = usec;
  }
  TimeVal(const struct timeval &val) { _val = val; }

  unsigned long sec(void) const { return _val.tv_sec; }
  unsigned long usec(void) const { return _val.tv_usec; }
  void sec(unsigned long s) { _val.tv_sec = s; }
  void usec(unsigned long us) { _val.tv_usec = us; }

  operator struct timeval(void) const { return _val; }

  ostream &printOn(ostream &os) const {
    return os <<"(" <<sec() <<" sec, " <<usec() <<" usec)";
  }

private:
  struct timeval _val;
};

inline ostream &operator<<(ostream &os, const TimeVal &tv){
  return tv.printOn(os);
}

#endif
