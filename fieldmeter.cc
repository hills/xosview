//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//
// $Id$
//
#include <fstream.h>
#include <stdio.h>
#include "general.h"
#include "fieldmeter.h"
#include "xosview.h"

CVSID("$Id$");
CVSID_DOT_H(FIELDMETER_H_CVSID);

FieldMeter::FieldMeter( XOSView *parent, int numfields, const char *title, 
                        const char *legend, int dolegends, int dousedlegends )
: Meter(parent, title, legend, dolegends, dousedlegends){
    /*  We need to set print_ to something valid -- the meters
     *  apparently get drawn before the meters have a chance to call
     *  CheckResources() themselves.  */
  numWarnings_ = printedZeroTotalMesg_ = 0;
  print_ = PERCENT;
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
  if (!strcasecmp (fmt, "percent"))
    print_ = PERCENT;
  else if (!strcasecmp (fmt, "autoscale"))
    print_ = AUTOSCALE;
  else if (!strcasecmp (fmt, "float"))
    print_ = FLOAT;
  else
  {
    fprintf (stderr, "Error:  could not parse format of '%s'\n", fmt);
    fprintf (stderr, "  I expected one of 'percent', 'bytes', or 'float'\n");
    fprintf (stderr, "  (Case-insensitive)\n");
    exit(1);
  }
}

void FieldMeter::setUsed (float val, float total)
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
    
    parent_->drawString( x_ - offset, y_ + height_, title_ );
    drawlegend();
  }

  drawfields( 1 );
}

void FieldMeter::drawlegend( void ){
  char *tmp1, *tmp2, buff[100];
  int n, x = x_;
  
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
    if ( (lastused_ == used_) )
      return;

  parent_->setStippleN(0);	/*  Use all-bits stipple.  */
  static const int onechar = parent_->textWidth( "X" );
  static int xoffset = parent_->textWidth( "XXXXX" );

  char buf[10];

  if (print_ == PERCENT){
    sprintf( buf, "%d", (int)used_ );
    strcat( buf, "%" );
  }
  else if (print_ == AUTOSCALE){
    char scale;
    float scaled_used;
      /*  Unfortunately, we have to do our comparisons by 1000s (otherwise
       *  a value of 1020, which is smaller than 1K, could end up
       *  being printed as 1020, which is wider than what can fit)  */
      /*  However, we do divide by 1024, so a K really is a K, and not
       *  1000.  */
      /*  In addition, we need to compare against 999.5*1000, because
       *  999.5, if not rounded up to 1.0 K, will be rounded by the
       *  %.0f to be 1000, which is too wide.  So anything at or above
       *  999.5 needs to be bumped up.  */
    if (used_ >= 999.5*1000*1000*1000*1000*1000*1000)
	{scale='E'; scaled_used = used_/1024/1024/1024/1024/1024/1024;}
    else if (used_ >= 999.5*1000*1000*1000*1000)
	{scale='P'; scaled_used = used_/1024/1024/1024/1024/1024;}
    else if (used_ >= 999.5*1000*1000*1000)
	{scale='T'; scaled_used = used_/1024/1024/1024/1024;}
    else if (used_ >= 999.5*1000*1000)
	{scale='G'; scaled_used = used_/1024/1024/1024;}
    else if (used_ >= 999.5*1000)
	{scale='M'; scaled_used = used_/1024/1024;}
    else if (used_ >= 999.5)
	{scale='K'; scaled_used = used_/1024;}
    else {scale=' '; scaled_used = used_;}
      /*  For now, we can only print 3 characters, plus the optional
       *  suffix, without overprinting the legends.  Thus, we can
       *  print 965, or we can print 34, but we can't print 34.7 (the
       *  decimal point takes up one character).  bgrayson   */
    if (scaled_used == 0.0)
      sprintf (buf, "0");
    else if (scaled_used < 9.95)  //  9.95 or above would get
				  //  rounded to 10.0, which is too wide.
      sprintf (buf, "%.1f%c", scaled_used, scale);
    /*  We don't need to check against 99.5 -- it all gets %.0f.  */
    /*else if (scaled_used < 99.5)*/
      /*sprintf (buf, "%.0f%c", scaled_used, scale);*/
    else 
      sprintf (buf, "%.0f%c", scaled_used, scale);
  }
  else {
    sprintf( buf, "%.1f", used_ );
  }

  parent_->clear( x_ - xoffset, y_ + height_ - parent_->textHeight(), 
		 xoffset - onechar / 2, parent_->textHeight() + 1 );
  parent_->setForeground( usedcolor_ );
  parent_->drawString( x_ - (strlen( buf ) + 1 ) * onechar, 
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

    twidth = (int) ((width_ * (float) fields_[i]) / total_); 
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

      if ( dousedlegends_ )
	drawused( manditory );
    }
    x += twidth;
  }

  //parent_->flush();
}

void FieldMeter::checkevent( void ){
  drawfields();
}

void FieldMeter::setNumFields(int n){
  numfields_ = n;
  delete[] fields_;
  delete[] colors_;
  delete[] lastvals_;
  delete[] lastx_;
  fields_ = new float[numfields_];
  colors_ = new unsigned long[numfields_];
  lastvals_ = new int[numfields_];
  lastx_ = new int[numfields_];

  total_ = 0;
  for ( int i = 0 ; i < numfields_ ; i++ )
    fields_[i] = lastvals_[i] = lastx_[i] = 0;
}

bool FieldMeter::checkX(int x, int width) const {
  if ((x < x_) || (x + width < x_)
      || (x > x_ + width_) || (x + width > x_ + width_)){
    cerr << "FieldMeter::checkX() : bad horiz values for meter : "
         << name() << endl;

    cerr <<"value "<<x<<", width "<<width<<", total_ = "<<total_<<endl;

    for (int i = 0 ; i < numfields_ ; i++)
      cerr <<"fields_[" <<i <<"] = " <<fields_[i] <<",";
    cerr <<endl;

    return false;
  }

  return true;
}
