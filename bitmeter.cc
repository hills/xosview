//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "bitmeter.h"


BitMeter::BitMeter( XOSView *parent,
		    const char *title, const char *legend, int numBits,
		    int docaptions, int, int dousedlegends)
  : Meter( parent, title, legend, docaptions, dousedlegends, dousedlegends ),
  bits_(NULL), lastbits_(NULL), disabled_(false)  {
  setNumBits(numBits);
}

BitMeter::~BitMeter( void ){
  delete [] bits_;
  delete [] lastbits_;
}

void BitMeter::setNumBits(int n){
  numbits_ = n;
  delete [] bits_;
  delete [] lastbits_;

  bits_ = new char[numbits_];
  lastbits_ = new char[numbits_];

  for ( int i = 0 ; i < numbits_ ; i++ )
      bits_[i] = lastbits_[i] = 0;
}

void BitMeter::disableMeter ( void ) {
  disabled_ = true;
  onColor_ = parent_->allocColor ("gray");
  offColor_ = onColor_;
  Meter::legend ("Disabled");

}

void BitMeter::checkResources( void ){
  Meter::checkResources();
}

void BitMeter::checkevent( void ){
  drawBits();
}

void BitMeter::drawBits( int mandatory ){
  static int pass = 1;

//  pass = (pass + 1) % 2;

  int x1 = x_ + 0, x2;

  for ( int i = 0 ; i < numbits_ ; i++ ){
    if ( i != (numbits_ - 1) )
      x2 = x_ + ((i + 1) * (width_+1)) / numbits_ - 1;
    else
      x2 = x_ + (width_+1) - 1;

    if ( (bits_[i] != lastbits_[i]) || mandatory ){
      if ( bits_[i] && pass )
	parent_->setForeground( onColor_ );
      else
	parent_->setForeground( offColor_ );

      parent_->drawFilledRectangle( x1, y_, x2 - x1, height_);
    }

    lastbits_[i] = bits_[i];

    x1 = x2 + 2;
  }
}

void BitMeter::draw( void ){
  parent_->lineWidth( 1 );
  parent_->setForeground( parent_->foreground() );
  parent_->drawFilledRectangle( x_ -1, y_ - 1, width_ + 2, height_ + 2 );

  parent_->lineWidth( 0 );

  if ( dolegends_ ){
    parent_->setForeground( textcolor_ );

    int offset;
    if ( dousedlegends_ )
      offset = parent_->textWidth( "XXXXXXXXXX" );
    else
      offset = parent_->textWidth( "XXXXXX" );

    parent_->drawString( x_ - offset + 1, y_ + height_, title_ );
    parent_->setForeground( onColor_ );
    if(docaptions_)
    {
      parent_->drawString( x_, y_ - 5, legend_ );
      }
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
