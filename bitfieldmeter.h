//  
//  Copyright (c) 1999 Thomas Waldmann (ThomasWaldmann@gmx.de)
//  based on work of Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//
#ifndef _BITFIELDMETER_H_
#define _BITFIELDMETER_H_

#include "meter.h"
#include "timer.h"

class BitFieldMeter : public Meter {
public:
  BitFieldMeter( XOSView *parent, int numBits = 1, int numfields = 1,
	      const char *title = "",
	      const char *bitlegend = "", const char *fieldlegend = "", 
	      int docaptions = 0, int dolegends = 0, int dousedlegends = 0 );
  virtual ~BitFieldMeter( void );
  
  virtual void drawfields( int manditory = 0 );
  void drawBits( int manditory = 0 );

  void setfieldcolor( int field, const char *color );
  void setfieldcolor( int field, unsigned long color);
  void docaptions( int val ) { docaptions_ = val; }
  void dolegends( int val ) { dolegends_ = val; }
  void dousedlegends( int val ) { dousedlegends_ = val; }
  void reset( void );

  void setUsed (float val, float total);
  void setBits(int startbit, unsigned char values);

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
  
  unsigned long onColor_, offColor_;
  char *bits_, *lastbits_;
  int numbits_;

  void SetUsedFormat ( const char * const str );
  void drawfieldlegend( void );
  void drawused( int manditory );
  bool checkX(int x, int width) const;

  void setNumFields(int n);
  void setNumBits(int n);
  char *fieldLegend_;

  void setfieldlegend(const char *fieldlegend);

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

