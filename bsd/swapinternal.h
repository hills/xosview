//  Copyright (c) 1995 by Brian Grayson (bgrayson@pine.ece.utexas.edu)
//  This code is borrowed HEAVILY from the systat source code in the
//  NetBSD distribution.
//  

//  Header file for the swap internal/NetBSD-specific code.

int
NetBSDInitSwapInfo();

void
NetBSDGetSwapInfo(int* total, int* free);
