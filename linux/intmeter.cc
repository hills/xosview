//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "intmeter.h"
#include "xosview.h"
#include <fstream.h>
#include <stdlib.h>


static const char *INTFILE = "/proc/interrupts";


IntMeter::IntMeter( XOSView *parent,
		   const char *, const char *, int dolegends,
		   int dousedlegends )
: BitMeter( parent, "INTS", "IRQs (0 - 15)", 16, 
	    dolegends, dousedlegends ) {
  for ( int i = 0 ; i < 16 ; i++ )
    irqs_[i] = lastirqs_[i] = 0;

}

IntMeter::~IntMeter( void ){
}

void IntMeter::checkevent( void ){
  getirqs();

  for ( int i = 0 ; i < 16 ; i++ ){
    bits_[i] = ((irqs_[i] - lastirqs_[i]) != 0);
    lastirqs_[i] = irqs_[i];
  }

  BitMeter::checkevent();
}

void IntMeter::checkResources( void ){
  BitMeter::checkResources();
  onColor_  = parent_->allocColor( parent_->getResource( "intOnColor" ) );
  offColor_ = parent_->allocColor( parent_->getResource( "intOffColor" ) );
}

void IntMeter::getirqs( void ){
  ifstream intfile( INTFILE );
  int intno, count;
  char buf[256];

  if ( !intfile ){
    cerr <<"Can not open file : " <<INTFILE <<endl;
    exit( 1 );
 }

 while ( !intfile.eof() ){
    intfile >>intno >>buf;;
    if ( !intfile.eof() ){
      intfile >>count;
     intfile.getline( buf, 256 );

      irqs_[intno] = count;
    }
  }

//  ifstream intfile( INTFILE );
//  char buf[256];
//
//  for ( int i = 0 ; i < 4 ; i++ )
//    intfile.getline( buf, 256 );
//
//  intfile >>buf;
//  
//  for ( i = 0 ; i < 16 ; i++ )
//    intfile >>irqs_[i];

}
