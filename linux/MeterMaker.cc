//
//  Copyright (c) 1994, 1995, 2002, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
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
#include "wirelessmeter.h"
#include <fstream>
#include "diskmeter.h"
#include "raidmeter.h"
#include "lmstemp.h"
#include "acpitemp.h"
#include "coretemp.h"
#include "nfsmeter.h"

#include <stdlib.h>

#include <sstream>
#include <iomanip>

using namespace std;

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

  // check for the wireless meter
static const char WLFILENAME[] = "/proc/net/wireless";
ifstream stats( WLFILENAME );
if ( stats ) {
  if (_xos->isResourceTrue("wireless")){
    int wirelessCount = WirelessMeter::countdevices();
    int start = (wirelessCount == 0) ? 0 : 1;
	if (wirelessCount != 0) {
    for (int i = start ; i <= wirelessCount ; i++)
      push(new WirelessMeter(_xos, i, WirelessMeter::wirelessStr(i)));
  } } }

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
#if defined (__arm__) || defined(__mc68000__) || defined(__powerpc__) || defined(__sparc__) || defined(__s390__) || defined(__s390x__)
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

  // check for the battery meter
  if (_xos->isResourceTrue("battery") && BtryMeter::has_source())
    push(new BtryMeter(_xos));

  // Check for the Intel Core temperature meter
  if (_xos->isResourceTrue("coretemp")) {
    char caption[25];
    snprintf(caption, 25, "ACT/HIGH/%s", _xos->getResourceOrUseDefault( "coretempHighest", "100" ) );
    const char *displayType = _xos->getResourceOrUseDefault( "coretempDisplayType", "separate" );
    for (uint i = 1 ; ; i++) {
      char s[80];
      snprintf(s, 80, "coretemp%dPackage", i);
      const char *dummy = _xos->getResourceOrUseDefault(s, NULL);
      if (i > 1 && ( !dummy || !*dummy ))
        break;
      int pkg = ( dummy ? atoi(dummy) : 0 );
      snprintf(s, 80, "coretemp%dDisplayType", i);
      displayType = _xos->getResourceOrUseDefault( s, displayType );
      if (strncmp(displayType, "separate", 1) == 0) {
        unsigned int cpuCount = CoreTemp::countCpus(pkg);
        char name[10];
        for (uint cpu = 0; cpu < cpuCount; cpu++) {
          sprintf(name, "CPU%d", cpu);
          push(new CoreTemp(_xos, name, caption, pkg, cpu));
        }
      }
      else if (strncmp(displayType, "average", 1) == 0)
        push(new CoreTemp(_xos, "CPU", caption, pkg, -1));
      else if (strncmp(displayType, "maximum", 1) == 0)
        push(new CoreTemp(_xos, "CPU", caption, pkg, -2));
      else {
        std::cerr << "Unknown value of coretempDisplayType: " << displayType << std::endl;
        std::cerr << "Supported types are: separate, average and maximum." << std::endl;
        _xos->done(1);
      }
    }
  }

  // check for the LmsTemp meter
  if (_xos->isResourceTrue("lmstemp")){
    char caption[80];
    snprintf(caption, 80, "ACT/HIGH/%s",
      _xos->getResourceOrUseDefault("lmstempHighest", "100"));
    for (int i = 1 ; ; i++) {
      char s[20];
      snprintf(s, 20, "lmstemp%d", i);
      const char *tempfile = _xos->getResourceOrUseDefault(s, NULL);
      if (!tempfile || !*tempfile)
        break;
      snprintf(s, 20, "lmshigh%d", i);
      const char *highfile = _xos->getResourceOrUseDefault(s, NULL);
      snprintf(s, 20, "lmstempLabel%d", i);
      const char *lab = _xos->getResourceOrUseDefault(s, "TMP");
      push(new LmsTemp(_xos, tempfile, highfile, lab, caption));
    }
  }

  // check for the ACPITemp meter
  if (_xos->isResourceTrue("acpitemp")) {
    char caption[80];
    snprintf(caption, 80, "ACT/HIGH/%s", _xos->getResourceOrUseDefault("acpitempHighest", "100"));
    for (int i = 1 ; ; i++) {
      char s[20];
      snprintf(s, 20, "acpitemp%d", i);
      const char *tempfile = _xos->getResourceOrUseDefault(s, NULL);
      if (!tempfile || !*tempfile)
        break;
      snprintf(s, 20, "acpihigh%d", i);
      const char *highfile = _xos->getResourceOrUseDefault(s, NULL);
      if (!highfile || !*highfile)
        break;
      snprintf(s, 20, "acpitempLabel%d", i);
      const char *lab = _xos->getResourceOrUseDefault(s, "TMP");
      push(new ACPITemp(_xos, tempfile, highfile, lab, caption));
    }
  }
}
