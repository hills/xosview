//
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//  2007 by Samuel Thibault ( samuel.thibault@ens-lyon.org )
//
//  This file may be distributed under terms of the GPL
//

#include "swapmeter.h"
#include "xosview.h"
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <error.h>

extern "C" {
#include <mach.h>
#include <mach/mach_traps.h>
#include <mach/default_pager.h>
#include "get_def_pager.h"
}

SwapMeter::SwapMeter( XOSView *parent )
: FieldMeterGraph( parent, 2, "SWAP", "ACTIVE/USED/FREE" ){
  def_pager = MACH_PORT_NULL;
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
  kern_return_t err;

  if (def_pager == MACH_PORT_NULL)
    def_pager = get_def_pager();

  if (!MACH_PORT_VALID (def_pager)) {
    def_pager = MACH_PORT_DEAD;
    parent_->done(1);
    return;
  }

  err = default_pager_info (def_pager, &def_pager_info);
  if (err) {
    error (0, err, "default_pager_info");
    parent_->done(1);
    return;
  }

  total_ = def_pager_info.dpi_total_space;
  fields_[1] = def_pager_info.dpi_free_space;
  fields_[0] = total_ - fields_[1];

  if (total_)
    setUsed (fields_[0], total_);
}
