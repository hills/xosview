//
//  Copyright (c) 2014 by Tomi Tapper (tomi.o.tapper@jyu.fi)
//
//  Based on bsd/intratemeter.* by
//    Copyright (c) 1999 by Brian Grayson (bgrayson@netbsd.org)
//  and on linux/intmeter.* by
//    Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _IRQRATEMETER_H_
#define _IRQRATEMETER_H_

#include "fieldmetergraph.h"
#include "xosview.h"


class IrqRateMeter : public FieldMeterGraph {
public:
  IrqRateMeter( XOSView *parent );
  ~IrqRateMeter( void );

  const char *name( void ) const { return "IrqRateMeter"; }
  void checkevent( void );
  void checkResources( void );

protected:
  void getinfo( void );

private:
  unsigned long long _lastirqs;
  unsigned int _cpucount;
};


#endif
