//  
//  Copyright (c) 1994 by Mike Romberg ( romberg@fsl.noaa.gov )
//  Modifications from FieldMeter class done in Oct. 1995
//    by Brian Grayson ( bgrayson@pine.ece.utexas.edu )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _FIELDMETERDECAY_H_
#define _FIELDMETERDECAY_H_

#include "meter.h"
#include "fieldmeter.h"

class FieldMeterDecay : public FieldMeter {
public:
  FieldMeterDecay( XOSView *parent, int numfields,
              const char *title = "", const char *legend = "", 
              int dolegends = 0, int dousedlegends = 0 );
  virtual ~FieldMeterDecay( void );
  
  virtual void drawfields( int manditory = 0 );

protected:
  int dodecay_;
  int firsttime_;  //  Used to set up decaying fields right the first time.
  float *decay_;
  float *lastDecayval_;
private:
};

#endif
