//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _MEMMETER_H_
#define _MEMMETER_H_

#include "fieldmetergraph.h"
#include "xosview.h"
#include <string.h>


class MemMeter : public FieldMeterGraph {
public:
  MemMeter( XOSView *parent );

  const char *name( void ) const { return "MemMeter"; }
  void checkevent( void );
  void checkResources( void );

private:
  void getstats( void );
};


#endif
