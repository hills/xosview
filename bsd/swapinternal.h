//  Copyright (c) 1995, 1996, 1997 by Brian Grayson (bgrayson@ece.utexas.edu)
//  This code is borrowed HEAVILY from the systat source code in the
//  NetBSD distribution.
//  
//
// $Id$
//

//  Header file for the swap internal/NetBSD-specific code.

int
BSDInitSwapInfo();

void
BSDGetSwapInfo(int* total, int* free);

void
BSDGetSwapCtlInfo(int* total, int* free);
