//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "meter.h"
#include <string.h>


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

double Meter::scaleValue( double value, char *scale, bool metric ){
  double scaled = ( value < 0 ? -value : value );

  if (scaled >= 999.5*1e15){
    scale[0] = 'E';
    scaled = value / ( metric ? 1e18 : 1ULL<<60 );
  }
  else if (scaled >= 999.5*1e12){
    scale[0] = 'P';
    scaled = value / ( metric ? 1e15 : 1ULL<<50 );
  }
  else if (scaled >= 999.5*1e9){
    scale[0] = 'T';
    scaled = value / ( metric ? 1e12 : 1ULL<<40 );
  }
  else if (scaled >= 999.5*1e6){
    scale[0] = 'G';
    scaled = value / ( metric ? 1e9 : 1UL<<30 );
  }
  else if (scaled >= 999.5*1e3){
    scale[0] = 'M';
    scaled = value / ( metric ? 1e6 : 1UL<<20 );
  }
  else if (scaled >= 999.5){
    scale[0] = ( metric ? 'k' : 'K' );
    scaled = value / ( metric ? 1e3 : 1UL<<10 );
  }
  else if (scaled < 0.9995 && metric){
    if (scaled >= 0.9995/1e3){
      scale[0] = 'm';
      scaled = value * 1e3;
    }
    else if (scaled >= 0.9995/1e6){
      scale[0] = '\265';
      scaled = value * 1e6;
    }
    else {
      scale[0] = 'n';
      scaled = value * 1e9;
    }
    // add more if needed
  }
  else {
    scale[0] = '\0';
    scaled = value;
  }
  scale[1] = '\0';
  return scaled;
}
