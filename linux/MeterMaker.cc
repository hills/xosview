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
#include "netmeter.h"
#include "intmeter.h"
#include "serialmeter.h"
#include "loadmeter.h"

#include <stdlib.h>


MeterMaker::MeterMaker(XOSView *xos){
  _xos = xos;
}

void MeterMaker::makeMeters(void){
  // check for the load meter
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
    push(new NetMeter(_xos, atof(_xos->getResource("network"))));

  // check for the serial meters.
  if (!strcmp(_xos->getResource("serial1"), "True"))
    push(new SerialMeter(_xos, SerialMeter::S0, "cua0"));
  if (!strcmp(_xos->getResource("serial2"), "True"))
    push(new SerialMeter(_xos, SerialMeter::S1, "cua1"));
  if (!strcmp(_xos->getResource("serial3"), "True"))
    push(new SerialMeter(_xos, SerialMeter::S2, "cua2"));
  if (!strcmp(_xos->getResource("serial4"), "True"))
    push(new SerialMeter(_xos, SerialMeter::S3, "cua3"));

  // check for the interrupt meter
  if (!strcmp(_xos->getResource("interrupts"), "True"))
    push(new IntMeter(_xos));
}
