//  
//  BSD version based on the Linux version by Mike:
//  Copyright (c) 1997 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//  BSD porting work done by David W. Talmage
//  <talmage@jefferson.cmf.nrl.navy.mil>
//
// $Id$
//
#include "btrymeter.h"
#include "xosview.h"
#include <fstream.h>
#include <stdlib.h>

#include <fcntl.h>
#include <machine/apmvar.h>
#include <math.h>
#include <sys/ioctl.h>
#include <unistd.h>

static const char APMFILENAME[] = "/dev/apm";

BtryMeter::BtryMeter( XOSView *parent )
  : FieldMeter( parent, 2, "BAT", "% LEFT/% USED", 1, 1 ){
}

BtryMeter::~BtryMeter( void ){
}

void BtryMeter::checkResources( void ){
  FieldMeter::checkResources();

  setfieldcolor( 0, parent_->getResource( "batteryUsedColor" ) );
  setfieldcolor( 1, parent_->getResource( "batteryLeftColor" ) );

  priority_ = atoi (parent_->getResource( "batteryPriority" ) );
  SetUsedFormat(parent_->getResource( "batteryUsedFormat" ) );
}

void BtryMeter::checkevent( void ){
  getpwrinfo();

  drawfields();
}


void BtryMeter::getpwrinfo( void ){
  struct apm_power_info buff;
  int error = 0;
  int loadinfo = open(APMFILENAME, O_RDONLY, 0);

  if ( !loadinfo ){
    cerr <<"Can not open file : " <<APMFILENAME <<endl;
    parent_->done(1);
    return;
  }

  error = ioctl(loadinfo, APM_IOC_GETPOWER, &buff);

  if (error != -1) {

    fields_[0] = (float)buff.battery_life;	// percent left
    fields_[1] = 100.0 - fields_[0];		// percent used
    float minutes_left = (float)buff.minutes_left; 

    //    minutes_left = (percent_left/100.0) * max_minutes;
    //    minutes_left / (percent_left/100.0) = max_minutes;
    //    minutes_left * 100.0 / percent_left = max_minutes;
    float max_left = minutes_left * 100.0 / fields_[0];
  
    //
    // Set total_ so that the graphing methods know to set the
    // relative sizes of the percent left and percent used graphs.
    //
    total_ = 100.0;

    //
    // This sets the actual number of minutes left.  If the resource
    // xosview*batteryUsedFormat is "float", then xosview will display
    // that number to the left of the battery graph.  If it's
    // "percent", then you get the percent remaining next to the
    // graph.
    //
    setUsed(minutes_left, max_left);
  }

  close(loadinfo);
}
