//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  NetBSD port:  
//  Copyright (c) 1995, 1996, 1997 by Brian Grayson (bgrayson@ece.utexas.edu)
//
//  This file was originally written by Brian Grayson for the NetBSD and
//    xosview projects.
//  The NetBSD memmeter was improved by Tom Pavel (pavel@slac.stanford.edu)
//    to provide active and inactive values, rather than just "used."
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
#include "memmeter.h"
#include "xosview.h"

#include <sys/param.h>
#include <sys/sysctl.h>
#include <vm/vm_param.h>
#ifdef XOSVIEW_FREEBSD
#warning "Quick hack for FreeBSD -- fix later."
#include <sys/vmmeter.h>
#endif
#include <stdlib.h>		//  For atoi().  BCG

CVSID("$Id$");
CVSID_DOT_H(MEMMETER_H_CVSID);
//  Once we figure out how to get the buffers field, change the next line.
#define FREE_INDEX 3

MemMeter::MemMeter( XOSView *parent )
: FieldMeterDecay( parent, 4, "MEM", "ACT/INACT/SHAR/FREE" ){
//: FieldMeterDecay( parent, 3, "MEM", "USED/SHAR/FREE" ){
}

MemMeter::~MemMeter( void ){ }

void MemMeter::checkResources( void ){
  FieldMeter::checkResources();

  //  The Active and Inactive colors are new.
  setfieldcolor( 0, parent_->getResource("memActiveColor") );
  setfieldcolor( 1, parent_->getResource("memInactiveColor") );
  setfieldcolor( 2, parent_->getResource("memSharedColor") );
  setfieldcolor( FREE_INDEX, parent_->getResource("memFreeColor") );
  priority_ = atoi (parent_->getResource("memPriority"));
  dodecay_ = !strcmp (parent_->getResource("memDecay"),"True");
  SetUsedFormat (parent_->getResource("memUsedFormat"));
}

void MemMeter::checkevent( void ){
  getmeminfo();
  drawfields();
}

void MemMeter::getmeminfo (void) {
//  Begin NetBSD-specific code...
  struct vmtotal meminfo;
  int params[] = {CTL_VM, VM_METER};
  unsigned meminfosize = sizeof (struct vmtotal);
  sysctl (params, 2, &meminfo, &meminfosize, NULL, NULL);
  /*  Note that the numbers are in terms of 4K pages.  */

  total_ = 4096*(meminfo.t_free+meminfo.t_rm);
  fields_[FREE_INDEX] = 4096*meminfo.t_free;
  fields_[2] = 4096*meminfo.t_rmshr;
  fields_[1] = 4096*(meminfo.t_rm - meminfo.t_arm - meminfo.t_rmshr);
  fields_[0] = 4096*meminfo.t_arm;
//  End NetBSD-specific code...

  setUsed (total_ - fields_[FREE_INDEX], total_);
}
