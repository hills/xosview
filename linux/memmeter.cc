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

  _MSlineInfos = NULL;
  _MIlineInfos = NULL;
  initLineInfo();
}

MemMeter::~MemMeter( void ){
  delete[] _MIlineInfos;
  delete[] _MSlineInfos;
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
  getmemstat(MEMFILENAME, _MIlineInfos, _numMIlineInfos);
  if (_shAdj == 0)
    getmemstat(MEMSTATFNAME, _MSlineInfos, _numMSlineInfos);

  fields_[0] = total_ - fields_[4 + _shAdj] - fields_[3 + _shAdj] 
    - fields_[2 + _shAdj];
  if (_shAdj == 0)
    fields_[0] -= fields_[1];

  if (total_)
    FieldMeterDecay::used( (int)((100 * (total_ - fields_[4 + _shAdj])) 
                                 / total_) );
}

MemMeter::LineInfo *MemMeter::findLines(LineInfo *tmplate, int len, 
                                             const char *fname){
  ifstream meminfo(fname);
  if (!meminfo){
    cerr << "Can not open file : " << fname << endl;
    exit(1);
  }

  LineInfo *rval = new LineInfo[len];

  char buf[256];
  istrstream line(buf, 256);

  // Get the info from the "standard" meminfo file.
  int lineNum = 0;
  int inum = 0;  // which info are we going to insert
  while (!meminfo.eof()){
    meminfo.getline(buf, 256);
    line.seekg(0, ios::beg);
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
  ifstream meminfo(fname);
  if (!meminfo){
    cerr << "Can not open file : " << fname << endl;
    exit(1);
  }

  char buf[256];
  istrstream line(buf, 256);

  char ignore[256];

  // Get the info from the "standard" meminfo file.
  int lineNum = 0;
  int inum = 0;
  while (!meminfo.eof()){
    meminfo.getline(buf, 256);
    lineNum++;

    if (lineNum != infos[inum].line())
      continue;

    line.seekg(0, ios::beg);

    unsigned long val;
    line >> ignore >> val;
    infos[inum].setVal(val);

    inum++;
    if (inum >= ninfos)
      break;
  }
}
