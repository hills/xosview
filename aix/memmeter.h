//
//  Copyright (c) 2026 by Antoni Sawicki ( as@tenoware.com )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _MEMMETER_H_
#define _MEMMETER_H_

#include "fieldmeterdecay.h"
#include "xosview.h"


class MemMeter : public FieldMeterDecay {
public:
  MemMeter( XOSView *parent );
  ~MemMeter( void );

  const char *name( void ) const { return "MemMeter"; }
  void checkevent( void );

  void checkResources( void );

protected:
  bool ok_;

  void getmeminfo( void );
};

#endif
