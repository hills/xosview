//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _SWAPMETER_H_
#define _SWAPMETER_H_

#include "fieldmetergraph.h"
#include "xosview.h"


class SwapMeter : public FieldMeterGraph {
public:
  SwapMeter( XOSView *parent );
  ~SwapMeter( void );

  const char *name( void ) const { return "SwapMeter"; }
  void checkevent( void );

  void checkResources( void );
protected:

  void getswapinfo( void );
private:
};


#endif
