//
//  Copyright (c) 2026 by Antoni Sawicki ( as@tenoware.com )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _SWAPMETER_H_
#define _SWAPMETER_H_

#include "fieldmeterdecay.h"
#include "xosview.h"


class SwapMeter : public FieldMeterDecay {
public:
  SwapMeter( XOSView *parent );
  ~SwapMeter( void );

  const char *name( void ) const { return "SwapMeter"; }
  void checkevent( void );

  void checkResources( void );

protected:
  bool ok_;

  void getswapinfo( void );
};

#endif
