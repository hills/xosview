//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  NetBSD port:  
//  Copyright (c) 1995, 1996, 1997 by Brian Grayson (bgrayson@ece.utexas.edu)
//
//  This file was originally written by Brian Grayson for the NetBSD and
//    xosview projects.
//
//  This file may be distributed under terms of the GPL or of the BSD
//    license, whichever you choose.  The full license notices are
//    contained in the files COPYING.GPL and COPYING.BSD, which you
//    should have received.  If not, contact one of the xosview
//    authors for a copy.
//
// $Id$
//
#include "general.h"
#include "pagemeter.h"
#include "xosview.h"

#include <sys/param.h>
#include <sys/sysctl.h>
#include <vm/vm_param.h>
#include <stdlib.h>		//  For atoi().  BCG
#include "kernel.h"		//  For NetBSD Page functions.

CVSID("$Id$");
CVSID_DOT_H(PAGEMETER_H_CVSID);

PageMeter::PageMeter( XOSView *parent, double total )
: FieldMeterDecay( parent, 3, "PAGE", "IN/OUT/IDLE" ){
  total_ = total;
  NetBSDPageInit();
  NetBSDGetPageStats(&prev_);
}

PageMeter::~PageMeter( void ){ }

void PageMeter::checkResources( void ){
  FieldMeter::checkResources();

  //  The Active and Inactive colors are new.
  setfieldcolor( 0, parent_->getResource("pageInColor") );
  setfieldcolor( 1, parent_->getResource("pageOutColor") );
  setfieldcolor( 2, parent_->getResource("pageIdleColor") );
  priority_ = atoi (parent_->getResource("pagePriority"));
  dodecay_ = !strcmp (parent_->getResource("pageDecay"),"True");
  SetUsedFormat (parent_->getResource("pageUsedFormat"));
}

void PageMeter::checkevent( void ){
  getpageinfo();
  drawfields();
}

void PageMeter::getpageinfo (void) {
//  Begin NetBSD-specific code...
  struct vmmeter vm;

  NetBSDGetPageStats(&vm);
#ifdef XOSVIEW_FREEBSD
#warning "FreeBSD hack"
  fields_[0] = vm.v_vnodein - prev_.v_vnodein;
  fields_[1] = vm.v_vnodeout - prev_.v_vnodeout;
#else
  fields_[0] = vm.v_pageins - prev_.v_pageins;
  fields_[1] = vm.v_pageouts - prev_.v_pageouts;
#endif
  prev_ = vm;
//  End NetBSD-specific code...
  if (total_ < fields_[0] + fields_[1])
    total_ = fields_[0] + fields_[1];

  fields_[2] = total_ - fields_[0] - fields_[1];
  setUsed (total_ - fields_[2], total_);
}
