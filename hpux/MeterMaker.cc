//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "MeterMaker.h"
#include "xosview.h"

#include "cpumeter.h"
#include "memmeter.h"
#include "swapmeter.h"

#include <stdlib.h>


MeterMaker::MeterMaker(XOSView *xos){
  _xos = xos;
}

void MeterMaker::makeMeters(void){
  if (!strcmp(_xos->getResource("cpu"), "True"))
      push(new CPUMeter(_xos));
  if (!strcmp(_xos->getResource("mem"), "True"))
      push(new MemMeter(_xos));
  if (!strcmp(_xos->getResource("swap"), "True"))
      push(new SwapMeter(_xos));
}
