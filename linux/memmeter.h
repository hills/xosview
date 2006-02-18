//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//
// $Id$
//
#ifndef _MEMMETER_H_
#define _MEMMETER_H_

#include "fieldmetergraph.h"

class MemMeter : public FieldMeterGraph {
public:
  MemMeter( XOSView *parent );
  ~MemMeter( void );

  const char *name( void ) const { return "MemMeter"; }
  void checkevent( void );

  void checkResources( void );
protected:

  void getmeminfo( void );
private:
  int _shAdj;

  class LineInfo {
  public:
    LineInfo(const char *id, float *val)
      { _line = -1; _id = id; _val = val; _idlen = strlen(_id); }
    LineInfo(void) {};

    int line(void) { return _line; }
    void line(int l) { _line = l; }
    const char *id(void) { return _id; }
    int idlen(void) { return _idlen; }

    void setVal(double val) { *_val = val; }

  private:
    int _line;
    const char *_id;
    int _idlen;
    float *_val;
  };

  LineInfo *_MIlineInfos;
  int _numMIlineInfos;

  LineInfo *_MSlineInfos;
  int _numMSlineInfos;

  void initLineInfo(void);
  LineInfo *findLines(LineInfo *tmplate, int len, const char *fname);
  void getmemstat(const char *fname, LineInfo *infos, int ninfos);
};


#endif
