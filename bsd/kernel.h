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

#include <stdio.h>
#include <fcntl.h>
#include <kvm.h>
#include <nlist.h>

#include <sys/socket.h>
//  net/if.h is not protected from multiple inclusions, and apparently
//  something changed recently such that it is included via
//  sys/socket.h???  bgrayson
//#include <net/if.h>

#define KERNEL_H_CVSID	"$Id$"

void
BSDInit();

void
SetKernelName(const char* const kernelName);

void
BSDPageInit();

void
BSDGetPageStats(struct vmmeter* vmp);

void
BSDCPUInit();

void
BSDGetCPUTimes(long* timesArray);

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
