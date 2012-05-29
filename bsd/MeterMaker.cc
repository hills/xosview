//
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//  Copyright (c) 1995, 1996, 1997-2002 by Brian Grayson (bgrayson@netbsd.org)
//
//  This file was written by Brian Grayson for the NetBSD and xosview
//    projects.
//  This file may be distributed under terms of the GPL or of the BSD
//    license, whichever you choose.  The full license notices are
//    contained in the files COPYING.GPL and COPYING.BSD, which you
//    should have received.  If not, contact one of the xosview
//    authors for a copy.
//

#include "defines.h"
#include <stdlib.h>
#include "MeterMaker.h"
#include "xosview.h"
#ifdef HAVE_BATTERY_METER
#include "btrymeter.h"
#endif
#include "cpumeter.h"
#include "memmeter.h"
#include "swapmeter.h"
#include "netmeter.h"
#include "loadmeter.h"
#include "diskmeter.h"
#include "pagemeter.h"
#include "intmeter.h"
#include "intratemeter.h"
#include "coretemp.h"
//  This one is not yet supported under *BSD.
//#include "serialmeter.h"

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

  if (_xos->isResourceTrue("page"))
    push(new PageMeter (_xos, atof(_xos->getResource("pageBandwidth"))));

  // check for the net meter
  if (_xos->isResourceTrue("net"))
    push(new NetMeter(_xos, atof(_xos->getResource("netBandwidth"))));

  if (_xos->isResourceTrue("disk"))
    push(new DiskMeter (_xos, atof(_xos->getResource("diskBandwidth"))));

  if (_xos->isResourceTrue("interrupts"))
      push(new IntMeter(_xos));

  if (_xos->isResourceTrue("irqrate"))
  {
      push(new IrqRateMeter(_xos));
  }

  if (_xos->isResourceTrue("coretemp"))
  {
    char caption[25];
    snprintf(caption, 25, "TEMPERATURE (C)/%s", _xos->getResourceOrUseDefault( "coretempHighest", "100" ) );
    std::string displayType = _xos->getResourceOrUseDefault("coretempDisplayType", "separate");
    if (displayType == "separate") {
      char name[5];
      for (int i=0; i<CoreTemp::countCpus(); i++) {
        sprintf(name, "TMP%d", i);
        push(new CoreTemp(_xos, name, caption, i));
      }
    }
    else if (displayType == "average")
      push(new CoreTemp(_xos, "TEMP", caption, -1));
    else if (displayType == "maximum")
      push(new CoreTemp(_xos, "TEMP", caption, -2));
    else {
      std::cerr << "Unknown value of coretempDisplayType: " << displayType << std::endl;
      _xos->done(1);
    }
  }

#ifdef HAVE_BATTERY_METER
  //  NOTE:  Only xosview for NetBSD (out of all the BSDs) currently
  //  supports the Battery Meter.
  //  This one is done in its own file, not kernel.cc
  if (_xos->isResourceTrue("battery"))
      push(new BtryMeter(_xos));
#endif

  //  The serial meters are not yet available for the BSDs.  BCG
}
