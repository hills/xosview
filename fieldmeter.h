//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//
// $Id$
//
#ifndef _FIELDMETER_H_
#define _FIELDMETER_H_

#define FIELDMETER_H_CVSID "$Id$"

#include "meter.h"
#include "timer.h"

class FieldMeter : public Meter {
public:
  FieldMeter( XOSView *parent, int numfields,
	      const char *title = "", const char *legend = "", 
	      int dolegends = 0, int dousedlegends = 0 );
  virtual ~FieldMeter( void );
  
  virtual void drawfields( int manditory = 0 );
  void setfieldcolor( int field, const char *color );
  void setfieldcolor( int field, unsigned long color);
  void dolegends( int val ) { dolegends_ = val; }
  void dousedlegends( int val ) { dousedlegends_ = val; }
  void reset( void );
    /*  These next two are deprecated -- use setUsed instead.  bgrayson  */
  void used_obsolete( int val ) { print_ = PERCENT; used_ = val; }
  void absolute_obsolete( float val ) { print_ = FLOAT; used_ = val; }

  void setUsed (float val, float total);
  void draw( void );
  void checkevent( void );
  void disableMeter ( void );
  
  virtual void checkResources( void );

protected:
  enum UsedType { INVALID_0, FLOAT, PERCENT, AUTOSCALE, INVALID_TAIL };

  int numfields_;
  float *fields_;
  float total_, used_, lastused_;
  int *lastvals_, *lastx_;
  unsigned long *colors_;
  unsigned long usedcolor_;
  UsedType print_;
  int printedZeroTotalMesg_;
  int numWarnings_;

  void SetUsedFormat ( const char * const str );
  void drawlegend( void );
  void drawused( int manditory );
  bool checkX(int x, int width) const;

  void setNumFields(int n);


private:
  Timer _timer;
protected:
  void IntervalTimerStart() { _timer.start(); }
  void IntervalTimerStop() { _timer.stop(); }
  long long IntervalTimeInMicrosecs() { return _timer.report(); }
};

#endif
