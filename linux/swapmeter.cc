//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//
// $Id$
//

#include "swapmeter.h"
#include "xosview.h"
#include <fstream.h>
#include <strstream.h>
#include <stdlib.h>

#ifdef USESYSCALLS
#if defined(GNULIBC) || defined(__GLIBC__)
#include <sys/sysinfo.h>
#else
#include <syscall.h>
#include <linux/kernel.h>
#endif
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

#if defined(GNULIBC) || defined(__GLIBC__)
  sysinfo(&sinfo);
#else
  syscall( SYS_sysinfo, &sinfo );
#endif

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
  ifstream meminfo( MEMFILENAME );
  if ( !meminfo ){
    cerr <<"Con not open file : " <<MEMFILENAME <<endl;
    exit( 1 );
  }

  total_ = fields_[0] = fields_[1] = 0;

  char buf[256];
  char ignore[256];

  // Get the info from the "standard" meminfo file.
  while (!meminfo.eof()){
    meminfo.getline(buf, 256);
    istrstream line(buf, 256);

    if(!strncmp("SwapTotal", buf, strlen("SwapTotal")))
        line >> ignore >> total_;

    if(!strncmp("SwapFree", buf, strlen("SwapFree")))
        line >> ignore >> fields_[1];
  }
  
  fields_[0] = total_ - fields_[1];

  if ( total_ == 0 ){
    total_ = 1;
    fields_[0] = 0;
    fields_[1] = 1;
  }

  if (total_)
    used( (int)((100 * fields_[0]) / total_ ) );
}
#endif

