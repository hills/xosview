//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "meter.h"
#include "xosview.h"

Meter::Meter( XOSView *parent, const char *title, const char *legend, 
              int dolegends, int dousedlegends ) {
  title_ = legend_ = NULL;
  Meter::title( title );
  Meter::legend( legend );
  parent_ = parent;
  dolegends_ = dolegends;
  dousedlegends_ = dousedlegends;
  priority_ = 1;
  counter_ = 0;
  resize( parent->xoff(), parent->newypos(), parent->width() - 10, 10 );

}

Meter::~Meter( void ){
  delete[] title_;
  delete[] legend_;
}

void Meter::checkResources( void ){
  textcolor_ = parent_->allocColor( parent_->getResource( "meterLabelColor") );
}

void Meter::title( const char *title ){
  delete[] title_;
  title_ = new char[strlen( title ) + 1];
  strcpy( title_, title );
}

void Meter::legend( const char *legend ){
  delete[] legend_;
  legend_ = new char[strlen( legend ) + 1];
  strcpy( legend_, legend );
}

void Meter::resize( int x, int y, int width, int height ){
  x_ = x;
  y_ = y;
  width_ = width;
  height_ = height;
}


