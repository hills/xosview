//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//  Most of this code was written by Brian Grayson
//  (bgrayson@pine.ece.utexas.edu).

#include "MeterMaker.h"
#include "xosview.h"

#include "cpumeter.h"
#include "memmeter.h"
#include "swapmeter.h"
#include "netmeter.h"
#include "loadmeter.h"
//#include "intmeter.h"  //  These two are not yet supported under NetBSD.
//#include "serialmeter.h"


#include <stdlib.h>

MeterMaker::MeterMaker(XOSView *xos){
  _xos = xos;

}

void MeterMaker::makeMeters(void){
  //  check for the loadmeter
  if (!strcmp(_xos->getResource("load"), "True"))
    push(new LoadMeter(_xos));

  // Standard meters (usually added, but users could turn them off)
  if (!strcmp(_xos->getResource("cpu"), "True"))
    push(new CPUMeter(_xos));
  if (!strcmp(_xos->getResource("mem"), "True"))
    push(new MemMeter(_xos));
  if (!strcmp(_xos->getResource("swap"), "True"))
    push(new SwapMeter(_xos));

  // check for the net meter
  //  FIXME  This check should be changed to check for the "net" resource.  BCG
  if (strcmp(_xos->getResource("network"), "0"))
  {
    push(new NetMeter(_xos, atof(_xos->getResource("network"))));
  }

  //  The serial meters and the interrupt meter are not yet
  //  available for NetBSD.  BCG
}
