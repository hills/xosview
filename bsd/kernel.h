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

#include "defines.h"

void
BSDInit();

void
SetKernelName(const char* const kernelName);

void
BSDPageInit();

#if defined(UVM)
void
BSDGetUVMPageStats(struct uvmexp* uvmp);
#else
void
BSDGetPageStats(struct vmmeter* vmp);
#endif

void
BSDCPUInit();

#if defined(XOSVIEW_NETBSD) && (__NetBSD_Version__ >= 104260000)
void
BSDGetCPUTimes(u_int64_t* timesArray);
#else
void
BSDGetCPUTimes(long* timesArray);
#endif

int
BSDNetInit();

int
BSDSwapInit();

#ifdef HAVE_SWAPCTL
void
BSDGetSwapCtlInfo(unsigned long long* total, unsigned long long* free);
#endif

int
BSDDiskInit();

void
#if __FreeBSD_version >= 500000
BSDGetDiskXFerBytes (u_int64_t *read_bytes, u_int64_t *write_bytes);
#else
BSDGetDiskXFerBytes (unsigned long long * bytes);
#endif

#ifdef XOSVIEW_FREEBSD
void
FreeBSDGetBufspace(int* bfsp);
#endif

#if defined(XOSVIEW_FREEBSD) && defined(__alpha__)
# define NUM_INTR	256
#else
# define NUM_INTR	16
#endif

int
BSDIntrInit();

int
BSDNumInts();

void
BSDGetIntrStats (unsigned long *intrCount, unsigned int *intrNbrs);

#endif
