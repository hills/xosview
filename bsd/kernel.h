#ifndef __kernel_h__
#define __kernel_h__

//
//  NetBSD port:  
//  Copyright (c) 1995, 1996, 1997 by Brian Grayson (bgrayson@ece.utexas.edu)
//
//  This file was written by Brian Grayson for the NetBSD and xosview
//    projects.
//  This file may be distributed under terms of the GPL or of the BSD
//    license, whichever you choose.  The full license notices are
//    contained in the files COPYING.GPL and COPYING.BSD, which you
//    should have received.  If not, contact one of the xosview
//    authors for a copy.
//
// $Id$
//

#define KERNEL_H_CVSID	"$Id$"

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

void
BSDNetInit();

void
BSDGetNetInOut (long long * inbytes, long long * outbytes);

int
BSDSwapInit();

#ifdef HAVE_SWAPCTL
void
BSDGetSwapCtlInfo(int* total, int* free);
#endif

int
BSDDiskInit();

void
BSDGetDiskXFerBytes (unsigned long long * bytes);

#ifdef XOSVIEW_FREEBSD
void
FreeBSDGetBufspace(int* bfsp);
#endif


#define NUM_INTR	16

int
BSDIntrInit();

int
BSDNumInts();

void
BSDGetIntrStats (unsigned long intrCount[NUM_INTR]);

#endif
