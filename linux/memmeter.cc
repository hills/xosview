//
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
// $Id$
//
#include "memmeter.h"
#include "xosview.h"
#include <fstream>
#include <sstream>
#include <stdlib.h>

static const char MEMFILENAME[] = "/proc/meminfo";
static const char MEMSTATFNAME[] = "/proc/memstat";

MemMeter::MemMeter( XOSView *parent )
: FieldMeterGraph( parent, 4, "MEM", "USED+SHAR/BUFF/CACHE/FREE" ){
  _shAdj = -1;

  // Check and see if the memstat module has been loaded
  std::ifstream test(MEMSTATFNAME);
  if (test){
    setNumFields(5);
    legend("USED/SHAR/BUFF/CACHE/FREE");
    _shAdj = 0;
  }

  _MSlineInfos = NULL;
  _MIlineInfos = NULL;
  initLineInfo();
}

MemMeter::~MemMeter( void ){
  delete[] _MIlineInfos;
  delete[] _MSlineInfos;
}

void MemMeter::checkResources( void ){
  FieldMeterGraph::checkResources();

  setfieldcolor( 0, parent_->getResource( "memUsedColor" ) );
  if (_shAdj == 0)
    setfieldcolor( 1, parent_->getResource( "memSharedColor" ) );
  setfieldcolor( 2 + _shAdj, parent_->getResource( "memBufferColor" ) );
  setfieldcolor( 3 + _shAdj, parent_->getResource( "memCacheColor" ) );
  setfieldcolor( 4 + _shAdj, parent_->getResource( "memFreeColor" ) );
  priority_ = atoi (parent_->getResource( "memPriority" ) );
  dodecay_ = parent_->isResourceTrue( "memDecay" );
  useGraph_ = parent_->isResourceTrue( "memGraph" );
  SetUsedFormat (parent_->getResource("memUsedFormat"));
}

void MemMeter::checkevent( void ){
  getmeminfo();
  /* for debugging (see below)
  printf("t %4.1f used %4.1f share %4.1f buffer %4.1f cache %4.1f free %4.1f\n",
         total_/1024.0/1024.0,
         fields_[0]/1024.0/1024.0, fields_[1]/1024.0/1024.0,
	 fields_[2]/1024.0/1024.0, fields_[3]/1024.0/1024.0,
	 fields_[4]/1024.0/1024.0);
  */
  drawfields();
}

// FIXME: /proc/memstat and /proc/meminfo don't seem to correspond
// maybe it is time to fix this in the kernel and get real infos ...

void MemMeter::getmeminfo( void ){
  getmemstat(MEMFILENAME, _MIlineInfos, _numMIlineInfos);
  if (_shAdj == 0)
    getmemstat(MEMSTATFNAME, _MSlineInfos, _numMSlineInfos);

  if (_shAdj == 0){
    fields_[3] -= fields_[1]; // cache size seems to contain shared size !?
                              // without this fix "used" sometimes gets < 0 !
    fields_[0] = total_ - fields_[4] - fields_[3] - fields_[2] - fields_[1];
  }else{
    fields_[0] = total_ - fields_[3] - fields_[2] - fields_[1];
  }

  if (total_)
    FieldMeterDecay::setUsed (total_ - fields_[4 + _shAdj], total_);
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
    LineInfo("MemFree", &fields_[4 + _shAdj]),
    LineInfo("Buffers", &fields_[2 + _shAdj]),
    LineInfo("Cached", &fields_[3 + _shAdj])
  };
  _numMIlineInfos = sizeof(infos) / sizeof(LineInfo);

  _MIlineInfos = findLines(infos, _numMIlineInfos, MEMFILENAME);

  static LineInfo msinfos[] = {
    LineInfo("Shared", &fields_[1])
  };
  _numMSlineInfos = sizeof(msinfos) / sizeof(LineInfo);

  if (_shAdj == 0)
    _MSlineInfos = findLines(msinfos, _numMSlineInfos, MEMSTATFNAME);
}

void MemMeter::getmemstat(const char *fname, LineInfo *infos, int ninfos){
  std::ifstream meminfo(fname);
  if (!meminfo){
    std::cerr << "Can not open file : " << fname << std::endl;
    exit(1);
  }

  char buf[256];

  // Get the info from the "standard" meminfo file.
  int lineNum = 0;
  int inum = 0;
  while (!meminfo.eof()){
    meminfo.getline(buf, 256);
    lineNum++;
    if (lineNum != infos[inum].line())
      continue;

    std::string sline(buf, 256);
    std::istringstream line(sline);
    unsigned long val;
    std::string ignore;
    line >> ignore >> val;
    /*  All stats are in KB.  */
    infos[inum].setVal(val*1024.0);	/*  Multiply by 1024 bytes per K  */

    inum++;
    if (inum >= ninfos)
      break;
  }
}
