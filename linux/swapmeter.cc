//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "swapmeter.h"
#include "xosview.h"
#include <fstream.h>
#include <stdlib.h>

#ifdef USESYSCALLS
#include <syscall.h>
#include <linux/kernel.h>
#endif


static const char MEMFILENAME[] = "/proc/meminfo";


SwapMeter::SwapMeter( XOSView *parent )
: FieldMeterDecay( parent, 2, "SWAP", "USED/FREE" ){
  
}

SwapMeter::~SwapMeter( void ){
}

void SwapMeter::checkResources( void ){
  FieldMeterDecay::checkResources();

  setfieldcolor( 0, parent_->getResource( "swapUsedColor" ) );
  setfieldcolor( 1, parent_->getResource( "swapFreeColor" ) );
  priority_ = atoi (parent_->getResource( "swapPriority" ) );
  dodecay_ = !strcmp (parent_->getResource( "swapDecay" ), "True" );
}

void SwapMeter::checkevent( void ){
  getswapinfo();
  drawfields();
}


#ifdef USESYSCALLS
void SwapMeter::getswapinfo( void ){
  struct sysinfo sinfo;

  syscall( SYS_sysinfo, &sinfo );

  total_ = sinfo.totalswap;
  fields_[0] = total_ - sinfo.freeswap;
  fields_[1] = sinfo.freeswap;

  if ( total_ == 0 ){
    total_ = 1;
    fields_[0] = 0;
    fields_[1] = 1;
  }
  
  if (total_)
    used( (int)((100 * fields_[0]) / total_ ) );
}
#else
void SwapMeter::getswapinfo( void ){
  char buf[256];  
  ifstream meminfo( MEMFILENAME );

  if ( !meminfo ){
    cerr <<"Con not open file : " <<MEMFILENAME <<endl;
    exit( 1 );
  }

  meminfo.getline( buf, 256 );
  meminfo.getline( buf, 256 );  
  meminfo >>buf >>total_ >>fields_[0] >>fields_[1];

  if ( total_ == 0 ){
    total_ = 1;
    fields_[0] = 0;
    fields_[1] = 1;
  }

  if (total_)
    used( (int)((100 * fields_[0]) / total_ ) );
}
#endif

