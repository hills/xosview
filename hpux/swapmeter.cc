//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "swapmeter.h"
#include "xosview.h"
#include <sys/pstat.h>
#include <stdlib.h>

static int MAX_SWAP_AREAS = 16;

SwapMeter::SwapMeter( XOSView *parent )
: FieldMeterDecay( parent, 2, "SWAP", "USED/FREE" ){
}

SwapMeter::~SwapMeter( void ){
}

void SwapMeter::checkResources( void ){
  FieldMeterDecay::checkResources();

  setfieldcolor( 0, parent_->getResource( "swapUsedColor" ) );
  setfieldcolor( 1, parent_->getResource( "swapFreeColor" ) );
  priority_ = atoi(parent_->getResource( "swapPriority" ) );
  dodecay_ = parent_->isResourceTrue( "swapDecay" );
  SetUsedFormat( parent_->getResource( "swapUsedFormat" ) );
}

void SwapMeter::checkevent( void ){
  static int pass = 0;

  pass = (pass + 1)%5;
  if ( pass != 0 )
    return;
  
  getswapinfo();
  drawfields();
}

void SwapMeter::getswapinfo( void ){
  struct pst_swapinfo swapinfo;

  total_ = 0;
  fields_[1] = 0;

  for (int i = 0 ; i < MAX_SWAP_AREAS ; i++)
      {
      pstat_getswap(&swapinfo, sizeof(swapinfo), 1, i);
      if (swapinfo.pss_idx == (unsigned)i)
          {
          total_ += (swapinfo.pss_nblksenabled * 1024);
          fields_[1] += (swapinfo.pss_nfpgs * 4 * 1024);
          }
      }

  fields_[0] = total_ - fields_[1];
  setUsed( fields_[0], total_ );
}
