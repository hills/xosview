//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "swapmeter.h"
#include "xosview.h"
#include <sys/pstat.h>
#include <stdlib.h>

SwapMeter::SwapMeter( XOSView *parent )
: FieldMeterDecay( parent, 2, "SWAP", "USED/FREE" ){
}

SwapMeter::~SwapMeter( void ){
}

void SwapMeter::checkResources( void ){
  FieldMeterDecay::checkResources();

  setfieldcolor( 0, parent_->getResource( "swapUsedColor" ) );
  setfieldcolor( 1, parent_->getResource( "swapFreeColor" ) );
  priority_ = atoi (parent_->getResource( "swapPriority" ) );
  dodecay_ = !strcmp (parent_->getResource( "swapDecay" ), "True" );
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
  struct pst_dynamic stats;

  pstat_getdynamic( &stats, sizeof( pst_dynamic ), 1, 0 );

  total_ = stats.psd_vm;	

  fields_[0] = stats.psd_avm;
  fields_[1] = total_ - stats.psd_avm;

  used( (int)((100 * fields_[0]) / total_ ) );
}
