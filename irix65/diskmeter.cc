//  
// $Id$
//  Initial port performed by Stefan Eilemann (eile@sgi.com)
//

#include "diskmeter.h"
#include "xosview.h"
#include "sarmeter.h"

#include <stdlib.h>


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
    diskinfo *di = SarMeter::Instance()->getDiskInfo();

    if( di == NULL )
        return;

    // new data
    total_ = maxspeed_;
    unsigned int read_curr, write_curr;

    write_curr = di[0].stat.io_wbcnt;
    read_curr  = di[0].stat.io_bcnt - write_curr;

    // avoid strange values at first call
    if(read_prev_ == 0) read_prev_ = read_curr;
    if(write_prev_ == 0) write_prev_ = write_curr;
    
    // calculate rate in bytes per second
    fields_[0] = ((read_curr - read_prev_) * 512);
    fields_[1] = ((write_curr - write_prev_) * 512);
    cerr << fields_[0] << " " << fields_[1] <<endl;

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

