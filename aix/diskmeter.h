//
//  Copyright (c) 2026 by Antoni Sawicki ( as@tenoware.com )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _DISKMETER_H_
#define _DISKMETER_H_

#include "fieldmetergraph.h"
#include "xosview.h"


class DiskMeter : public FieldMeterGraph {
public:
  DiskMeter( XOSView *parent, float max );
  ~DiskMeter( void );

  const char *name( void ) const { return "DiskMeter"; }
  void checkevent( void );

  void checkResources( void );

protected:
  void getdiskinfo( void );

private:
  float maxspeed_;
  bool ok_;
  double readPrev_, writePrev_;
  bool first_;
};

#endif
