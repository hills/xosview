//  Copyright (c) 1995, 1996, 1997-2002 by Brian Grayson (bgrayson@netbsd.org)
//  This code is borrowed HEAVILY from the systat source code in the
//  NetBSD distribution.

//  Header file for the swap internal/NetBSD-specific code.

#if !(defined(XOSVIEW_OPENBSD) && defined(HAVE_SWAPCTL))
// For OpenBSD with swapctl, don't provide the old method at all.
int
BSDInitSwapInfo();

void
BSDGetSwapInfo(int64_t* total, int64_t* free);
#endif

void
BSDGetSwapCtlInfo(int64_t* total, int64_t* free);
