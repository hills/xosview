//
//  Copyright (c) 2026 by Antoni Sawicki ( as@tenoware.com )
//
//  This file may be distributed under terms of the GPL
//

#include "diskmeter.h"
#include "aixstats.h"
#include <stdlib.h>

//  These are the same counters iostat(1) reports, so they only move when the
//  kernel is maintaining disk history: see chdev -l sys0 -a iostat=true.

DiskMeter::DiskMeter( XOSView *parent, float max )
  : FieldMeterGraph( parent, 3, "DISK", "READ/WRITE/IDLE" ){
  double read, written;

  maxspeed_ = max;
  readPrev_ = writePrev_ = 0;
  first_ = true;
  ok_ = AIXStats::disk( read, written );

  if ( !ok_ )
    disableMeter();
}

DiskMeter::~DiskMeter( void ){
}

void DiskMeter::checkResources( void ){
  FieldMeterGraph::checkResources();

  priority_ = atoi( parent_->getResource( "diskPriority" ) );

  //  disableMeter() collapsed this meter to a single field, so setting the
  //  per field colours below would run off the end of the array.
  if ( !ok_ )
    return;

  setfieldcolor( 0, parent_->getResource( "diskReadColor" ) );
  setfieldcolor( 1, parent_->getResource( "diskWriteColor" ) );
  setfieldcolor( 2, parent_->getResource( "diskIdleColor" ) );
  dodecay_ = parent_->isResourceTrue( "diskDecay" );
  useGraph_ = parent_->isResourceTrue( "diskGraph" );
  SetUsedFormat( parent_->getResource( "diskUsedFormat" ) );
}

void DiskMeter::checkevent( void ){
  getdiskinfo();
  drawfields();
}

void DiskMeter::getdiskinfo( void ){
  double readNow, writeNow;

  total_ = maxspeed_;

  if ( !AIXStats::disk( readNow, writeNow ) )
    return;

  IntervalTimerStop();

  if ( first_ ){
    readPrev_ = readNow;
    writePrev_ = writeNow;
    first_ = false;
  }

  //  On AIX 4.x these are 32 bit kernel counters and they wrap on a busy
  //  disk, which shows up as the total moving backwards.  Skip that sample
  //  rather than plotting an enormous spike.
  double t = IntervalTimeInSecs();
  fields_[0] = readNow >= readPrev_ ? ( readNow - readPrev_ ) / t : 0.0;
  fields_[1] = writeNow >= writePrev_ ? ( writeNow - writePrev_ ) / t : 0.0;

  IntervalTimerStart();
  readPrev_ = readNow;
  writePrev_ = writeNow;

  if ( fields_[0] + fields_[1] > total_ )
    total_ = fields_[0] + fields_[1];
  fields_[2] = total_ - fields_[0] - fields_[1];

  setUsed( fields_[0] + fields_[1], total_ );
}
