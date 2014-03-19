//
//  Copyright (c) 1994, 1995, 2002, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "MeterMaker.h"
#include "xosview.h"

#include "loadmeter.h"
#include "cpumeter.h"
#include "memmeter.h"
#include "diskmeter.h"
#include "raidmeter.h"
#include "swapmeter.h"
#include "pagemeter.h"
#include "wirelessmeter.h"
#include "netmeter.h"
#include "nfsmeter.h"
#include "serialmeter.h"
#include "intmeter.h"
#include "intratemeter.h"
#include "btrymeter.h"
#if defined(__i386__) || defined(__x86_64__)
#include "coretemp.h"
#endif
#include "lmstemp.h"
#include "acpitemp.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>


MeterMaker::MeterMaker(XOSView *xos){
  _xos = xos;
}

void MeterMaker::makeMeters(void){
  // check for the load meter
  if (_xos->isResourceTrue("load"))
    push(new LoadMeter(_xos));

  // Standard meters (usually added, but users could turn them off)
  if (_xos->isResourceTrue("cpu")){
    bool single, both, all;
    unsigned int cpuCount = CPUMeter::countCPUs();

    single = (strncmp(_xos->getResource("cpuFormat"), "single", 2) == 0);
    both = (strncmp(_xos->getResource("cpuFormat"), "both", 2) == 0);
    all = (strncmp(_xos->getResource("cpuFormat"), "all", 2) == 0);

    if (strncmp(_xos->getResource("cpuFormat"), "auto", 2) == 0) {
      if (cpuCount == 1 || cpuCount > 4) {
	single = true;
      } else {
	all = true;
      }
    }

    if (single || both)
      push(new CPUMeter(_xos, CPUMeter::cpuStr(0)));

    if (all || both) {
      for (unsigned int i = 1; i <= cpuCount; i++)
	push(new CPUMeter(_xos, CPUMeter::cpuStr(i)));
    }
  }
  if (_xos->isResourceTrue("mem"))
    push(new MemMeter(_xos));
  if (_xos->isResourceTrue("disk"))
      push(new DiskMeter(_xos, atof(_xos->getResource("diskBandwidth"))));

  // check for the RAID meter
  if (_xos->isResourceTrue("RAID")){
    int RAIDCount = atoi(_xos->getResource("RAIDdevicecount"));
    for (int i = 0 ; i < RAIDCount ; i++)
      push(new RAIDMeter(_xos, i));
  }

  if (_xos->isResourceTrue("swap"))
    push(new SwapMeter(_xos));

  if (_xos->isResourceTrue("page"))
    push(new PageMeter(_xos, atof(_xos->getResource("pageBandwidth"))));

  // check for the wireless meter
  if ( _xos->isResourceTrue("wireless") ) {
    std::ifstream stats( WLFILENAME );
    if (!stats)
      std::cerr << "Wireless Meter needs Linux Wireless Extensions or cfg80211-"
                << "WEXT compatibility to work." << std::endl;
    else {
      int count = WirelessMeter::countdevices();
      for (int i = 0; i < count; i++)
        push(new WirelessMeter(_xos, i, ( count == 1 ? "WLAN" : WirelessMeter::wirelessStr(i))));
    }
  }

  // check for the net meter
  if (_xos->isResourceTrue("net"))
    push(new NetMeter(_xos, atof(_xos->getResource("netBandwidth"))));

  // check for the NFS mesters
  if (_xos->isResourceTrue("NFSDStats")){
      push(new NFSDStats(_xos));
  }
  if (_xos->isResourceTrue("NFSStats")){
      push(new NFSStats(_xos));
  }


  // check for the serial meters.
#if defined(__aarch64__) || defined (__arm__) || defined(__mc68000__) || defined(__powerpc__) || defined(__powerpc64__) || defined(__sparc__) || defined(__s390__) || defined(__s390x__)
  /* these architectures have no ioperm() */
#else
  for (int i = 0 ; i < SerialMeter::numDevices() ; i++)
      {
      bool ok ;  unsigned long val ;
      const char *res = SerialMeter::getResourceName((SerialMeter::Device)i);
      if ( !(ok = _xos->isResourceTrue(res)) )
          {
          std::istringstream is(_xos->getResource(res));
          is >> std::setbase(0) >> val;
          if (!is)
              ok = false;
          else
              ok = val & 0xFFFF;
          }

      if ( ok )
          push(new SerialMeter(_xos, (SerialMeter::Device)i));
      }
#endif

  // check for the interrupt meter
  if (_xos->isResourceTrue("interrupts")) {
    int cpuCount = IntMeter::countCPUs();
    cpuCount = cpuCount == 0 ? 1 : cpuCount;
    if (_xos->isResourceTrue("intSeparate")) {
      for (int i = 0 ; i < cpuCount ; i++)
        push(new IntMeter(_xos, i));
    }
    else
      push(new IntMeter(_xos, cpuCount-1));
  }

  // check for irqrate meter
  if (_xos->isResourceTrue("irqrate"))
    push(new IrqRateMeter(_xos));

  // check for the battery meter
  if (_xos->isResourceTrue("battery") && BtryMeter::has_source())
    push(new BtryMeter(_xos));

#if defined(__i386__) || defined(__x86_64__)
  // Check for the CPU temperature meter
  if (_xos->isResourceTrue("coretemp")) {
    char caption[32], name[8] = "CPU";
    unsigned int coreCount, pkgCount, cpu, pkg = 0;
    snprintf(caption, 32, "ACT(\260C)/HIGH/%s",
             _xos->getResourceOrUseDefault( "coretempHighest", "100" ) );
    const char *displayType = _xos->getResourceOrUseDefault( "coretempDisplayType", "separate" );

    pkgCount = CoreTemp::countCpus();
    if ( strncmp(displayType, "separate", 1) == 0 ) {
      for (pkg = 0; pkg < pkgCount; pkg++) {
        coreCount = CoreTemp::countCores(pkg);
        for (cpu = 0; cpu < coreCount; cpu++) {
          if (pkgCount > 1) {
            if (cpu == 0)  // give title only to first core of each physical cpu
              snprintf(name, 8, "CPU%d", pkg);
            else
              name[0] = '\0';
          }
          else {
            if (coreCount > 1)
              snprintf(name, 8, "CPU%d", cpu);
          }
          push(new CoreTemp(_xos, name, caption, pkg, cpu));
        }
      }
    }
    else if ( strncmp(displayType, "average", 1) == 0 ) {
      do {
        if (pkgCount > 1)
          snprintf(name, 8, "CPU%d", pkg);
        push(new CoreTemp(_xos, name, caption, pkg, -1));
      } while (++pkg < pkgCount);
    }
    else if ( strncmp(displayType, "maximum", 1) == 0 ) {
      do {
        if (pkgCount > 1)
          snprintf(name, 8, "CPU%d", pkg);
        push(new CoreTemp(_xos, name, caption, pkg, -2));
      } while (++pkg < pkgCount);
    }
    else {
      std::cerr << "Unknown value of coretempDisplayType: " << displayType << std::endl;
      std::cerr << "Supported types are: separate, average and maximum." << std::endl;
      _xos->done(1);
    }
  }
#endif

  // check for the LmsTemp meter
  if (_xos->isResourceTrue("lmstemp")){
    char caption[16], s[16];
    const char *tempfile, *highfile, *lowfile, *name, *label;
    snprintf( caption, 16, "ACT/HIGH/%s",
              _xos->getResourceOrUseDefault("lmstempHighest", "100") );
    for (int i = 1 ; ; i++) {
      snprintf(s, 16, "lmstemp%d", i);
      tempfile = _xos->getResourceOrUseDefault(s, NULL);
      if (!tempfile || !*tempfile)
        break;
      snprintf(s, 16, "lmshigh%d", i);
      highfile = _xos->getResourceOrUseDefault(s, NULL);
      snprintf(s, 16, "lmslow%d", i);
      lowfile = _xos->getResourceOrUseDefault(s, NULL);
      snprintf(s, 16, "lmsname%d", i);
      name = _xos->getResourceOrUseDefault(s, NULL);
      snprintf(s, 16, "lmstempLabel%d", i);
      label = _xos->getResourceOrUseDefault(s, "TMP");
      push(new LmsTemp(_xos, name, tempfile, highfile, lowfile, label, caption, i));
    }
  }

  // check for the ACPITemp meter
  if (_xos->isResourceTrue("acpitemp")) {
    char caption[32];
    snprintf(caption, 32, "ACT(\260C)/HIGH/%s",
             _xos->getResourceOrUseDefault("acpitempHighest", "100"));
    for (int i = 1 ; ; i++) {
      char s[16];
      snprintf(s, 16, "acpitemp%d", i);
      const char *tempfile = _xos->getResourceOrUseDefault(s, NULL);
      if (!tempfile || !*tempfile)
        break;
      snprintf(s, 16, "acpihigh%d", i);
      const char *highfile = _xos->getResourceOrUseDefault(s, NULL);
      if (!highfile || !*highfile)
        break;
      snprintf(s, 16, "acpitempLabel%d", i);
      const char *lab = _xos->getResourceOrUseDefault(s, "TMP");
      push(new ACPITemp(_xos, tempfile, highfile, lab, caption));
    }
  }
}
