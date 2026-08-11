//
//  Copyright (c) 2026 by Antoni Sawicki ( as@tenoware.com )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _LOADMETER_H_
#define _LOADMETER_H_

#include "fieldmetergraph.h"
#include "xosview.h"


class LoadMeter : public FieldMeterGraph {
public:
  LoadMeter( XOSView *parent );
  ~LoadMeter( void );

  const char *name( void ) const { return "LoadMeter"; }
  void checkevent( void );

  void checkResources( void );

protected:
  void getloadinfo( void );
  unsigned long procloadcol_, warnloadcol_, critloadcol_;

private:
  bool ok_;
  int warnThreshold, critThreshold, alarmstate, lastalarmstate;
};

#endif
