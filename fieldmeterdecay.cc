//  
//  Copyright (c) 1994 by Mike Romberg ( romberg@fsl.noaa.gov )
//  Modifications from FieldMeter class done in Oct. 1995
//    by Brian Grayson ( bgrayson@pine.ece.utexas.edu )
//
//  This file may be distributed under terms of the GPL
//


// In order to use the FieldMeterDecay class in place of a FieldMeter class in
// a meter file (say, cpumeter.cc), make the following changes:
//   1.  Change cpumeter.h to include fieldmeterdecay.h instead of
//       fieldmeter.h
//   2.  Change CPUMeter to inherit from FieldMeterDecay, rather than
//       FieldMeter.
//   3.  Change the constructor call to use FieldMeterDecay(), rather than
//       FieldMeter().
//   4.  Make the checkResources () function in the meter set the 
//	 dodecay_ variable according to the, e.g., xosview*cpuDecay resource.

#include "fieldmeter.h"
#include "fieldmeterdecay.h"
#include "xosview.h"
#include <fstream.h>
#include <stdio.h>

FieldMeterDecay::FieldMeterDecay( XOSView *parent,
                int numfields, const char *title,
                const char *legend, int dolegends, int dousedlegends )
: FieldMeter (parent, numfields, title, legend, dolegends, dousedlegends)
{
  decay_ = new float[numfields];
  lastDecayval_ = new float[numfields];
  for (int decayCtr = 0; decayCtr < numfields; decayCtr++) {
    decay_[decayCtr] = 0.0;
    lastDecayval_[decayCtr] = 0.0;
  }
  decay_[numfields-1] = 1.0;  //  Initialize to all free...
  firsttime_ = 1;
  dodecay_ = 1;
}

FieldMeterDecay::~FieldMeterDecay( void ){
  delete[] decay_;
  delete[] lastDecayval_;
}

void FieldMeterDecay::drawfields( int manditory ){
  int twidth, x = x_;

  if (!dodecay_)
  {
    //  If this meter shouldn't be done as a decaying splitmeter,
    //  call the ordinary fieldmeter code.
    FieldMeter::drawfields (manditory);
    return;
  }
  int halfheight = height_ / 2;
  int decaytwidth, decayx = x_;

  if ( total_ == 0 )
    return;
  
  //  This code is supposed to make the average display look just like
  //  the ordinary display for the first drawfields, but it doesn't seem
  //  to work too well.  But it's better than setting all decay_ fields
  //  to 0.0 initially!

  if ((total_ != 0.0) && firsttime_) {
    firsttime_ = 0;
    for (int i = 0; i < numfields_; i++) 
         {
                decay_[i] = 1.0*fields_[i]/total_;
         }
  }

  //  Update the decay fields.  This is not quite accurate, since if
  //  the screen is refreshed, we will update the decay fields more
  //  often than we need to.  However, this makes the decay stuff
  //  TOTALLY independent of the ????Meter methods.

  //  The constant below can be modified for quicker or slower
  //  exponential rates for the average.  No fancy math is done to
  //  set it to correspond to a five-second decay or anything -- I
  //  just played with it until I thought it looked good!  :)  BCG
#define ALPHA 0.97

  if (total_ != 0.0)
    for ( int i = 0 ; i < numfields_ ; i++ ){
      decay_[i] = ALPHA*decay_[i] + (1-ALPHA)*(fields_[i]*1.0/total_);
    }
    
  for ( int i = 0 ; i < numfields_ ; i++ ){

    //  We want to round the widths, rather than truncate.
    twidth = (int) (0.5 + (width_ * (float) fields_[i]) / total_); 
    decaytwidth = (int) (0.5 + width_ * decay_[i]);

    //  However, due to rounding, we may have gone one
    //  pixel too far by the time we get to the later fields...
    if (x + twidth > x_ + width_)
      twidth = width_ + x_ - x;
    if (decayx + decaytwidth > x_ + width_)
      decaytwidth = width_ + x_ - decayx;

    //  Also, due to rounding error, the last field may not go far
    //  enough...
    if ( (i == numfields_ - 1) && ((x + twidth) != (x_ + width_)) )
      twidth = width_ + x_ - x;
    if ( (i == numfields_ - 1) && ((decayx + decaytwidth) != (x_ + width_)))
      decaytwidth = width_ + x_ - decayx;

    parent_->setForeground( colors_[i] );

    if ( manditory || (twidth != lastvals_[i]) || (x != lastx_[i]) )
      parent_->drawFilledRectangle( x, y_, twidth, halfheight );

    if ( manditory || (decay_[i] != lastDecayval_[i]) )
      parent_->drawFilledRectangle( decayx, y_+halfheight,
            decaytwidth, height_ - halfheight);

    lastvals_[i] = twidth;
    lastx_[i] = x;
    lastDecayval_[i] = decay_[i];

    if ( dousedlegends_ )
      drawused( manditory );
    x += twidth;

    decayx += decaytwidth;

  }

  parent_->flush();
}
