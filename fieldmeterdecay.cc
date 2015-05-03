//
//  The original FieldMeter class is Copyright (c) 1994, 2006 by Mike Romberg
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

#include "fieldmeterdecay.h"
#include <iostream>


FieldMeterDecay::FieldMeterDecay( XOSView *parent,
                int numfields, const char *title,
                const char *legend, int docaptions, int dolegends,
                int dousedlegends )
: FieldMeter (parent, numfields, title, legend, docaptions, dolegends,
              dousedlegends)
{
  decay_ = new double[numfields];
  lastDecayval_ = new double[numfields];
  firsttime_ = 1;
  dodecay_ = 1;
}

FieldMeterDecay::~FieldMeterDecay( void ){
  delete[] decay_;
  delete[] lastDecayval_;
}

void FieldMeterDecay::drawfields( int mandatory ){
  int twidth, x = x_;
  int decay_changed = 0;

  if (!dodecay_)
  {
    //  If this meter shouldn't be done as a decaying splitmeter,
    //  call the ordinary fieldmeter code.
    FieldMeter::drawfields (mandatory);
    return;
  }

  if ( total_ == 0.0 )
    return;

  int halfheight = height_ / 2;
  int decaytwidth, decayx = x_;

  //  This code is supposed to make the average display look just like
  //  the ordinary display for the first drawfields, but it doesn't seem
  //  to work too well.  But it's better than setting all decay_ fields
  //  to 0.0 initially!

  if (firsttime_) {
    firsttime_ = 0;
    mandatory = 1;
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

    /*  This is majorly ugly code.  It needs a rewrite.  BCG  */
    /*  I think one good way to do it may be to normalize all of the
     *  fields_ in a temporary array into the range 0.0 .. 1.0,
     *  calculate the shifted starting positions and ending positions
     *  for coloring, multiply by the pixel width of the meter, and
     *  then turn to ints.  I think this will solve a whole bunch of
     *  our problems with rounding that before we tackled at a whole
     *  lot of places.  BCG */
  for ( int i = 0 ; i < numfields_ ; i++ ){

    decay_[i] = ALPHA*decay_[i] + (1-ALPHA)*(fields_[i]*1.0/total_);

    //  We want to round the widths, rather than truncate.
    twidth = (int) (0.5 + (width_ * (double) fields_[i]) / total_);
    decaytwidth = (int) (0.5 + width_ * decay_[i]);
    if (decaytwidth < 0.0) {
        std::cerr << "Error:  FieldMeterDecay " << name() << ":  decaytwidth of ";
        std::cerr << decaytwidth << ", width of " << width_ << ", decay_[" << i;
        std::cerr << "] of " << decay_[i] << std::endl;
    }

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
    parent_->setStippleN(i%4);

    //  drawFilledRectangle() adds one to its width and height.
    //    Let's correct for that here.
    if ( mandatory || (twidth != lastvals_[i]) || (x != lastx_[i]) ){
      if (!checkX(x, twidth))
        std::cerr <<__FILE__ << ":" << __LINE__ <<std::endl;
      parent_->drawFilledRectangle( x, y_, twidth, halfheight );
    }

    if ( mandatory || decay_changed || (decay_[i] != lastDecayval_[i]) ){
      if (!checkX(decayx, decaytwidth))
        std::cerr <<__FILE__ << ":" << __LINE__ <<std::endl;
      decay_changed = 1;
      parent_->drawFilledRectangle( decayx, y_+halfheight+1,
            decaytwidth, height_ - halfheight-1);
    }

    lastvals_[i] = twidth;
    lastx_[i] = x;
    lastDecayval_[i] = decay_[i];

    parent_->setStippleN(0);	/*  Restore all-bits stipple.  */
    if ( dousedlegends_ )
      drawused( mandatory );
    x += twidth;

    decayx += decaytwidth;

  }
}
