//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//
// $Id$
//
#include "general.h"
#include "xosview.h"

CVSID("$Id$");

main( int argc, char *argv[] ) {
  XOSView xosview( argc, argv );

  xosview.run();
}
