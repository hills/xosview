//
//  Original FieldMeter class is Copyright (c) 1994, 2006 by Mike Romberg
//    ( mike.romberg@noaa.gov )
//  Modifications from FieldMeter class done in Oct. 1995
//    by Brian Grayson ( bgrayson@netbsd.org )
//
//  This file was written by Brian Grayson for the NetBSD and xosview
//    projects.
//  This file may be distributed under terms of the GPL or of the BSD
//    license, whichever you choose.  The full license notices are
//    contained in the files COPYING.GPL and COPYING.BSD, which you
//    should have received.  If not, contact one of the xosview
//    authors for a copy.
//

#ifndef _FIELDMETERDECAY_H_
#define _FIELDMETERDECAY_H_

#define FIELDMETERDECAY_H_CVSID "$Id$"

#include "meter.h"
#include "fieldmeter.h"

class FieldMeterDecay : public FieldMeter {
public:
  FieldMeterDecay( XOSView *parent, int numfields,
              const char *title = "", const char *legend = "",
              int docaptions = 0, int dolegends = 0, int dousedlegends = 0 );
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
