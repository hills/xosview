//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _FIELDMETER_H_
#define _FIELDMETER_H_

#include "meter.h"
#include "xosview.h"
#include "timer.h"


class FieldMeter : public Meter {
public:
  FieldMeter( XOSView *parent, int numfields,
	      const char *title = "", const char *legend = "",
	      int docaptions = 0, int dolegends = 0, int dousedlegends = 0 );
  virtual ~FieldMeter( void );

  virtual void drawfields( int mandatory = 0 );
  void setfieldcolor( int field, const char *color );
  void setfieldcolor( int field, unsigned long color);
  void docaptions( int val ) { docaptions_ = val; }
  void dolegends( int val ) { dolegends_ = val; }
  void dousedlegends( int val ) { dousedlegends_ = val; }
  void reset( void );
  void setUsed (double val, double total);
  void draw( void );
  void checkevent( void );
  void disableMeter ( void );

  virtual void checkResources( void );

protected:
  enum UsedType { INVALID_0, FLOAT, PERCENT, AUTOSCALE, INVALID_TAIL };

  int numfields_;
  double *fields_;
  double total_, used_, lastused_;
  int *lastvals_, *lastx_;
  unsigned long *colors_;
  unsigned long usedcolor_;
  UsedType print_;
  int printedZeroTotalMesg_;
  int numWarnings_;
  bool metric_;
  int usedoffset_;

  void SetUsedFormat ( const char * const str );
  void drawlegend( void );
  void drawused( int mandatory );
  bool checkX(int x, int width) const;

  void setNumFields(int n);


private:
  Timer _timer;
protected:
  void IntervalTimerStart() { _timer.start(); }
  void IntervalTimerStop() { _timer.stop(); }
  //  Before, we simply called _timer.report(), which returns usecs.
  //  However, it suffers from wrap/overflow/sign-bit problems, so
  //  instead we use doubles for everything.
  double IntervalTimeInMicrosecs() { return _timer.report_usecs(); }
  double IntervalTimeInSecs() { return _timer.report_usecs()/1e6; }
};

#endif
