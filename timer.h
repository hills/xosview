//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _TIMER_H_
#define _TIMER_H_

//
//                 General purpose interval timer class
//
//  Implemented using BSD derived function gettimeofday for greater resolution
//
//   Author : Mike Romberg

#include "timeval.h"
#include <sys/time.h>
#include <iostream>


class Timer {
public:
  Timer( int start = 0 ) { if ( start ) Timer::start(); }
  ~Timer( void ){}

  void start( void ) { gettimeofday( &starttime_, NULL ); }
  void stop( void )  { gettimeofday( &stoptime_, NULL );  }

  //  This one uses doubles as the return value, to avoid
  //  overflow/sign problems.
  double report_usecs(void) const {
    return (stoptime_.tv_sec - starttime_.tv_sec) * 1000000.0
      + stoptime_.tv_usec - starttime_.tv_usec;
  }

  std::ostream &printOn(std::ostream &os) const {
    return os <<"Timer : ["
      <<"starttime_ = " <<TimeVal(starttime_)
      <<", stoptime_ = " <<TimeVal(stoptime_)
      <<", duration = " <<report_usecs() <<" usecs]";
  }

protected:
  struct timeval starttime_, stoptime_;

private:
};

inline std::ostream &operator<<(std::ostream &os, const Timer &t){
  return t.printOn(os);
}

#endif
