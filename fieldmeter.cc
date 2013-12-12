//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "fieldmeter.h"
#include "xosview.h"

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
  else
  {
    fprintf (stderr, "Error:  could not parse format of '%s'\n", fmt);
    fprintf (stderr, "  I expected one of 'percent', 'bytes', or 'float'\n");
    fprintf (stderr, "  (Case-insensitive)\n");
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
	fprintf(stderr, "Warning:  %s meter had a zero total "
		"field!  Would have caused a div-by-zero "
		"exception.\n", name());
      }
      used_ = 0.0;
    }
  }
  else if (print_ == AUTOSCALE)
    used_ = val;
  else {
    fprintf (stderr, "Error in %s:  I can't handle a "
		     "UsedType enum value of %d!\n", name(), print_);
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
      offset = parent_->textWidth( "XXXXXXXXX" );
    else
      offset = parent_->textWidth( "XXXXX" );

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

void FieldMeter::drawused( int manditory ){
  if ( !manditory )
    if ( lastused_ == used_ )
      return;

  parent_->setStippleN(0);	/*  Use all-bits stipple.  */
  static const int onechar = parent_->textWidth( "X" );
  static int xoffset = parent_->textWidth( "XXXXX" );

  char buf[10];

  if (print_ == PERCENT){
    snprintf( buf, 10, "%d%%", (int)used_ );
  }
  else if (print_ == AUTOSCALE){
    char scale;
    double scaled_used;
      /*  Unfortunately, we have to do our comparisons by 1000s (otherwise
       *  a value of 1020, which is smaller than 1K, could end up
       *  being printed as 1020, which is wider than what can fit)  */
      /*  However, we do divide by 1024, unless the meter is defined as metric,
       *  so a K really is a K.  */
      /*  In addition, we need to compare against 999.5*1000, because
       *  999.5, if not rounded up to 1.0 K, will be rounded by the
       *  %.0f to be 1000, which is too wide.  So anything at or above
       *  999.5 needs to be bumped up.  */
    if (used_ >= 999.5*1e15){
      scale='E';
      if (metric_)
        scaled_used = used_/1e18;
      else
        scaled_used = used_/(1ULL<<60);
    }
    else if (used_ >= 999.5*1e12){
      scale='P';
      if (metric_)
        scaled_used = used_/1e15;
      else
        scaled_used = used_/(1ULL<<50);
    }
    else if (used_ >= 999.5*1e9){
      scale='T';
      if (metric_)
        scaled_used = used_/1e12;
      else
        scaled_used = used_/(1ULL<<40);
    }
    else if (used_ >= 999.5*1e6){
      scale='G';
      if (metric_)
        scaled_used = used_/1e9;
      else
        scaled_used = used_/(1UL<<30);
    }
    else if (used_ >= 999.5*1e3){
      scale='M';
      if (metric_)
        scaled_used = used_/1e6;
      else
        scaled_used = used_/(1UL<<20);
    }
    else if (used_ >= 999.5){
      if (metric_) {
        scale='k';
        scaled_used = used_/1e3;
      }
      else {
        scale='K';
        scaled_used = used_/(1UL<<10);
      }
    }
    else if (used_ < 0.9995 && metric_) {
      if (used_ >= 0.9995/1e3) {
        scale='m';
        scaled_used = used_*1e3;
      }
      else if (used_ >= 0.9995/1e6) {
        scale='\265';
        scaled_used = used_*1e6;
      }
      else {
        scale='n';
        scaled_used = used_*1e9;
      }
      // add more if needed
    }
    else {
      scale='\0';
      scaled_used = used_;
    }
      /*  For now, we can only print 3 characters, plus the optional
       *  suffix, without overprinting the legends.  Thus, we can
       *  print 965, or we can print 34, but we can't print 34.7 (the
       *  decimal point takes up one character).  bgrayson   */
      /*  Also check for negative values, and just print "-" for
       *  them.  */
    if (scaled_used < 0)
      snprintf (buf, 10, "-");
    else if (scaled_used == 0.0)
      snprintf (buf, 10, "0");
    else if (scaled_used < 9.95) {
      //  9.95 or above would get
      //  rounded to 10.0, which is too wide.
      if (scale)
	snprintf (buf, 10, "%.1f%c", scaled_used, scale);
      else
	snprintf (buf, 10, "%.1f", scaled_used);
    } else {
      if (scale)
        snprintf (buf, 10, "%.0f%c", scaled_used, scale);
      else
        snprintf (buf, 10, "%.0f", scaled_used);
    }
  }
  else {
    snprintf( buf, 10, "%.1f", used_ );
  }

  parent_->clear( x_ - xoffset, y_ + height_ - parent_->textHeight(),
		 xoffset - onechar / 2, parent_->textHeight() + 1 );
  parent_->setForeground( usedcolor_ );
  parent_->drawString( x_ - (strlen( buf ) + 1 ) * onechar + 2,
		      y_ + height_, buf );

  lastused_ = used_;
}

void FieldMeter::drawfields( int manditory ){
  int twidth, x = x_;

  if ( total_ == 0 )
    return;

  for ( int i = 0 ; i < numfields_ ; i++ ){
    /*  Look for bogus values.  */
    if (fields_[i] < 0.0) {
      /*  Only print a warning 5 times per meter, followed by a
       *  message about no more warnings.  */
      numWarnings_ ++;
      if (numWarnings_ < 5)
	fprintf(stderr, "Warning:  meter %s had a negative "
	  "value of %f for field %d\n", name(), fields_[i], i);
      if (numWarnings_ == 5)
        fprintf(stderr, "Future warnings from the %s meter "
	  "will not be displayed.\n", name());
    }

    twidth = (int) ((width_ * (double) fields_[i]) / total_);
//    twidth = (int)((fields_[i] * width_) / total_);
    if ( (i == numfields_ - 1) && ((x + twidth) != (x_ + width_)) )
      twidth = width_ + x_ - x;

    if ( manditory || (twidth != lastvals_[i]) || (x != lastx_[i]) ){
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
    drawused( manditory );
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
