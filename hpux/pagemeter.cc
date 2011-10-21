//  
//  Copyright (c) 1997 by Mike Romberg (romberg@fsl.noaa.gov)
//
//  This file may be distributed under terms of the GPL
//

#include "pagemeter.h"
#include "xosview.h"
#include <stdlib.h>
#include <sys/pstat.h>

PageMeter::PageMeter( XOSView *parent, float max )
  : FieldMeterDecay( parent, 3, "PAGE", "IN/OUT/IDLE" ){
  for ( int i = 0 ; i < 2 ; i++ )
    for ( int j = 0 ; j < 2 ; j++ )
      pageinfo_[j][i] = 0;

  maxspeed_ = max;
  pageindex_ = 0;
}

PageMeter::~PageMeter( void ){
}

void PageMeter::checkResources( void ){
  FieldMeterDecay::checkResources();

  setfieldcolor( 0, parent_->getResource( "pageInColor" ) );
  setfieldcolor( 1, parent_->getResource( "pageOutColor" ) );
  setfieldcolor( 2, parent_->getResource( "pageIdleColor" ) );
  priority_ = atoi (parent_->getResource( "pagePriority" ) );
  maxspeed_ *= priority_ / 10.0;
  dodecay_ = parent_->isResourceTrue( "pageDecay" );
  SetUsedFormat( parent_->getResource( "pageUsedFormat" ) );
}

void PageMeter::checkevent( void ){
  getpageinfo();
  drawfields();
}

void PageMeter::getpageinfo( void ){

  struct pst_vminfo vminfo;
  pstat_getvminfo(&vminfo, sizeof(vminfo), 1, 0);

  total_ = 0;

  pageinfo_[pageindex_][0] = vminfo.psv_spgin;
  pageinfo_[pageindex_][1] = vminfo.psv_spgout;

  int oldindex = (pageindex_+1)%2;
  
  for ( int i = 0; i < 2; i++ ) {
    if ( pageinfo_[oldindex][i] == 0 )
      pageinfo_[oldindex][i] = pageinfo_[pageindex_][i];

    fields_[i] = pageinfo_[pageindex_][i] - pageinfo_[oldindex][i];
    total_ += fields_[i];
  }

  if ( total_ > maxspeed_ )
    fields_[2] = 0.0;
  else {
    fields_[2] = maxspeed_ - total_;
    total_ = maxspeed_;
  }

  setUsed (total_ - fields_[2], maxspeed_ );
  pageindex_ = (pageindex_ + 1) % 2;
}
