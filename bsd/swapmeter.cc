//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  NetBSD port:  
//  Copyright (c) 1995 Brian Grayson(bgrayson@pine.ece.utexas.edu)
//
//  This file may be distributed under terms of the GPL
//

#include "swapmeter.h"
#include "xosview.h"

#include "swapinternal.h"
#include <stdlib.h>		//  For atoi().  BCG

SwapMeter::SwapMeter( XOSView *parent )
: FieldMeterDecay( parent, 2, "SWAP", "USED/FREE" ){
  NetBSDInitSwapInfo();
}

SwapMeter::~SwapMeter( void ){
}

void SwapMeter::checkResources( void ){
  FieldMeter::checkResources();

  setfieldcolor( 0, parent_->getResource("swapUsedColor") );
  setfieldcolor( 1, parent_->getResource("swapFreeColor") );

  priority_ = atoi (parent_->getResource("swapPriority"));
  dodecay_ = !strcmp (parent_->getResource("swapDecay"),"True");
}

void SwapMeter::checkevent( void ){
  getswapinfo();
  drawfields();
}


void SwapMeter::getswapinfo( void ){
  int total_int, free_int;

  NetBSDGetSwapInfo (&total_int, &free_int);
  total_ = total_int;
  fields_[1] = free_int;
  fields_[0] = total_ - fields_[1];

  if ( total_ == 0 ){
    total_ = 1;
    fields_[0] = 0;
    fields_[1] = 1;
  }

  used( (int)((100 * fields_[0]) / total_ ) );
}
