//
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//  2007 by Samuel Thibault ( samuel.thibault@ens-lyon.org )
//
//  This file may be distributed under terms of the GPL
//
//  Most of this code was written by Werner Fink <werner@suse.de>.
//  Only small changes were made on my part (M.R.)
//

#include "loadmeter.h"
#include "xosview.h"
#include <fstream>
#include <stdlib.h>
#include <error.h>

extern "C" {
#include <mach/mach_traps.h>
#include <mach/mach_host.h>
}

LoadMeter::LoadMeter( XOSView *parent )
  : FieldMeterGraph( parent, 2, "LOAD", "PROCS/MIN", 1, 1, 0 ){
  lastalarmstate = -1;
  total_ = 2.0;
}

LoadMeter::~LoadMeter( void ){
}

void LoadMeter::checkResources( void ){
  FieldMeterGraph::checkResources();

  procloadcol_ = parent_->allocColor(parent_->getResource( "loadProcColor" ));
  warnloadcol_ = parent_->allocColor(parent_->getResource( "loadWarnColor" ));
  critloadcol_ = parent_->allocColor(parent_->getResource( "loadCritColor" ));

  setfieldcolor( 0, procloadcol_ );
  setfieldcolor( 1, parent_->getResource( "loadIdleColor" ) );
  priority_ = atoi (parent_->getResource( "loadPriority" ) );
  useGraph_ = parent_->isResourceTrue( "loadGraph" );
  dodecay_ = parent_->isResourceTrue( "loadDecay" );
  SetUsedFormat (parent_->getResource("loadUsedFormat"));

  warnThreshold = atoi (parent_->getResource("loadWarnThreshold"));
  critThreshold = atoi (parent_->getResource("loadCritThreshold"));


  if (dodecay_){
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
  host_load_info_data_t info;
  mach_msg_type_number_t count = HOST_LOAD_INFO_COUNT;
  kern_return_t err;

  err = host_info(mach_host_self(), HOST_LOAD_INFO, (host_info_t) &info, &count);
  if (err) {
    std::cerr << "Can not get host info";
    parent_->done(1);
    return;
  }
  fields_[0] = (float) info.avenrun[0] / LOAD_SCALE;

  if ( fields_[0] <  warnThreshold ) alarmstate = 0;
  else
  if ( fields_[0] >= critThreshold ) alarmstate = 2;
  else
  /* if fields_[0] >= warnThreshold */ alarmstate = 1;

  if ( alarmstate != lastalarmstate ){
    if ( alarmstate == 0 ) setfieldcolor( 0, procloadcol_ );
    else
    if ( alarmstate == 1 ) setfieldcolor( 0, warnloadcol_ );
    else
    /* if alarmstate == 2 */ setfieldcolor( 0, critloadcol_ );
    drawlegend();
    lastalarmstate = alarmstate;
  }

  if ( fields_[0]*5.0<total_ )
    total_ = fields_[0];
  else
  if ( fields_[0]>total_ )
    total_ = fields_[0]*5.0;

  if ( total_ < 1.0)
    total_ = 1.0;

  fields_[1] = (float) (total_ - fields_[0]);

  setUsed(fields_[0], (float) 1.0);
}
