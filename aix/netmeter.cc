//
//  Copyright (c) 2026 by Antoni Sawicki ( as@tenoware.com )
//
//  This file may be distributed under terms of the GPL
//

#include "netmeter.h"
#include "aixstats.h"
#include <stdlib.h>

NetMeter::NetMeter( XOSView *parent, float max )
  : FieldMeterGraph( parent, 3, "NET", "IN/OUT/IDLE" ){
  double in, out;

  maxpackets_ = max;
  lastBytesIn_ = lastBytesOut_ = 0;
  first_ = true;
  ignored_ = false;
  ok_ = AIXStats::net( 0, false, in, out );

  if ( !ok_ )
    disableMeter();
}

NetMeter::~NetMeter( void ){
}

void NetMeter::checkResources( void ){
  FieldMeterGraph::checkResources();

  priority_ = atoi( parent_->getResource( "netPriority" ) );

  //  disableMeter() collapsed this meter to a single field, so setting the
  //  per field colours below would run off the end of the array.
  if ( !ok_ )
    return;

  setfieldcolor( 0, parent_->getResource( "netInColor" ) );
  setfieldcolor( 1, parent_->getResource( "netOutColor" ) );
  setfieldcolor( 2, parent_->getResource( "netBackground" ) );
  dodecay_ = parent_->isResourceTrue( "netDecay" );
  useGraph_ = parent_->isResourceTrue( "netGraph" );
  SetUsedFormat( parent_->getResource( "netUsedFormat" ) );

  netIface_ = parent_->getResource( "netIface" );
  if ( netIface_[0] == '-' ){
    ignored_ = true;
    netIface_.erase( 0, netIface_.find_first_not_of( "- " ) );
  }
}

void NetMeter::checkevent( void ){
  getnetstats();
  drawfields();
}

void NetMeter::getnetstats( void ){
  double nowBytesIn, nowBytesOut;
  bool filtering = netIface_ != "False";

  total_ = maxpackets_;

  if ( !AIXStats::net( filtering ? netIface_.c_str() : 0, ignored_,
                       nowBytesIn, nowBytesOut ) )
    return;

  IntervalTimerStop();

  if ( first_ ){
    lastBytesIn_ = nowBytesIn;
    lastBytesOut_ = nowBytesOut;
    first_ = false;
  }

  //  On AIX 4.x these are 32 bit kernel counters and they wrap on a busy
  //  link, which shows up as the total going backwards.  Skip that sample
  //  rather than plotting an enormous spike.
  double t = IntervalTimeInSecs();
  fields_[0] = nowBytesIn >= lastBytesIn_ ?
    ( nowBytesIn - lastBytesIn_ ) / t : 0.0;
  fields_[1] = nowBytesOut >= lastBytesOut_ ?
    ( nowBytesOut - lastBytesOut_ ) / t : 0.0;

  IntervalTimerStart();
  lastBytesIn_ = nowBytesIn;
  lastBytesOut_ = nowBytesOut;

  if ( total_ < fields_[0] + fields_[1] )
    total_ = fields_[0] + fields_[1];
  fields_[2] = total_ - fields_[0] - fields_[1];

  setUsed( fields_[0] + fields_[1], total_ );
}
