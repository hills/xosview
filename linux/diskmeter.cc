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
#define MAX_PROCSTAT_LENGTH 2048

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
    char buf[MAX_PROCSTAT_LENGTH];
    ifstream stats( STATFILENAME );

    if ( !stats )
        {
        cerr <<"Can not open file : " <<STATFILENAME <<endl;
        exit( 1 );
        }

    // Find the line with 'page'
    stats >> buf;
    while (strncmp(buf, "page", 9))
        {
        stats.ignore(MAX_PROCSTAT_LENGTH, '\n');
        stats >> buf;
        }

	// read values
    unsigned long one, two;
    stats >> one >> two;

    // assume each "unit" is 1k. 
    // This is true for ext2, but seems to be 512 bytes 
    // for vfat and 2k for cdroms
    // work in 512-byte blocks
    
    // tw: strange, on my system, a ext2fs (read and write)
    // unit seems to be 2048. kernel 2.2.12 and the file system
    // is on a SW-RAID5 device (/dev/md0).
    
    // So this is a FIXME - but how ???
    
    float itim = IntervalTimeInMicrosecs();
    unsigned long read_curr = one * 2;  // FIXME!
    unsigned long write_curr = two * 2; // FIXME!

    // avoid strange values at first call
    if(read_prev_ == 0) read_prev_ = read_curr;
    if(write_prev_ == 0) write_prev_ = write_curr;
    
    // calculate rate in bytes per second
    fields_[0] = ((read_curr - read_prev_) * 1e6 * 512) / itim;
    fields_[1] = ((write_curr - write_prev_) * 1e6 * 512) / itim;

    // fix overflow (conversion bug?)
    if (fields_[0] < 0.0)
        fields_[0] = 0.0;
    if (fields_[1] < 0.0)
        fields_[1] = 0.0;
    
    if (fields_[0] + fields_[1] > total_)
       	total_ = fields_[0] + fields_[1];

    fields_[2] = total_ - (fields_[0] + fields_[1]);
    
    read_prev_ = read_curr;
    write_prev_ = write_curr;
    
    setUsed((fields_[0]+fields_[1]), total_);
    IntervalTimerStart();
    }
