//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//  Most of this code was written by Brian Grayson
//  (bgrayson@pine.ece.utexas.edu).
//
// $Id$
//
#include <stdlib.h>
#include "general.h"
#include "MeterMaker.h"
#include "xosview.h"
#include "cpumeter.h"
#include "memmeter.h"
#include "swapmeter.h"
#include "netmeter.h"
#include "loadmeter.h"
#include "diskmeter.h"
//#include "intmeter.h"  //  These two are not yet supported under NetBSD.
//#include "serialmeter.h"

CVSID_DOT_H2(PLLIST_H_CVSID);
CVSID_DOT_H(METERMAKER_H_CVSID);
CVSID("$Id$");

MeterMaker::MeterMaker(XOSView *xos){
  _xos = xos;
}

void MeterMaker::makeMeters(void){
  //  check for the loadmeter
  if (_xos->isResourceTrue("load"))
    push(new LoadMeter(_xos));

  // Standard meters (usually added, but users could turn them off)
  if (_xos->isResourceTrue("cpu"))
    push(new CPUMeter(_xos));
  if (_xos->isResourceTrue("mem"))
    push(new MemMeter(_xos));
  if (_xos->isResourceTrue("swap"))
    push(new SwapMeter(_xos));

  // check for the net meter
  if (_xos->isResourceTrue("net"))
    push(new NetMeter(_xos, atof(_xos->getResource("netBandwidth"))));

  if (_xos->isResourceTrue("disk"))
    push(new DiskMeter (_xos, atof(_xos->getResource("diskBandwidth"))));

  //  The serial meters and the interrupt meter are not yet
  //  available for NetBSD.  BCG
}
