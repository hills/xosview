//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//
// $Id$
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

  if ( !intfile ){
    cerr <<"Can not open file : " <<INTFILE <<endl;
    exit( 1 );
  }

  while ( !intfile.eof() ){
    intfile >>intno;
    if (!intfile) break;
    intfile.ignore(1);
    if ( !intfile.eof() ){
      intfile >>count;
      intfile.istream::ignore(1024, '\n');
      
      irqs_[intno] = count;
    }
  }

#if 0
  while ( !intfile.eof() ){
    intfile >>intno;
    intfile.ignore(1);
    if ( !intfile.eof() ){
      intfile >>count;
      intfile.istream::ignore(1024, '\n');
      
      irqs_[intno] = count;
    }
  }
#endif
}
