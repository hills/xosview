//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//
// $Id$
//
#include "general.h"
#include "meter.h"
#include "xosview.h"

CVSID("$Id$");
CVSID_DOT_H(METER_H_CVSID);

Meter::Meter( XOSView *parent, const char *title, const char *legend, 
              int docaptions, int dolegends, int dousedlegends ) {
  title_ = legend_ = NULL;
  Meter::title( title );
  Meter::legend( legend );
  parent_ = parent;
  docaptions_ = docaptions;
  dolegends_ = dolegends;
  dousedlegends_ = dousedlegends;
  priority_ = 1;
  counter_ = 0;
  resize( parent->xoff(), parent->newypos(), parent->width() - 10, 10 );

}

Meter::~Meter( void ){
  if ( title_ )
        delete[] title_;
  if ( legend_ )
        delete[] legend_;
}

void Meter::checkResources( void ){
  textcolor_ = parent_->allocColor( parent_->getResource( "meterLabelColor") );
}

void Meter::title( const char *title ){

  if ( title_ )
  	delete[] title_;

  int len = strlen(title);
  title_ = new char[len + 1];
  strncpy( title_, title, len );
  title_[len] = '\0'; // strncpy() will not null terminate if s2 > len
}

void Meter::legend( const char *legend ){

  if ( legend_ )
	  delete[] legend_;

  int len = strlen(legend);
  legend_ = new char[len + 1];
  strncpy( legend_, legend, len );
  legend_[len] = '\0'; // strncpy() will not null terminate if s2 > len
}

void Meter::resize( int x, int y, int width, int height ){
  x_ = x;
  y_ = y;
  width_ = (width>=0) ? width : 0;    // fix for cosmetical bug:
  height_ = (height>=0) ? height : 0; // beware of values < 0 !
  width_ &= ~1;                       // only allow even width_ values
}


