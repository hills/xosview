//  
//  Original FieldMeter class is Copyright (c) 1994 by Mike Romberg 
//    ( romberg@fsl.noaa.gov )
//
//  Modifications from FieldMeter class done in Oct. 1995
//    by Brian Grayson ( bgrayson@netbsd.org )
//
//  Modifications from FieldMeterDecay class done in Oct. 1998
//    by Scott McNab ( jedi@tartarus.uwa.edu.au )
//

#ifndef _FIELDMETERGRAPH_H_
#define _FIELDMETERGRAPH_H_

#define FIELDMETERGRAPH_H_CVSID "$Id$"

#include "meter.h"
#include "fieldmeterdecay.h"

class FieldMeterGraph : public FieldMeterDecay {
public:
  FieldMeterGraph( XOSView *parent, int numfields,
              const char *title = "", const char *legend = "", 
              int docaptions = 0, int dolegends = 0, int dousedlegends = 0 );
  virtual ~FieldMeterGraph( void );
  
  virtual void drawfields( int manditory = 0 );

  virtual void checkResources( void );

protected:
  void setNumCols( int n );

  int useGraph_;
  int graphNumCols_;
  int graphpos_;
  int firstTimeDrawn_;
  /*  There's some sort of corruption going on -- we can't have
   *  variables after the heightfield_ below, otherwise they get
   *  corrupted???  */
  float *heightfield_;
private:
  void drawBar( int i );
};

#endif
