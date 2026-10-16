//
//  Copyright (c) 1994, 1995, 2002 Mike Romberg
//  Copyright (c) 2026 Jonathan Snow (Darwin support)
//
//  This file may be distributed under terms of the GPL
//

#include "MeterMaker.h"
#include "xosview.h"

#include "cpumeter.h"
#include "diskmeter.h"
#include "loadmeter.h"
#include "memmeter.h"
#include "netmeter.h"
#include "swapmeter.h"

#include <cstdlib>
#include <cstring>

MeterMaker::MeterMaker(XOSView *xos) {
  _xos = xos;
}

void MeterMaker::makeMeters(void) {
  if (_xos->isResourceTrue("load"))
    push(new LoadMeter(_xos));

  if (_xos->isResourceTrue("cpu")) {
    bool single = false;
    bool all = false;
    bool both = false;
    unsigned int cpuCount = CPUMeter::countCPUs();
    const char *format = _xos->getResource("cpuFormat");

    single = strncmp(format, "single", 2) == 0;
    all = strncmp(format, "all", 2) == 0;
    both = strncmp(format, "both", 2) == 0;
    if (strncmp(format, "auto", 2) == 0) {
      if (cpuCount == 1 || cpuCount > 4)
        single = true;
      else
        all = true;
    }

    if (single || both)
      push(new CPUMeter(_xos));
    if (all || both) {
      for (unsigned int cpu = 1; cpu <= cpuCount; ++cpu)
        push(new CPUMeter(_xos, cpu));
    }
  }

  if (_xos->isResourceTrue("mem"))
    push(new MemMeter(_xos));

  if (_xos->isResourceTrue("disk"))
    push(new DiskMeter(_xos, atof(_xos->getResource("diskBandwidth"))));

  if (_xos->isResourceTrue("swap"))
    push(new SwapMeter(_xos));

  if (_xos->isResourceTrue("net"))
    push(new NetMeter(_xos, atof(_xos->getResource("netBandwidth"))));

}
