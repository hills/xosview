#ifndef __kernel_h__
#define __kernel_h__

//
//  NetBSD port:
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

#include <sys/types.h>
#include "defines.h"

void
BSDInit();

void
SetKernelName(const char* kernelName);

int
BSDGetCPUSpeed();

void
BSDPageInit();

void
BSDGetPageStats(u_int64_t *meminfo, u_int64_t *pageinfo);

void
BSDCPUInit();

void
#if defined(XOSVIEW_NETBSD) || defined(XOSVIEW_DFBSD)
BSDGetCPUTimes(u_int64_t *timesArray);
#else
BSDGetCPUTimes(long *timesArray);
#endif

int
BSDNetInit();

void
BSDGetNetInOut(unsigned long long *inbytes, unsigned long long *outbytes, const char *netIface, bool ignored);

int
BSDSwapInit();

void
BSDGetSwapInfo(u_int64_t* total, u_int64_t* free);

int
BSDDiskInit();

u_int64_t
BSDGetDiskXFerBytes(u_int64_t *read_bytes, u_int64_t *write_bytes);

int
BSDIntrInit();

int
BSDNumInts();

void
BSDGetIntrStats(unsigned long *intrCount, unsigned int *intrNbrs);

int
BSDCountCpus(void);

#if defined(__i386__) || defined(__x86_64)
unsigned int
BSDGetCPUTemperature(float *temps, float *tjmax);
#endif

void
BSDGetSensor(const char *name, const char *valname, float *value, char *unit = NULL);

bool
BSDHasBattery();

void
BSDGetBatteryInfo(int *remaining, unsigned int *state);


#endif
