//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//
// $Id$
//
#ifndef _TIMER_H_
#define _TIMER_H_

#define TIMER_H_CVSID "$Id$"

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
  //  XXX  NOTE THAT THIS SUFFERS FROM OVERFLOW!
  //       After 53 minutes, report() will return a negative number!
  //       To be safe, we should use the one that returns a double.
  //       Once the new function appears to work fine, this whole
  //       region can be removed from the file.    bgrayson, 11/99
#if 0
  LONG_LONG report( void ) const { 
    err << "This function Timer::report() should no longer be used!\n";
    exit(-1);
    return (stoptime_.tv_sec - starttime_.tv_sec) * 1000000
           + stoptime_.tv_usec - starttime_.tv_usec; 
  }
#endif
  //  This one uses doubles as the return value, to avoid
  //  overflow/sign problems.
  double report_usecs(void) const {
    return (stoptime_.tv_sec - starttime_.tv_sec) * 1000000.0
      + stoptime_.tv_usec - starttime_.tv_usec;
  }
  
  ostream &printOn(ostream &os) const {
    return os <<"Timer : ["
      <<"starttime_ = " <<TimeVal(starttime_)
      <<", stoptime_ = " <<TimeVal(stoptime_)
      <<", duration = " <<report_usecs() <<" usecs]";
  }

protected:
  struct timeval starttime_, stoptime_;

private:
};

inline ostream &operator<<(ostream &os, const Timer &t){
  return t.printOn(os);
}

#endif
