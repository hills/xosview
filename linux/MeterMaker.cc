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
#include "btrymeter.h"

#include <stdlib.h>


MeterMaker::MeterMaker(XOSView *xos){
  _xos = xos;
}

void MeterMaker::makeMeters(void){
  // check for the load meter
  if (_xos->isResourceTrue("load"))
    push(new LoadMeter(_xos));

  // Standard meters (usually added, but users could turn them off)
  if (_xos->isResourceTrue("cpu")){
    int cpuCount = CPUMeter::countCPUs();
    int start = (cpuCount == 0) ? 0 : 1;
    for (int i = start ; i <= cpuCount ; i++)
      push(new CPUMeter(_xos, CPUMeter::cpuStr(i)));
  }
  if (_xos->isResourceTrue("mem"))
    push(new MemMeter(_xos));
  if (_xos->isResourceTrue("swap"))
    push(new SwapMeter(_xos));
  
  if (_xos->isResourceTrue("page"))
    push(new PageMeter(_xos, atof(_xos->getResource("pageBandwidth"))));

  // check for the net meter
  if (_xos->isResourceTrue("net"))
    push(new NetMeter(_xos, atof(_xos->getResource("netBandwidth"))));

  // check for the serial meters.
  for (int i = 0 ; i < SerialMeter::numDevices() ; i++)
    if (strcmp(_xos->getResource(
      SerialMeter::getResourceName((SerialMeter::Device)i)), "False"))
      push(new SerialMeter(_xos, (SerialMeter::Device)i));

  // check for the interrupt meter
  if (_xos->isResourceTrue("interrupts")) {
    int cpuCount = IntMeter::countCPUs();
    cpuCount = cpuCount == 0 ? 1 : cpuCount;
    for (int i = 0 ; i < cpuCount ; i++)
      push(new IntMeter(_xos, i));
  }

  if (_xos->isResourceTrue("battery"))
    push(new BtryMeter(_xos));
}
