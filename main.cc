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
  /*  Icky.  Need to check for -name option here.  */
  char** argp = argv;
  char* instanceName = "xosview";	// Default value.
  while (argp && *argp)
  {
    if (!strncmp(*argp, "-name", 6))
      instanceName = argp[1];
    argp++;
  }  //  instanceName will end up pointing to the last such -name option.
  XOSView xosview( instanceName, argc, argv );

  xosview.run();
}
