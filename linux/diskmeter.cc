//  
//  Copyright (c) 1999 by Mike Romberg (romberg@fsl.noaa.gov)
//
//  This file may be distributed under terms of the GPL
//
// $Id$
//

#include "diskmeter.h"
#include "xosview.h"
#include <fstream.h>
#include <stdlib.h>

static const char STATFILENAME[] = "/proc/stat";

DiskMeter::DiskMeter( XOSView *parent, float max ) : FieldMeterGraph(
  parent, 2, "DISK", "XFER/IDLE")
    {
    prev_ = 0;
    maxspeed_ = max;
    }

DiskMeter::~DiskMeter( void )
    {
    }

void DiskMeter::checkResources( void )
    {
    FieldMeterDecay::checkResources();

    setfieldcolor( 0, parent_->getResource("diskUsedColor") );
    setfieldcolor( 1, parent_->getResource("diskIdleColor") );
    priority_ = atoi (parent_->getResource( "diskPriority" ) );
    dodecay_ = !strncasecmp(parent_->getResource("diskDecay" ), "True", 5);
    useGraph_ = !strncasecmp(parent_->getResource( "diskGraph" ), "True", 5 );
    SetUsedFormat(parent_->getResource("diskUsedFormat"));
    }

void DiskMeter::checkevent( void )
    {
    getdiskinfo();
    drawfields();
    }

void DiskMeter::getdiskinfo( void )
    {
    IntervalTimerStop();
    total_ = maxspeed_;
    char buf[256];
    ifstream stats( STATFILENAME );

    if ( !stats )
        {
        cerr <<"Can not open file : " <<STATFILENAME <<endl;
        exit( 1 );
        }

    // Find the line with the rblk
    stats >> buf;
    while (strncmp(buf, "disk_rblk", 9))
        {
        stats.ignore(1024, '\n');
        stats >> buf;
        }

    unsigned long rone, rtwo, rthree, rfour;
    stats >>rone >> rtwo >> rthree >> rfour;

    // Now get the writes
    stats >> buf;
    while (strncmp(buf, "disk_wblk", 9))
        {
        stats.ignore(1024, '\n');
        stats >> buf;
        }

    unsigned long wone, wtwo, wthree, wfour;
    stats >>wone >> wtwo >> wthree >> wfour;

    unsigned long one = rone + wone, two = rtwo + wtwo, 
      three = rthree + wthree, four = rfour + wfour;

    // assume each "unit" is 4k.  This could very well be wrong.
    unsigned long int curr = (one + two + three + four) * 4 * 1024;
    if( prev_ )
        {
    	fields_[0] = ((curr - prev_) * 1e6) / IntervalTimeInMicrosecs();
    	if (fields_[0] > total_)
            total_ = fields_[0];
    	fields_[1] = total_ - fields_[0];
        }
    else 
        {
        fields_[0] = 0;
        fields_[1] = 0;
        }
    
    prev_ = curr;

    setUsed(fields_[0] * IntervalTimeInMicrosecs()/1e6, total_);
    IntervalTimerStart();
    }
