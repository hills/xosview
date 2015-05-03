//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "fieldmeter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iostream>


FieldMeter::FieldMeter( XOSView *parent, int numfields, const char *title,
                        const char *legend, int docaptions, int dolegends,
                        int dousedlegends )
: Meter(parent, title, legend, docaptions, dolegends, dousedlegends){
    /*  We need to set print_ to something valid -- the meters
     *  apparently get drawn before the meters have a chance to call
     *  CheckResources() themselves.  */
  numWarnings_ = printedZeroTotalMesg_ = 0;
  print_ = PERCENT;
  metric_ = false;
  usedoffset_ = 0;
  used_ = 0;
  lastused_ = -1;
  fields_ = NULL;
  colors_ = NULL;
  lastvals_ = NULL;
  lastx_ = NULL;
  setNumFields(numfields);
}

void
FieldMeter::disableMeter ( )
{
  setNumFields(1);
  setfieldcolor (0, "gray");
  Meter::legend ("Disabled");
  // And specify the total of 1.0, so the meter is grayed out.
  total_ = 1.0;
  fields_[0] = 1.0;
}


FieldMeter::~FieldMeter( void ){
  delete[] fields_;
  delete[] colors_;
  delete[] lastvals_;
  delete[] lastx_;
}

void FieldMeter::checkResources( void ){
  Meter::checkResources();
  usedcolor_ = parent_->allocColor( parent_->getResource( "usedLabelColor") );
}


void FieldMeter::SetUsedFormat ( const char * const fmt ) {
    /*  Do case-insensitive compares.  */
  if (!strncasecmp (fmt, "percent", 8))
    print_ = PERCENT;
  else if (!strncasecmp (fmt, "autoscale", 10))
    print_ = AUTOSCALE;
  else if (!strncasecmp (fmt, "float", 6))
    print_ = FLOAT;
  else {
    std::cerr << "Error:  could not parse format of '" << fmt << "'.\n"
              << "  I expected one of 'percent', 'autoscale', or 'float'"
              << " (Case-insensitive)." << std::endl;
    exit(1);
  }
}

void FieldMeter::setUsed (double val, double total)
{
  if (print_ == FLOAT)
    used_ = val;
  else if (print_ == PERCENT)
  {
    if (total != 0.0)
      used_ = val / total * 100.0;
    else
    {
      if (!printedZeroTotalMesg_) {
        printedZeroTotalMesg_ = 1;
        std::cerr << "Warning: " << name() << " meter had a zero total field! "
                  << "Would have caused a div-by-zero exception." << std::endl;
      }
      used_ = 0.0;
    }
  }
  else if (print_ == AUTOSCALE)
    used_ = val;
  else {
    std::cerr << "Error in " << name() << ":  I can't handle a UsedType enum "
              << "value of " << print_ << "!" << std::endl;
    exit(1);
  }
}

void FieldMeter::reset( void ){
  for ( int i = 0 ; i < numfields_ ; i++ )
    lastvals_[i] = lastx_[i] = -1;
}

void FieldMeter::setfieldcolor( int field, const char *color ){
  colors_[field] = parent_->allocColor( color );
}

void FieldMeter::setfieldcolor( int field, unsigned long color ) {
  colors_[field] = color;
}

void FieldMeter::draw( void ){
    /*  Draw the outline for the fieldmeter.  */
  parent_->setForeground( parent_->foreground() );
  parent_->drawRectangle( x_ - 1, y_ - 1, width_ + 2, height_ + 2 );
  if ( dolegends_ ){
    parent_->setForeground( textcolor_ );

    int offset;
    if ( dousedlegends_ )
      offset = parent_->textWidth( "XXXXXXXXXX" );
    else
      offset = parent_->textWidth( "XXXXXX" );

    parent_->drawString( x_ - offset + 1, y_ + height_, title_ );
  }

  drawlegend();
  drawfields( 1 );
}

void FieldMeter::drawlegend( void ){
  char *tmp1, *tmp2, buff[100];
  int n, x = x_;

  if (!docaptions_ || !dolegends_)
    return;

  parent_->clear( x_, y_ - 5 - parent_->textHeight(),
                  width_ + 5, parent_->textHeight() + 4 );

  tmp1 = tmp2 = legend_;
  for ( int i = 0 ; i < numfields_ ; i++ ){
    n = 0;
    while ( (*tmp2 != '/') && (*tmp2 != '\0') ){
      if ( (*tmp2 == '\\') && (*(tmp2 + 1) == '/') ) // allow '/' in field as '\/'
        memmove( tmp2, tmp2 + 1, strlen(tmp2) );
      tmp2++;
      n++;
    }
    tmp2++;
    strncpy( buff, tmp1, n );
    buff[n] = '\0';
    parent_->setStippleN(i%4);
    parent_->setForeground( colors_[i] );
    parent_->drawString( x, y_ - 5, buff );
    x += parent_->textWidth( buff, n );
    parent_->setForeground( parent_->foreground() );
    if ( i != numfields_ - 1 )
      parent_->drawString( x, y_ - 5, "/" );
    x += parent_->textWidth( "/", 1 );
    tmp1 = tmp2;
  }
  parent_->setStippleN(0);	/*  Restore default all-bits stipple.  */
}

