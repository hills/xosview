//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
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

class Timer {
public:
  Timer( int start = 0 ) { if ( start ) Timer::start(); }
  ~Timer( void ){}

  void start( void ) { gettimeofday( &starttime_, NULL ); }
  void stop( void )  { gettimeofday( &stoptime_, NULL );  }
  //  reports time intervall between calls to start and stop in usec
  long report( void ) const { 
    return (stoptime_.tv_sec - starttime_.tv_sec) * 1000000
           + stoptime_.tv_usec - starttime_.tv_usec; 
  }
  
  ostream &printOn(ostream &os) const {
    return os <<"Timer : ["
      <<"starttime_ = " <<TimeVal(starttime_)
      <<", stoptime_ = " <<TimeVal(stoptime_)
      <<", duration = " <<report() <<"]";
  }

protected:
  struct timeval starttime_, stoptime_;

private:
};

inline ostream &operator<<(ostream &os, const Timer &t){
  return t.printOn(os);
}

#endif
