//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "swapmeter.h"
#include <stdlib.h>
#include <sys/sysinfo.h>


SwapMeter::SwapMeter( XOSView *parent )
: FieldMeterGraph( parent, 2, "SWAP", "USED/FREE" ){

}

SwapMeter::~SwapMeter( void ){
}

void SwapMeter::checkResources( void ){
  FieldMeterGraph::checkResources();

  setfieldcolor( 0, parent_->getResource( "swapUsedColor" ) );
  setfieldcolor( 1, parent_->getResource( "swapFreeColor" ) );
  priority_ = atoi (parent_->getResource( "swapPriority" ) );
  dodecay_ = parent_->isResourceTrue( "swapDecay" );
  useGraph_ = parent_->isResourceTrue( "swapGraph" );
  SetUsedFormat (parent_->getResource("swapUsedFormat"));
}

void SwapMeter::checkevent( void ){
  getswapinfo();
  drawfields();
}

void SwapMeter::getswapinfo( void ){
  struct sysinfo sinfo;
  typeof (sinfo.mem_unit) unit;

  sysinfo(&sinfo);
  unit = (sinfo.mem_unit ? sinfo.mem_unit : 1);
  total_ = (double)sinfo.totalswap * unit;
  fields_[0] = (double)(sinfo.totalswap - sinfo.freeswap) * unit;

  if ( total_ == 0 ){
    total_ = 1;
    fields_[0] = 0;
  }

  if (total_)
    setUsed (fields_[0], total_);
}
