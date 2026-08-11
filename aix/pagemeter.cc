//
//  Copyright (c) 2026 by Antoni Sawicki ( as@tenoware.com )
//
//  This file may be distributed under terms of the GPL
//

#include "pagemeter.h"
#include "aixstats.h"
#include <stdlib.h>

//  The counters used here cover traffic to and from paging space only, which
//  is what vmstat reports in its pi and po columns.  Ordinary file paging is
//  deliberately left out because the disk meter already covers it.

PageMeter::PageMeter( XOSView *parent, float max )
  : FieldMeterGraph( parent, 3, "PAGE", "IN/OUT/IDLE" ){
  double in, out;

  for ( int i = 0 ; i < 2 ; i++ )
    for ( int j = 0 ; j < 2 ; j++ )
      pageinfo_[j][i] = 0;

  maxspeed_ = max;
  pageindex_ = 0;
  ok_ = AIXStats::paging( in, out );

  if ( !ok_ )
    disableMeter();
}

PageMeter::~PageMeter( void ){
}

void PageMeter::checkResources( void ){
  FieldMeterGraph::checkResources();

  priority_ = atoi( parent_->getResource( "pagePriority" ) );
  maxspeed_ *= priority_ / 10.0;

  //  disableMeter() collapsed this meter to a single field, so setting the
  //  per field colours below would run off the end of the array.
  if ( !ok_ )
    return;

  setfieldcolor( 0, parent_->getResource( "pageInColor" ) );
  setfieldcolor( 1, parent_->getResource( "pageOutColor" ) );
  setfieldcolor( 2, parent_->getResource( "pageIdleColor" ) );
  dodecay_ = parent_->isResourceTrue( "pageDecay" );
  useGraph_ = parent_->isResourceTrue( "pageGraph" );
  SetUsedFormat( parent_->getResource( "pageUsedFormat" ) );
}

void PageMeter::checkevent( void ){
  getpageinfo();
  drawfields();
}

void PageMeter::getpageinfo( void ){
  if ( !AIXStats::paging( pageinfo_[pageindex_][0], pageinfo_[pageindex_][1] ) )
    return;

  total_ = 0;

  int oldindex = ( pageindex_ + 1 ) % 2;
  for ( int i = 0 ; i < 2 ; i++ ){
    if ( pageinfo_[oldindex][i] == 0 )
      pageinfo_[oldindex][i] = pageinfo_[pageindex_][i];

    fields_[i] = pageinfo_[pageindex_][i] - pageinfo_[oldindex][i];
    if ( fields_[i] < 0 )
      fields_[i] = 0;
    total_ += fields_[i];
  }

  if ( total_ > maxspeed_ )
    fields_[2] = 0.0;
  else {
    fields_[2] = maxspeed_ - total_;
    total_ = maxspeed_;
  }

  setUsed( total_ - fields_[2], maxspeed_ );
  pageindex_ = oldindex;
}
