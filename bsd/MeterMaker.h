//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//
// $Id$
//
#ifndef _MeterMaker_h
#define _MeterMaker_h

#define METERMAKER_H_CVSID "$Id$"

#include "pllist.h"

class Meter;
class XOSView;

class MeterMaker : public PLList<Meter *> {
public:
  MeterMaker(XOSView *xos);
  void makeMeters(void);
private:
  XOSView *_xos;
};

#endif
