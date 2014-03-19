//
//  Copyright (c) 1994, 1995, 2006, 2007 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "xosview.h"
#include <string.h>

int main( int argc, char *argv[] ) {
  /*  Icky.  Need to check for -name option here.  */
  char** argp = argv;
  const char* instanceName = "xosview";	// Default value.
  while (argp && *argp)
  {
    if (!strncmp(*argp, "-name", 6))
      instanceName = argp[1];
    argp++;
  }  //  instanceName will end up pointing to the last such -name option.
  XOSView xosview( instanceName, argc, argv );

  xosview.run();

  return 0;
}
