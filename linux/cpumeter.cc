//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "cpumeter.h"
#include "xosview.h"
#include <fstream.h>
#include <stdlib.h>


static const char STATFILENAME[] = "/proc/stat";


CPUMeter::CPUMeter( XOSView *parent )
: FieldMeterDecay( parent, 4, "CPU", "USR/NICE/SYS/FREE" ){
  for ( int i = 0 ; i < 2 ; i++ )
    for ( int j = 0 ; j < 4 ; j++ )
      cputime_[i][j] = 0;
  cpuindex_ = 0;

}

CPUMeter::~CPUMeter( void ){
}

void CPUMeter::checkResources( void ){
  FieldMeterDecay::checkResources();

  setfieldcolor( 0, parent_->getResource( "cpuUserColor" ) );
  setfieldcolor( 1, parent_->getResource( "cpuNiceColor" ) );
  setfieldcolor( 2, parent_->getResource( "cpuSystemColor" ) );
  setfieldcolor( 3, parent_->getResource( "cpuFreeColor" ) );
  priority_ = atoi (parent_->getResource( "cpuPriority" ) );
  dodecay_ = !strcmp (parent_->getResource( "cpuDecay" ), "True" );
}

void CPUMeter::checkevent( void ){
  getcputime();
  drawfields();
}

void CPUMeter::getcputime( void ){
  total_ = 0;
  char tmp[10];
  ifstream stats( STATFILENAME );

  if ( !stats ){
    cerr <<"Con not open file : " <<STATFILENAME <<endl;
    exit( 1 );
  }

  stats >>tmp >>cputime_[cpuindex_][0]  
	      >>cputime_[cpuindex_][1]  
	      >>cputime_[cpuindex_][2]  
	      >>cputime_[cpuindex_][3];

  int oldindex = (cpuindex_+1)%2;
  for ( int i = 0 ; i < 4 ; i++ ){
    fields_[i] = cputime_[cpuindex_][i] - cputime_[oldindex][i];
    total_ += fields_[i];
  }

  if (total_){
    used( (int)((100 * (total_ - fields_[3])) / total_) );
    cpuindex_ = (cpuindex_ + 1) % 2;
  }
}
