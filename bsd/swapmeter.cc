//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  NetBSD port:  
//  Copyright (c) 1995, 1996, 1997 by Brian Grayson (bgrayson@ece.utexas.edu)
//
//  This file was written by Brian Grayson for the NetBSD and xosview
//    projects.
//  This file may be distributed under terms of the GPL or of the BSD
//    copyright, whichever you choose.  The full copyright notices are
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
#include "netbsd.h"
#include <stdlib.h>		//  For atoi().  BCG

CVSID("$Id$");
CVSID_DOT_H(SWAPMETER_H_CVSID);

SwapMeter::SwapMeter( XOSView *parent )
: FieldMeterDecay( parent, 2, "SWAP", "USED/FREE" ){
  NetBSDSwapInit();	//  In netbsd.cc
  NetBSDInitSwapInfo();	//  In swapinternal.cc
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

  NetBSDGetSwapInfo (&total_int, &free_int);
  total_ = total_int;
  if ( total_ == 0 )
    total_ = 1;	/*  We don't want any division by zero, now, do we?  :)  */
  fields_[1] = free_int;
  fields_[0] = total_ - fields_[1];

  setUsed (fields_[0], total_);
}
