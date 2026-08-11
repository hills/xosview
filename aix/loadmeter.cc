//
//  Copyright (c) 2026 by Antoni Sawicki ( as@tenoware.com )
//
//  This file may be distributed under terms of the GPL
//

#include "loadmeter.h"
#include "cpumeter.h"
#include "aixstats.h"
#include <stdlib.h>
#include <string.h>
#include <iostream>

LoadMeter::LoadMeter( XOSView *parent )
  : FieldMeterGraph( parent, 2, "LOAD", "PROCS/MIN", 1, 1, 0 ){
  double avg[3];

  lastalarmstate = -1;
  total_ = 2.0;
  ok_ = AIXStats::load( avg );

  if ( !ok_ )
    disableMeter();
}

LoadMeter::~LoadMeter( void ){
}

void LoadMeter::checkResources( void ){
  FieldMeterGraph::checkResources();

  priority_ = atoi( parent_->getResource( "loadPriority" ) );

  //  disableMeter() collapsed this meter to a single field, so setting the
  //  per field colours below would run off the end of the array.
  if ( !ok_ )
    return;

  procloadcol_ = parent_->allocColor( parent_->getResource( "loadProcColor" ) );
  warnloadcol_ = parent_->allocColor( parent_->getResource( "loadWarnColor" ) );
  critloadcol_ = parent_->allocColor( parent_->getResource( "loadCritColor" ) );

  setfieldcolor( 0, procloadcol_ );
  setfieldcolor( 1, parent_->getResource( "loadIdleColor" ) );
  useGraph_ = parent_->isResourceTrue( "loadGraph" );
  dodecay_ = parent_->isResourceTrue( "loadDecay" );
  SetUsedFormat( parent_->getResource( "loadUsedFormat" ) );

  const char *warn = parent_->getResource( "loadWarnThreshold" );
  if ( strncmp( warn, "auto", 2 ) == 0 )
    warnThreshold = CPUMeter::countCPUs();
  else
    warnThreshold = atoi( warn );

  const char *crit = parent_->getResource( "loadCritThreshold" );
  if ( strncmp( crit, "auto", 2 ) == 0 )
    critThreshold = warnThreshold * 4;
  else
    critThreshold = atoi( crit );

  if ( dodecay_ ){
    //  Warning:  Since the loadmeter changes scale occasionally, old
    //  decay values need to be rescaled.  However, if they are rescaled,
    //  they could go off the edge of the screen.  Thus, for now, to
    //  prevent this whole problem, the load meter can not be a decay
    //  meter.  The load is a decaying average kind of thing anyway,
    //  so having a decaying load average is redundant.
    std::cerr << "Warning:  The loadmeter can not be configured as a decay\n"
              << "  meter.  See the source code (" << __FILE__ << ") for further\n"
              << "  details.\n";
    dodecay_ = 0;
  }
}

void LoadMeter::checkevent( void ){
  getloadinfo();
  drawfields();
}

void LoadMeter::getloadinfo( void ){
  double avg[3];

  if ( !AIXStats::load( avg ) )
    return;

  fields_[0] = avg[0];

  if ( fields_[0] < warnThreshold )
    alarmstate = 0;
  else if ( fields_[0] >= critThreshold )
    alarmstate = 2;
  else
    alarmstate = 1;

  if ( alarmstate != lastalarmstate ){
    if ( alarmstate == 0 )
      setfieldcolor( 0, procloadcol_ );
    else if ( alarmstate == 1 )
      setfieldcolor( 0, warnloadcol_ );
    else
      setfieldcolor( 0, critloadcol_ );
    drawlegend();
    lastalarmstate = alarmstate;
  }

  // Adjust total to next power-of-two of the current load.
  if ( ( fields_[0] * 5.0 < total_ && total_ > 1.0 ) || fields_[0] > total_ ){
    unsigned int i = (unsigned int)fields_[0];
    i |= i >> 1; i |= i >> 2; i |= i >> 4; i |= i >> 8; i |= i >> 16;
    total_ = i + 1;
  }

  fields_[1] = total_ - fields_[0];

  setUsed( fields_[0], 1.0 );
}
