//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
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
#include "general.h"
#include "swapmeter.h"
#include "xosview.h"

#include "swapinternal.h"
#include "kernel.h"
#include <err.h>			//  For warnx.  BCG
#include <stdlib.h>		//  For atoi().  BCG

CVSID("$Id$");
CVSID_DOT_H(SWAPMETER_H_CVSID);

static int doSwap = 1;

SwapMeter::SwapMeter( XOSView *parent )
: FieldMeterDecay( parent, 2, "SWAP", "USED/FREE" ){
#ifdef HAVE_SWAPCTL
  useSwapCtl = 0;
#endif
  BSDSwapInit();	//  In kernel.cc
  if (!BSDInitSwapInfo())
  {
#ifdef HAVE_SWAPCTL
    //  Set up to use new swap code instead.
    useSwapCtl = 1;
#else
  warnx("The kernel does not seem to have the symbols needed for the\n"
  "SwapMeter.  If your kernel is newer than 1.2F, but xosview was\n"
  "compiled on an older system, then recompile xosview on a 1.2G or later\n"
  "system and it will automatically adjust to using swapctl() when needed.\n"
  "\nIf this is not the case (kernel before version 1.2G), make sure the\n"
  "running kernel is /netbsd, or use the -N flag for xosview to specify\n"
  "an alternate kernel file.\n"
  "\nThe SwapMeter has been disabled.\n");
  doSwap = 0;
  disableMeter();
#endif
  }
}

SwapMeter::~SwapMeter( void ){
}

void SwapMeter::checkResources( void ){
  FieldMeter::checkResources();

  setfieldcolor( 0, parent_->getResource("swapUsedColor") );
  setfieldcolor( 1, parent_->getResource("swapFreeColor") );
  priority_ = atoi (parent_->getResource("swapPriority"));
  dodecay_ = !strcmp (parent_->getResource("swapDecay"),"True");
  SetUsedFormat (parent_->getResource("swapUsedFormat"));
}

void SwapMeter::checkevent( void ){
  getswapinfo();
  drawfields();
}

void SwapMeter::getswapinfo( void ){
  int total_int, free_int;

  if (doSwap) {
#ifdef HAVE_SWAPCTL
    if (useSwapCtl)
      BSDGetSwapCtlInfo(&total_int, &free_int);
    else
#endif
      BSDGetSwapInfo (&total_int, &free_int);
  }
  else {
    total_int = 1;	/*  So the meter looks blank.  */
    free_int = 1;
  }

  total_ = total_int;
  if ( total_ == 0 )
    total_ = 1;	/*  We don't want any division by zero, now, do we?  :)  */
  fields_[1] = free_int;
  fields_[0] = total_ - fields_[1];

  setUsed (fields_[0], total_);
}
