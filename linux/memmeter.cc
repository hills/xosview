//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "memmeter.h"
#include <stdlib.h>
// #include <stdio.h>
#include <string.h>
#include <fstream>
#include <iostream>

static const char MEMFILENAME[] = "/proc/meminfo";


MemMeter::MemMeter( XOSView *parent )
: FieldMeterGraph( parent, 6, "MEM", "USED/BUFF/SLAB/MAP/CACHE/FREE" ){
  _MIlineInfos = NULL;
  initLineInfo();
}

MemMeter::~MemMeter( void ){
  delete[] _MIlineInfos;
}

void MemMeter::checkResources( void ){
  FieldMeterGraph::checkResources();

  setfieldcolor( 0, parent_->getResource( "memUsedColor" ) );
  setfieldcolor( 1, parent_->getResource( "memBufferColor" ) );
  setfieldcolor( 2, parent_->getResource( "memSlabColor" ) );
  setfieldcolor( 3, parent_->getResource( "memMapColor" ) );
  setfieldcolor( 4, parent_->getResource( "memCacheColor" ) );
  setfieldcolor( 5, parent_->getResource( "memFreeColor" ) );
  priority_ = atoi (parent_->getResource( "memPriority" ) );
  dodecay_ = parent_->isResourceTrue( "memDecay" );
  useGraph_ = parent_->isResourceTrue( "memGraph" );
  SetUsedFormat (parent_->getResource("memUsedFormat"));
}

void MemMeter::checkevent( void ){
  getmeminfo();
  /* for debugging (see below)
  printf("t %4.1f used %4.1f buffer %4.1f slab %4.1f map %4.1f cache %4.1f free %4.1f\n",
         total_/1024.0/1024.0,
         fields_[0]/1024.0/1024.0, fields_[1]/1024.0/1024.0,
	 fields_[2]/1024.0/1024.0, fields_[3]/1024.0/1024.0,
	 fields_[4]/1024.0/1024.0, fields_[5]/1024.0/1024.0);
  */
  drawfields();
}

// FIXME: /proc/memstat and /proc/meminfo don't seem to correspond
// maybe it is time to fix this in the kernel and get real infos ...

void MemMeter::getmeminfo( void ){
  getmemstat(MEMFILENAME, _MIlineInfos, _numMIlineInfos);
  /*
   * Commenting out as "mapped" and "cached" seem to be unrelated.
   * Sometimes mapped > cached, sometimes cached > mapped.
   * Let's leave them raw.
   *		--RAM, 2015-09-16
   */
#if 0	/* DISABLED */
  fields_[3] -= fields_[4]; // mapped comes from cache
#endif
  fields_[0] = total_ - fields_[5] - fields_[4] - fields_[3] - fields_[2] - fields_[1];

  if (total_)
    setUsed (total_ - fields_[5], total_);
}

MemMeter::LineInfo *MemMeter::findLines(LineInfo *tmplate, int len,
                                             const char *fname){
  std::ifstream meminfo(fname);
  if (!meminfo){
    std::cerr << "Can not open file : " << fname << std::endl;
    exit(1);
  }

  LineInfo *rval = new LineInfo[len];

  char buf[256];

  // Get the info from the "standard" meminfo file.
  int lineNum = 0;
  int inum = 0;  // which info are we going to insert
  while (!meminfo.eof()){
    meminfo.getline(buf, 256);
    lineNum++;

    for (int i = 0 ; i < len ; i++)
      if(!strncmp(tmplate[i].id(), buf, tmplate[i].idlen())){
        rval[inum] = tmplate[i];
        rval[inum].line(lineNum);
        inum++;
      }
  }

  return rval;
}

void MemMeter::initLineInfo(void){
  static LineInfo infos[] = {
    LineInfo("MemTotal", &total_),
    LineInfo("MemFree", &fields_[5]),
    LineInfo("Buffers", &fields_[1]),
    LineInfo("Slab", &fields_[2]),
    LineInfo("Mapped", &fields_[3]),
    LineInfo("Cached", &fields_[4])
  };
  _numMIlineInfos = sizeof(infos) / sizeof(LineInfo);

  _MIlineInfos = findLines(infos, _numMIlineInfos, MEMFILENAME);
}

void MemMeter::getmemstat(const char *fname, LineInfo *infos, int ninfos){
  std::ifstream meminfo(fname);
  if (!meminfo){
    std::cerr << "Can not open file : " << fname << std::endl;
    exit(1);
  }

  // Get the info from the "standard" meminfo file.
  int lineNum = 0, inum = 0;
  unsigned long long val;
  char buf[256];
  while (inum < ninfos && !meminfo.eof()){
    meminfo.getline(buf, 256);
    if (++lineNum != infos[inum].line())
      continue;

    val = strtoull(buf + infos[inum].idlen() + 1, NULL, 10);
    /*  All stats are in KB.  */
    infos[inum++].setVal((double)(val<<10));	/*  Multiply by 1024 bytes per K  */
  }
}
