//
//  Copyright (c) 2026 by Antoni Sawicki ( as@tenoware.com )
//
//  This file may be distributed under terms of the GPL
//

#include "memmeter.h"
#include "aixstats.h"
#include <stdlib.h>

MemMeter::MemMeter( XOSView *parent )
  : FieldMeterDecay( parent, 3, "MEM", "USED/CACHE/FREE" ){
  double total, cache, free;

  ok_ = AIXStats::memory( total, cache, free );

  if ( !ok_ )
    disableMeter();
}

MemMeter::~MemMeter( void ){
}

void MemMeter::checkResources( void ){
  FieldMeterDecay::checkResources();

  priority_ = atoi( parent_->getResource( "memPriority" ) );

  //  disableMeter() collapsed this meter to a single field, so setting the
  //  per field colours below would run off the end of the array.
  if ( !ok_ )
    return;

  setfieldcolor( 0, parent_->getResource( "memUsedColor" ) );
  setfieldcolor( 1, parent_->getResource( "memCacheColor" ) );
  setfieldcolor( 2, parent_->getResource( "memFreeColor" ) );
  dodecay_ = parent_->isResourceTrue( "memDecay" );
  SetUsedFormat( parent_->getResource( "memUsedFormat" ) );
}

void MemMeter::checkevent( void ){
  getmeminfo();
  drawfields();
}

void MemMeter::getmeminfo( void ){
  //  The cache is made up of file pages, which the kernel keeps resident but
  //  will release under pressure, so show them apart from used memory.
  double cache, free;

  if ( !AIXStats::memory( total_, cache, free ) )
    return;

  fields_[0] = total_ - cache - free;
  fields_[1] = cache;
  fields_[2] = free;

  if ( fields_[0] < 0 )
    fields_[0] = 0;

  setUsed( fields_[0], total_ );
}
