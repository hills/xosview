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
  parent, 3, "DISK", "READ/WRITE/IDLE")
    {
    read_prev_ = 0;
    write_prev_ = 0;
    maxspeed_ = max;
    getdiskinfo();
    }

DiskMeter::~DiskMeter( void )
    {
    }

void DiskMeter::checkResources( void )
    {
    FieldMeterGraph::checkResources();

    setfieldcolor( 0, parent_->getResource("diskReadColor") );
    setfieldcolor( 1, parent_->getResource("diskWriteColor") );
    setfieldcolor( 2, parent_->getResource("diskIdleColor") );
    priority_ = atoi (parent_->getResource( "diskPriority" ) );
    dodecay_ = parent_->isResourceTrue("diskDecay" );
    useGraph_ = parent_->isResourceTrue( "diskGraph" );
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

    // assume each "unit" is 4k.  This could very well be wrong.
    // SM: This is wrong. A quick look at the kernel shows that
    // 'disk_rblk' and 'disk_wblk' are the number of disk sectors
    // read/written. The 'disk' entry is the total number of R/W
    // commands sent to the drive. We need a way of determining
    // the sector size properly rather than just guessing.
    unsigned long int read_curr = (rone + rtwo + rthree + rfour) * 4 * 1024;

    // Now get the writes
    stats >> buf;
    while (strncmp(buf, "disk_wblk", 9))
        {
        stats.ignore(1024, '\n');
        stats >> buf;
        }

    unsigned long wone, wtwo, wthree, wfour;
    stats >>wone >> wtwo >> wthree >> wfour;

    unsigned long int write_curr = (wone + wtwo + wthree + wfour) * 4 * 1024;

   	fields_[0] = ((read_curr - read_prev_) * 1e6) / IntervalTimeInMicrosecs();
  	fields_[1] = ((write_curr - write_prev_) * 1e6) / IntervalTimeInMicrosecs();

  	if (fields_[0] + fields_[1] > total_)
        total_ = fields_[0] + fields_[1];

	fields_[2] = total_ - (fields_[0] + fields_[1]);
    
    read_prev_ = read_curr;
    write_prev_ = write_curr;

    setUsed((fields_[0]+fields_[1]) * IntervalTimeInMicrosecs()/1e6, total_);
    IntervalTimerStart();
    }
