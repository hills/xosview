//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
// $Id$
//
#include "memmeter.h"
#include "xosview.h"
#include <fstream.h>
#include <stdlib.h>

// DISABLED (see MemMeter::getmemstat())
#ifdef USESYSCALLS_DISABLED
#include <syscall.h>
#include <linux/kernel.h>
#endif


static const char MEMFILENAME[] = "/proc/meminfo";
static const char MEMSTATFNAME[] = "/proc/memstat";


MemMeter::MemMeter( XOSView *parent )
: FieldMeterDecay( parent, 4, "MEM", "USED+SHAR/BUFF/CACHE/FREE" ){
  _shAdj = -1;

  // Check and see if the memstat module has been loaded
  ifstream test(MEMSTATFNAME);
  if (test){
    setNumFields(5);
    legend("USED/SHAR/BUFF/CACHE/FREE");
    _shAdj = 0;
  }
}

MemMeter::~MemMeter( void ){
}

void MemMeter::checkResources( void ){
  FieldMeterDecay::checkResources();

  setfieldcolor( 0, parent_->getResource( "memUsedColor" ) );
  if (_shAdj == 0)
    setfieldcolor( 1, parent_->getResource( "memSharedColor" ) );
  setfieldcolor( 2 + _shAdj, parent_->getResource( "memBufferColor" ) );
  setfieldcolor( 3 + _shAdj, parent_->getResource( "memCacheColor" ) );
  setfieldcolor( 4 + _shAdj, parent_->getResource( "memFreeColor" ) );
  priority_ = atoi (parent_->getResource( "memPriority" ) );
  dodecay_ = !strcmp (parent_->getResource( "memDecay" ), "True" );
}

void MemMeter::checkevent( void ){
  getmeminfo();
  drawfields();
}

void MemMeter::getmeminfo( void ){
  MemStat mstat;
  getmemstat(&mstat);

  total_ = mstat.total;

  fields_[0] = total_ - mstat.free - mstat.cache - mstat.buff;
  if (_shAdj == 0){
    fields_[0] -= mstat.shared;
    fields_[1] = mstat.shared;
  }
  fields_[2 + _shAdj] = mstat.buff;
  fields_[3 + _shAdj] = mstat.cache;
  fields_[4 + _shAdj] = mstat.free;
  
  if (total_)
    FieldMeterDecay::used( (int)((100 * (total_ - fields_[4 + _shAdj])) 
                                 / total_) );
}

void MemMeter::getmemstat(MemStat *mstat){
  // This has been disabled because cached ram is not in
  // struct sysinfo so we will have to read meminfo anyway.
#ifdef USESYSCALLS_DISABLED

  struct sysinfo sinfo;

  syscall( SYS_sysinfo, &sinfo );

  mstat->total = sinfo.totalram;
  mstat.free = sinfo.freeram;
  mstat.shared = sinfo.sharedram;
  mstat.buff = sinfo.bufferram;

#else

  ifstream meminfo( MEMFILENAME );
  if ( !meminfo ){
    cerr <<"Con not open file : " <<MEMFILENAME <<endl;
    exit( 1 );
  }

  char buf[256];
  meminfo.getline( buf, 256 );

  meminfo >>buf >>mstat->total >>mstat->used >>mstat->free
          >>mstat->shared >>mstat->buff >>mstat->cache;

#endif

  if (_shAdj == 0){
    ifstream memstat(MEMSTATFNAME);
    if ( !memstat ){
      cerr <<"Con not open file : " <<MEMSTATFNAME <<endl;
      exit( 1 );
    }

    while (!memstat.eof()){
      memstat >> buf >> mstat->shared;
      mstat->shared *= 1024;
      if (memstat.eof() || !strcmp("Shared:", buf))
        break;
    }
  }
}
