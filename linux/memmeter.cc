//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "memmeter.h"
#include "xosview.h"
#include <fstream.h>
#include <stdlib.h>

#ifdef USESYSCALLS
#undef USESYSCALLS
#endif

#ifdef USESYSCALLS
#include <syscall.h>
#include <linux/kernel.h>
#endif

static const char MEMFILENAME[] = "/proc/meminfo";


MemMeter::MemMeter( XOSView *parent )
: FieldMeterDecay( parent, 5, "MEM", "USED/SHAR/BUFF/CACHE/FREE" ){

}

MemMeter::~MemMeter( void ){
}

void MemMeter::checkResources( void ){
  FieldMeterDecay::checkResources();

  setfieldcolor( 0, parent_->getResource( "memUsedColor" ) );
  setfieldcolor( 1, parent_->getResource( "memSharedColor" ) );
  setfieldcolor( 2, parent_->getResource( "memBufferColor" ) );
  setfieldcolor( 3, parent_->getResource( "memCacheColor" ) );
  setfieldcolor( 4, parent_->getResource( "memFreeColor" ) );
  priority_ = atoi (parent_->getResource( "memPriority" ) );
  dodecay_ = !strcmp (parent_->getResource( "memDecay" ), "True" );
}

void MemMeter::checkevent( void ){
  getmeminfo();
  drawfields();
}

#ifdef USESYSCALLS
void MemMeter::getmeminfo( void ){
  struct sysinfo sinfo;

  syscall( SYS_sysinfo, &sinfo );

  total_ = sinfo.totalram;
  fields_[3] = sinfo.freeram;
  fields_[1] = sinfo.sharedram;
  fields_[2] = sinfo.bufferram;

  fields_[0] = total_ - fields_[3] - fields_[2] - fields_[1];
  if ( fields_[0] < 0 ){
    // Fixing up the user field
    fields_[0] = 0;

    // Fixing up the buffers field
    fields_[2] = total_ - fields_[3] - fields_[1];
  }

  if (total_)
    FieldMeterDecay::used( (int)((100 * (total_ - fields_[3])) / total_) );
}
#else
void MemMeter::getmeminfo( void ){
  float used;
  char buf[256];  

  ifstream meminfo( MEMFILENAME );

  if ( !meminfo ){
    cerr <<"Con not open file : " <<MEMFILENAME <<endl;
    exit( 1 );
  }

  ifstream memstat( "/proc/memstat" );

  if ( !memstat ){
    cerr <<"Con not open file : " <<"/proc/memstat" <<endl;
    exit( 1 );
  }

  meminfo.getline( buf, 256 );

  meminfo >>buf >>total_ >>used >>fields_[4] 
          >>fields_[1] >>fields_[2] >>fields_[3];

  while (!memstat.eof()){
    memstat >> buf >> fields_[1];
    fields_[1] *= 1024;
    if (memstat.eof() || !strcmp("Shared:", buf))
      break;
  }

  fields_[0] = total_ - fields_[4] - fields_[3] - fields_[2] - fields_[1];

//   float tt = 0;
//   for (int i = 0 ; i < 5 ; i++){
//     tt += fields_[i];
//     cerr <<fields_[i] <<", ";
//   }
//   cerr <<tt <<endl;

  if ( fields_[0] < 0 ){
    // Fixing up the user field
    fields_[0] = 0;

    // Fixing up the buffers field
    fields_[2] = total_ - fields_[4] - fields_[1];
  }
  
  if (total_)
    FieldMeterDecay::used( (int)((100 * (total_ - fields_[3])) / total_) );
}
#endif

