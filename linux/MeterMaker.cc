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
#include "diskmeter.h"
#include "raidmeter.h"

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

  if (_xos->isResourceTrue("disk"))
      push(new DiskMeter(_xos, atof(_xos->getResource("diskBandwidth"))));

  // check for the serial meters.
#ifndef __mc68000__
  for (int i = 0 ; i < SerialMeter::numDevices() ; i++)
    if (_xos->isResourceTrue(SerialMeter::getResourceName(
      (SerialMeter::Device)i)))
        push(new SerialMeter(_xos, (SerialMeter::Device)i));
#endif

  // check for the interrupt meter
  if (_xos->isResourceTrue("interrupts")) {
    int cpuCount = IntMeter::countCPUs();
    cpuCount = cpuCount == 0 ? 1 : cpuCount;
    for (int i = 0 ; i < cpuCount ; i++)
      push(new IntMeter(_xos, i));
  }

  // check for the battery meter
  if (_xos->isResourceTrue("battery"))
    push(new BtryMeter(_xos));

  // check for the RAID meter
  if (_xos->isResourceTrue("RAID")){
    int RAIDCount = atoi(_xos->getResource("RAIDdevicecount"));
    for (int i = 0 ; i < RAIDCount ; i++)
      push(new RAIDMeter(_xos, i));
  }
}
