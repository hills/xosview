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
#include <strstream.h>
#include <stdlib.h>

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
  ifstream meminfo(MEMFILENAME);
  if (!meminfo){
    cerr << "Can not open file : " << MEMFILENAME << endl;
    exit(1);
  }
 
  bzero(mstat, sizeof(MemStat));

  char buf[256];
  char ignore[256];

  // Get the info from the "standard" meminfo file.
  while (!meminfo.eof()){
    meminfo.getline(buf, 256);
    istrstream line(buf, 256);

    if(!strncmp("MemTotal", buf, strlen("MemTotal")))
      line >> ignore >> mstat->total;

    if(!strncmp("MemFree", buf, strlen("MemFree")))
      line >> ignore >> mstat->free;

    if(!strncmp("Buffers", buf, strlen("Buffers")))
      line >> ignore >> mstat->buff;

    if(!strncmp("Cached", buf, strlen("Cached")))
      line >> ignore >> mstat->cache;
  }

  // If the xosview memstat module is here then we
  // can look for shared memory stats.
  if (_shAdj == 0){
    ifstream memstat(MEMSTATFNAME);
    if ( !memstat ){
      cerr <<"Can not open file : " <<MEMSTATFNAME <<endl;
      exit( 1 );
    }
    
    while (!memstat.eof()){
      memstat.getline(buf, 256);
      istrstream line(buf, 256);
    
      if(!strncmp("Shared", buf, strlen("Shared")))
        line >> ignore >> mstat->shared;
    }
  }
}
