//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//  Most of this code was written by Werner Fink <werner@suse.de>.
//  Only small changes were made on my part (M.R.)
//  And the near-trivial port to NetBSD was done by Brian Grayson

#include "loadmeter.h"
#include "xosview.h"
#include <stdlib.h>  //  for getloadavg()
#include <stdio.h>

LoadMeter::LoadMeter( XOSView *parent )
  : FieldMeterDecay( parent, 2, "LOAD", "PROCS per MIN/IDLE", 1, 0 ){
}

LoadMeter::~LoadMeter( void ){
}

void LoadMeter::checkResources( void ){
  FieldMeter::checkResources();

  warnloadcol_ = parent_->allocColor(parent_->getResource("loadWarnColor"));
  procloadcol_ = parent_->allocColor(parent_->getResource("loadProcColor"));

  if (total_ == 20 )
    setfieldcolor( 0, warnloadcol_ );
  else
    setfieldcolor( 0, procloadcol_ );

  setfieldcolor( 1,
      parent_->getResource("loadIdleColor") );

  priority_ = atoi (parent_->getResource("loadPriority"));
  dodecay_ = !strcmp (parent_->getResource("loadDecay"),"True");

  alarmThreshold = atoi (parent_->getResource("loadAlarmThreshold"));

  if (dodecay_){
    //  Warning:  Since the loadmeter changes scale occasionally, old
    //  decay values need to be rescaled.  However, if they are rescaled,
    //  they could go off the edge of the screen.  Thus, for now, to
    //  prevent this whole problem, the load meter can not be a decay
    //  meter.  The load is a decaying average kind of thing anyway,
    //  so having a decaying load average is redundant.
    cerr << "Warning:  The loadmeter can not be configured as a decay\n"
         << "  meter.  See the source code (" << __FILE__ << ") for further\n"
         << "  details.\n";
    dodecay_ = 0;
  }

  //  Now, grab a sample.  I don't know if this is needed here.  BCG
  getloadinfo();
}

void LoadMeter::checkevent( void ){
  getloadinfo();

  drawfields();
}


void LoadMeter::getloadinfo( void ){
  double oneMinLoad;

  getloadavg (&oneMinLoad, 1);  //  Only get the 1-minute-average sample.
  fields_[0] = oneMinLoad;  //  Convert from double to float.

  if ( fields_[0] > alarmThreshold ) {
    if (total_ == alarmThreshold ) {
      setfieldcolor( 0, warnloadcol_ );
      if (dolegends_) drawlegend();
    }
    total_ = fields_[1] = 20;
  } else {
    if (total_ == 20 ) {
      setfieldcolor( 0, procloadcol_ );
      if (dolegends_) drawlegend();
    }
    total_ = fields_[1] = alarmThreshold;
  }

  absolute( fields_[0] );
}
