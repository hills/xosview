//
//  BSD version based on the Linux version by Mike:
//  Copyright (c) 1997 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//  BSD porting work done by David W. Talmage
//  <talmage@jefferson.cmf.nrl.navy.mil>
//
//  Only supported for NetBSD so far.

#include "btrymeter.h"
#ifdef HAVE_BATTERY_METER

#include "xosview.h"
#ifdef HAVE_FSTREAM
#include <fstream>
#else
#include <fstream.h>
#endif
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
  float minutes_left, max_left;
  int error = 0;
  int loadinfo = open(APMFILENAME, O_RDONLY, 0);

  if ( !loadinfo ){
    std::cerr <<"Can not open file : " <<APMFILENAME << std::endl;
    parent_->done(1);
    return;
  }

  error = ioctl(loadinfo, APM_IOC_GETPOWER, &buff);

  if (error != -1) {
    if (buff.battery_state != APM_BATT_ABSENT) {

      fields_[0] = (float)buff.battery_life;	// percent left
      fields_[1] = 100.0 - fields_[0];		// percent used
      minutes_left = (float)buff.minutes_left;

      //    minutes_left = (percent_left/100.0) * max_minutes;
      //    minutes_left / (percent_left/100.0) = max_minutes;
      //    minutes_left * 100.0 / percent_left = max_minutes;
      if (minutes_left == 0) {
        // This fix a bogus warning when the apm bios doesn't know
        // the correct remaining time.
        minutes_left = (float)0.01;
      }
      max_left = minutes_left * 100.0 / fields_[0];

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
  }

  close(loadinfo);
}

#endif  // HAVE_BATTERY_METER
