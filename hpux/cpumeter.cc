//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "cpumeter.h"
#include "xosview.h"
#include <sys/pstat.h>
#include <stdlib.h>

CPUMeter::CPUMeter( XOSView *parent )
: FieldMeterGraph( parent, 5, "CPU", "USR/NICE/SYS/INT/FREE" ){
  for ( int i = 0 ; i < 2 ; i++ )
    for ( int j = 0 ; j < 5 ; j++ )
      cputime_[i][j] = 0;
  cpuindex_ = 0;
}

CPUMeter::~CPUMeter( void ){
}

void CPUMeter::checkResources( void ){
  FieldMeterGraph::checkResources();

  setfieldcolor( 0, parent_->getResource( "cpuUserColor" ) );
  setfieldcolor( 1, parent_->getResource( "cpuNiceColor" ) );
  setfieldcolor( 2, parent_->getResource( "cpuSystemColor" ) );
  setfieldcolor( 3, parent_->getResource( "cpuInterruptColor" ) );
  setfieldcolor( 4, parent_->getResource( "cpuFreeColor" ) );
  priority_ = atoi (parent_->getResource( "cpuPriority" ) );
  dodecay_ = parent_->isResourceTrue( "cpuDecay" );
  useGraph_ = parent_->isResourceTrue( "cpuGraph" );
  SetUsedFormat( parent_->getResource( "cpuUsedFormat" ) );
}

void CPUMeter::checkevent( void ){
  getcputime();
  drawfields();
}

void CPUMeter::getcputime( void ){
  total_ = 0;
  struct pst_dynamic stats;

  pstat_getdynamic( &stats, sizeof( struct pst_dynamic ), 1, 0 );

  cputime_[cpuindex_][0] = stats.psd_cpu_time[0];
  cputime_[cpuindex_][1] = stats.psd_cpu_time[1];
  cputime_[cpuindex_][2] = stats.psd_cpu_time[2];
  cputime_[cpuindex_][3] = stats.psd_cpu_time[4];
  cputime_[cpuindex_][4] = stats.psd_cpu_time[3];

  int oldindex = (cpuindex_+1)%2;
  for ( int i = 0 ; i < 5 ; i++ ){
    fields_[i] = cputime_[cpuindex_][i] - cputime_[oldindex][i];
    total_ += fields_[i];
  }
  cpuindex_ = (cpuindex_ + 1) % 2;

  if (total_){
    setUsed( total_ - fields_[4], total_ );
  }
}
