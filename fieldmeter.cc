//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "fieldmeter.h"
#include "xosview.h"
#include <fstream.h>
#include <stdio.h>

FieldMeter::FieldMeter( XOSView *parent, int numfields, const char *title, 
                        const char *legend, int dolegends, int dousedlegends )
: Meter(parent, title, legend, dolegends, dousedlegends){
  print_ = PERCENT;
  used_ = 0;
  lastused_ = -1;
  numfields_ = numfields;
  fields_ = new float[numfields_];
  colors_ = new unsigned long[numfields_];
  lastvals_ = new int[numfields_];
  lastx_ = new int[numfields_];
  total_ = 0;

  for ( int i = 0 ; i < numfields_ ; i++ )
    fields_[i] = lastvals_[i] = lastx_[i] = 0;
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
  parent_->setForeground( parent_->foreground() );
  parent_->drawRectangle( x_ - 1, y_ - 1, width_ + 1, height_ + 1 );
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
    parent_->setForeground( colors_[i] );
    parent_->drawString( x, y_ - 5, buff );
    x += parent_->textWidth( buff, n );
    parent_->setForeground( parent_->foreground() );
    if ( i != numfields_ - 1 )
      parent_->drawString( x, y_ - 5, "/" );
    x += parent_->textWidth( "/", 1 );
    tmp1 = tmp2;
  }
}

void FieldMeter::drawused( int manditory ){
  if ( !manditory )
    if ( (lastused_ == used_) )
      return;

  static const int onechar = parent_->textWidth( "X" );
  static int xoffset = parent_->textWidth( "XXXXX" );

  char buf[10];

  if (print_ == PERCENT){
    sprintf( buf, "%d", (int)used_ );
    strcat( buf, "%" );
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
    twidth = (int) ((width_ * (float) fields_[i]) / total_); 
//    twidth = (int)((fields_[i] * width_) / total_);
    if ( (i == numfields_ - 1) && ((x + twidth) != (x_ + width_)) )
      twidth = width_ + x_ - x;

    if ( manditory || (twidth != lastvals_[i]) || (x != lastx_[i]) ){
      parent_->setForeground( colors_[i] );
      parent_->drawFilledRectangle( x, y_, twidth, height_ );
      lastvals_[i] = twidth;
      lastx_[i] = x;

      if ( dousedlegends_ )
	drawused( manditory );
    }
    x += twidth;
  }

  parent_->flush();
}

void FieldMeter::checkevent( void ){
  drawfields();
}
