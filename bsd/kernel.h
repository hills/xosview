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
NetBSDInit();

void
SetKernelName(const char* const kernelName);

void
NetBSDPageInit();

void
NetBSDGetPageStats(struct vmmeter* vmp);

void
NetBSDCPUInit();

void
NetBSDGetCPUTimes(long* timesArray);

void
NetBSDNetInit();

void
NetBSDGetNetInOut (long long * inbytes, long long * outbytes);

int
NetBSDSwapInit();

#ifdef HAVE_SWAPCTL
void
NetBSDGetSwapCtlInfo(int* total, int* free);
#endif

int
NetBSDDiskInit();

void
NetBSDGetDiskXFerBytes (unsigned long long * bytes);

#ifdef XOSVIEW_FREEBSD
void
FreeBSDGetBufspace(int* bfsp);
#endif

#endif
