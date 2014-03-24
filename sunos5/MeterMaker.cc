//  
//  Initial port performed by Greg Onufer (exodus@cheers.bungi.com)
//
#include "MeterMaker.h"
#include "xosview.h"

#include "cpumeter.h"
#include "memmeter.h"
#include "swapmeter.h"
#include "loadmeter.h"
#include "pagemeter.h"
#include "diskmeter.h"
#include "netmeter.h"
#include <string.h>
#include <stdlib.h>


MeterMaker::MeterMaker(XOSView *xos)
{
  _xos = xos;
}

void MeterMaker::makeMeters(void)
{
  kstat_ctl_t *kc;

  kc = kstat_open();
  if (kc == NULL)
    return;

  if (_xos->isResourceTrue("load"))
    push(new LoadMeter(_xos, kc));

  // Standard meters (usually added, but users could turn them off)
  if (_xos->isResourceTrue("cpu")) {
    bool single, both, all;
    int cpuCount = CPUMeter::countCPUs(kc);

    single = (strncmp(_xos->getResource("cpuFormat"), "single", 2) == 0);
    both = (strncmp(_xos->getResource("cpuFormat"), "both", 2) == 0);
    all = (strncmp(_xos->getResource("cpuFormat"), "all", 2) == 0);

    if (strncmp(_xos->getResource("cpuFormat"), "auto", 2) == 0) {
      if (cpuCount == 1 || cpuCount > 4)
        single = true;
      else
        all = true;
    }

    if (single || both)
      push(new CPUMeter(_xos, kc, 0));

    if (all || both) {
      for (int i = 1; i <= cpuCount; i++)
        push(new CPUMeter(_xos, kc, i));
    }
  }

  if (_xos->isResourceTrue("mem"))
    push(new MemMeter(_xos, kc));

  if (_xos->isResourceTrue("disk"))
    push(new DiskMeter(_xos, kc, atof(_xos->getResource("diskBandwidth"))));

  if (_xos->isResourceTrue("swap"))
    push(new SwapMeter(_xos, kc));

  if (_xos->isResourceTrue("page"))
    push(new PageMeter(_xos, kc,
		       atof(_xos->getResource("pageBandwidth"))));

  if (_xos->isResourceTrue("net"))
    push(new NetMeter(_xos, kc, atof(_xos->getResource("netBandwidth"))));
}
