//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//
// $Id$
//
#include "MeterMaker.h"
#include "xosview.h"

#include "cpumeter.h"
#include "memmeter.h"
#include "swapmeter.h"
#include "pagemeter.h"
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
  
  if (!strcmp(_xos->getResource("page"), "True"))
      push(new PageMeter(_xos, atof(_xos->getResource("pageBandwidth"))));

  // check for the net meter
  if (!strcmp(_xos->getResource("net"), "True"))
    push(new NetMeter(_xos, atof(_xos->getResource("netBandwidth"))));

  // check for the serial meters.
//   if (!strcmp(_xos->getResource("serial1"), "True"))
//     push(new SerialMeter(_xos, SerialMeter::S0, "cua0"));
//   if (!strcmp(_xos->getResource("serial2"), "True"))
//     push(new SerialMeter(_xos, SerialMeter::S1, "cua1"));
//   if (!strcmp(_xos->getResource("serial3"), "True"))
//     push(new SerialMeter(_xos, SerialMeter::S2, "cua2"));
//   if (!strcmp(_xos->getResource("serial4"), "True"))
//     push(new SerialMeter(_xos, SerialMeter::S3, "cua3"));

  if (!strcmp(_xos->getResource("serial0"), "True"))
      push(new SerialMeter(_xos, SerialMeter::S0, "ttyS0"));
  if (!strcmp(_xos->getResource("serial1"), "True"))
      push(new SerialMeter(_xos, SerialMeter::S1, "ttyS1"));
  if (!strcmp(_xos->getResource("serial2"), "True"))
      push(new SerialMeter(_xos, SerialMeter::S2, "ttyS2"));
  if (!strcmp(_xos->getResource("serial3"), "True"))
      push(new SerialMeter(_xos, SerialMeter::S3, "ttyS3"));
  if (!strcmp(_xos->getResource("serial4"), "True"))
      push(new SerialMeter(_xos, SerialMeter::S4, "ttyS4"));
  if (!strcmp(_xos->getResource("serial5"), "True"))
      push(new SerialMeter(_xos, SerialMeter::S5, "ttyS5"));
  if (!strcmp(_xos->getResource("serial6"), "True"))
      push(new SerialMeter(_xos, SerialMeter::S6, "ttyS6"));
  if (!strcmp(_xos->getResource("serial7"), "True"))
      push(new SerialMeter(_xos, SerialMeter::S7, "ttyS7"));
  if (!strcmp(_xos->getResource("serial8"), "True"))
      push(new SerialMeter(_xos, SerialMeter::S8, "ttyS8"));
  if (!strcmp(_xos->getResource("serial9"), "True"))
      push(new SerialMeter(_xos, SerialMeter::S0, "ttyS9"));

  // check for the interrupt meter
  if (!strcmp(_xos->getResource("interrupts"), "True"))
    push(new IntMeter(_xos));
}
