//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "bitmeter.h"
#include "xosview.h"


BitMeter::BitMeter( XOSView *parent,
		    const char *title, const char *legend, int numBits,
		    int, int dousedlegends)
: Meter( parent, title, legend, dousedlegends, dousedlegends ) {
  numbits_ = numBits;

  bits_ = new char[numbits_];
  lastbits_ = new char[numbits_];

  for ( int i = 0 ; i < numbits_ ; i++ )
    bits_[i] = lastbits_[i] = 0;
}

BitMeter::~BitMeter( void ){
  delete bits_;
  delete lastbits_;
}

void BitMeter::checkResources( void ){
  Meter::checkResources();
}

void BitMeter::checkevent( void ){
  drawBits();
}

void BitMeter::drawBits( int manditory ){
  static int pass = 1;

//  pass = (pass + 1) % 2;

  int x1 = x_ + 0, x2;

  for ( int i = 0 ; i < numbits_ ; i++ ){
    if ( i != (numbits_ - 1) )
      x2 = x_ + ((i + 1) * width_) / numbits_ - 1;
    else
      x2 = x_ + width_ - 1;
    
    if ( (bits_[i] != lastbits_[i]) || manditory ){
      if ( bits_[i] && pass )
	parent_->setForeground( onColor_ );
      else
	parent_->setForeground( offColor_ );

      parent_->drawFilledRectangle( x1, y_, x2 - x1, height_ - 1 );
    }
    
    lastbits_[i] = bits_[i];

    x1 = x2 + 2;
  }
}

void BitMeter::draw( void ){
  parent_->lineWidth( 2 );
  parent_->setForeground( parent_->foreground() );
  parent_->drawRectangle( x_ - 1, y_ - 1, width_ + 1, height_ + 1 );

  for ( int i = 1 ; i < numbits_ ; i++ ){
    int x = x_ + (i * width_) / numbits_;
    parent_->drawLine( x, y_, x, y_ + height_ );
  }
  parent_->lineWidth( 0 );

  if ( dolegends_ ){
    parent_->setForeground( textcolor_ );
    
    int offset;
    if ( dousedlegends_ )
      offset = parent_->textWidth( "XXXXXXXXX" );
    else
      offset = parent_->textWidth( "XXXXX" );
    
    parent_->drawString( x_ - offset, y_ + height_, title_ );
    parent_->drawString( x_, y_ - 5, legend_ );
  }

  drawBits( 1 );
}

void BitMeter::setBits(int startbit, unsigned char values){
  unsigned char mask = 1;
  for (int i = startbit ; i < startbit + 8 ; i++){
    bits_[i] = values & mask;
    mask = mask << 1;
  }
}