void FieldMeter::drawused( int mandatory ){
  if ( !mandatory )
    if ( lastused_ == used_ )
      return;

  parent_->setStippleN(0);	/*  Use all-bits stipple.  */
  static const int onechar = parent_->textWidth( "X" );
  if (!usedoffset_)  // metric meters need extra space for '-' sign
    usedoffset_ = ( metric_ ? parent_->textWidth( "XXXXXX" )
                            : parent_->textWidth( "XXXXX" ) );
  char buf[10];

  if (print_ == PERCENT){
    snprintf( buf, 10, "%d%%", (int)used_ );
  }
  else if (print_ == AUTOSCALE){
    char scale[2];
    double scaled_used = scaleValue(used_, scale, metric_);
    /*  For now, we can only print 3 characters, plus the optional sign and
     *  suffix, without overprinting the legends.  Thus, we can
     *  print 965, or we can print 34, but we can't print 34.7 (the
     *  decimal point takes up one character).  bgrayson   */
    if (scaled_used == 0.0)
      snprintf (buf, 10, "0");
    else {
      if (scaled_used < 0 && !metric_)
        snprintf (buf, 10, "-");
      else if ( fabs(scaled_used) < 9.95 )
        //  9.95 or above would get
        //  rounded to 10.0, which is too wide.
        snprintf (buf, 10, "%.1f%s", scaled_used, scale);
      else
        snprintf (buf, 10, "%.0f%s", scaled_used, scale);
    }
  }
  else {
    if ( fabs(used_) < 99.95 )
      snprintf( buf, 10, "%.1f", used_ );
    else  // drop the decimal if the string gets too long
      snprintf( buf, 10, "%.0f", used_ );
  }

  parent_->clear( x_ - usedoffset_, y_ + height_ - parent_->textHeight(),
                  usedoffset_ - onechar / 2, parent_->textHeight() + 1 );
  parent_->setForeground( usedcolor_ );
  parent_->drawString( x_ - (strlen( buf ) + 1 ) * onechar + 2,
                       y_ + height_, buf );
  lastused_ = used_;
}

void FieldMeter::drawfields( int mandatory ){
  int twidth, x = x_;

  if ( total_ == 0 )
    return;

  for ( int i = 0 ; i < numfields_ ; i++ ){
    /*  Look for bogus values.  */
    if (fields_[i] < 0.0 && !metric_) {
      /*  Only print a warning 5 times per meter, followed by a
       *  message about no more warnings.  */
      numWarnings_ ++;
      if (numWarnings_ < 5)
        std::cerr << "Warning: meter " << name() << " had a negative value of "
                  << fields_[i] << " for field " << i << std::endl;
      if (numWarnings_ == 5)
        std::cerr << "Future warnings from the " << name() << " meter will not "
                  << "be displayed." << std::endl;
    }

    twidth = (int)fabs(((width_ * fields_[i]) / total_));
    if ( (i == numfields_ - 1) && ((x + twidth) != (x_ + width_)) )
      twidth = width_ + x_ - x;

    if ( mandatory || (twidth != lastvals_[i]) || (x != lastx_[i]) ){
      parent_->setForeground( colors_[i] );
      parent_->setStippleN(i%4);
      parent_->drawFilledRectangle( x, y_, twidth, height_ );
      parent_->setStippleN(0);	/*  Restore all-bits stipple.  */
      lastvals_[i] = twidth;
      lastx_[i] = x;
    }
    x += twidth;
  }
  if ( dousedlegends_ )
    drawused( mandatory );
}

void FieldMeter::checkevent( void ){
  drawfields(0);
}

void FieldMeter::setNumFields(int n){
  numfields_ = n;
  delete[] fields_;
  delete[] colors_;
  delete[] lastvals_;
  delete[] lastx_;
  fields_ = new double[numfields_];
  colors_ = new unsigned long[numfields_];
  lastvals_ = new int[numfields_];
  lastx_ = new int[numfields_];

  total_ = 0;
  for ( int i = 0 ; i < numfields_ ; i++ ){
    fields_[i] = 0.0;             /* egcs 2.91.66 bug !? don't do this and */
    lastvals_[i] = lastx_[i] = 0; /* that in a single statement or it'll   */
                                  /* overwrite too much with 0 ...         */
				  /* Thomas Waldmann ( tw@com-ma.de )      */
  }
}

bool FieldMeter::checkX(int x, int width) const {
  if ((x < x_) || (x + width < x_)
      || (x > x_ + width_) || (x + width > x_ + width_)){
    std::cerr << "FieldMeter::checkX() : bad horiz values for meter : "
         << name() << std::endl;

    std::cerr <<"value "<<x<<", width "<<width<<", total_ = "<<total_<<std::endl;

    for (int i = 0 ; i < numfields_ ; i++)
      std::cerr <<"fields_[" <<i <<"] = " <<fields_[i] <<",";
    std::cerr <<std::endl;

    return false;
  }

  return true;
}
