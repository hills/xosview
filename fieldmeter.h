//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _FIELDMETER_H_
#define _FIELDMETER_H_

#include "meter.h"

class FieldMeter : public Meter {
public:
  FieldMeter( XOSView *parent, int numfields,
	      const char *title = "", const char *legend = "", 
	      int dolegends = 0, int dousedlegends = 0 );
  virtual ~FieldMeter( void );
  
  void drawfields( int manditory = 0 );
  void setfieldcolor( int field, const char *color );
  void setfieldcolor( int field, unsigned long color);
  void dolegends( int val ) { dolegends_ = val; }
  void dousedlegends( int val ) { dousedlegends_ = val; }
  void reset( void );
  void used( int val ) { print_ = PERCENT; used_ = val; }
  void absolute( float val ) { print_ = FLOAT; used_ = val; }
  void draw( void );
  void checkevent( void );
  
  virtual void checkResources( void );

protected:
  enum UsedType { FLOAT, PERCENT };

  int numfields_;
  float *fields_, total_, used_, lastused_;
  int *lastvals_, *lastx_;
  unsigned long *colors_, usedcolor_;
  UsedType print_;

  void drawlegend( void );
  void drawused( int manditory );


private:
};

#endif
