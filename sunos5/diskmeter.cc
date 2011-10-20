//  
//  Rewritten for Solaris by Arno Augustin 1999
//  augustin@informatik.uni-erlangen.de
//
// $Id$
//

#include "diskmeter.h"
#include "xosview.h"
#include <fstream>
#include <stdlib.h>


DiskMeter::DiskMeter( XOSView *parent, kstat_ctl_t *_kc, float max )
  : FieldMeterGraph( parent, 3, "DISK", "READ/WRITE/IDLE")
    {
      kc = _kc;
    read_prev_ = 0;
    write_prev_ = 0;
    maxspeed_ = max;
    npart=0;
    for (kstat_t *ksp = kc->kc_chain; ksp != NULL && npart <NPARTS;
	 ksp = ksp->ks_next) {
      if (ksp->ks_type == KSTAT_TYPE_IO ){
	if(kstat_read(kc, ksp, NULL) != -1) {
	  part[npart++] = ksp;
	}
      }
    }
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
    total_ = maxspeed_;
    kstat_io_t     kio;
    u_longlong_t read_curr = 0;
    u_longlong_t write_curr = 0;

    IntervalTimerStop();

    for (int i = 0; i < npart; i++) {
      if (kstat_read(kc, part[i], &kio) == -1)
	continue;
      read_curr += kio.nread;
      write_curr += kio.nwritten;
    }
    if(read_prev_ == 0) read_prev_ = read_curr;
    if(write_prev_ == 0) write_prev_ = write_curr;
    fields_[0] = (read_curr - read_prev_) / IntervalTimeInSecs();
    fields_[1] = (write_curr - write_prev_) / IntervalTimeInSecs();

    //  Adjust 
    if (fields_[0] < 0) fields_[0] = 0.0;
    if (fields_[1] < 0) fields_[1] = 0.0;
    if (fields_[0] + fields_[1] > total_)
       	total_ = fields_[0] + fields_[1];

    fields_[2] = total_ - (fields_[0] + fields_[1]);
    
    read_prev_ = read_curr;
    write_prev_ = write_curr;
    
    //setUsed((fields_[0]+fields_[1]) * IntervalTimeInMicrosecs()/1e6, total_);
    // give rate in units per second, not units per interval
    setUsed((fields_[0]+fields_[1]), total_);
    IntervalTimerStart();
}
