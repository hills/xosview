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
#include "kernel.h"

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/vmmeter.h>
#include <stdlib.h>		//  For atoi().  BCG

CVSID("$Id$");
CVSID_DOT_H(MEMMETER_H_CVSID);

MemMeter::MemMeter( XOSView *parent )
#ifdef  XOSVIEW_FREEBSD
: FieldMeterDecay( parent, 5, "MEM", "ACT/INACT/WRD/BUF/FR" ) {
#define FREE_INDEX 4
#else
  //  Once we figure out how to get the buffers field for NetBSD,
  //  change the next line.
: FieldMeterDecay( parent, 4, "MEM", "ACT/INACT/WIRE/FREE" ){
#define FREE_INDEX 3
#endif
}

MemMeter::~MemMeter( void ){ }

void MemMeter::checkResources( void ){
  FieldMeter::checkResources();

  //  The Active and Inactive colors are new.
  setfieldcolor( 0, parent_->getResource("memActiveColor") );
  setfieldcolor( 1, parent_->getResource("memInactiveColor") );
  setfieldcolor( 2, parent_->getResource("memCacheColor") );
			// use the Linux Cache color (red) for wired mem
#ifdef XOSVIEW_FREEBSD
  setfieldcolor( 3, parent_->getResource("memBufferColor") );
#endif
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
//  Begin *BSD-specific code...

#if 0 /* Old code */
  struct vmtotal meminfo;
  int params[] = {CTL_VM, VM_METER};
  size_t meminfosize = sizeof (struct vmtotal);
  sysctl (params, 2, &meminfo, &meminfosize, NULL, NULL);
  /*  Note that the numbers are in terms of 4K pages.  */

  total_ = 4096*(meminfo.t_free+meminfo.t_rm);
  fields_[FREE_INDEX] = 4096*meminfo.t_free;
  fields_[2] = 4096*meminfo.t_rmshr;
  fields_[1] = 4096*(meminfo.t_rm - meminfo.t_arm - meminfo.t_rmshr);
  fields_[0] = 4096*meminfo.t_arm;
#endif

  // New code.  Use the cnt structure:
  struct vmmeter kvm_cnt;
  NetBSDGetPageStats (&kvm_cnt);

  /*  Note that the numbers are in terms of pages,
      and we want fields_ in bytes.  */
  int pgsize = kvm_cnt.v_page_size;
  fields_[0] = kvm_cnt.v_active_count * pgsize;
  fields_[1] = kvm_cnt.v_inactive_count * pgsize;
  fields_[2] = kvm_cnt.v_wire_count * pgsize;
#ifdef XOSVIEW_FREEBSD
  /* I believe v_cache_count is the right answer here, rather than the
     bufspace variable.  I think that bufspace is a subset of "cache" space
     that is used for filesystem io, and I think that "cache" pages also
     include text pages that are inactive.  Empirically, I find I need to use
     the "cache" pages, though, to get the numbers to add up to the right
     total memory.  Perhaps a better representation of what is going on would
     be to subtract bufspace from v_cache_count, and add that difference to
     v_inactive_count.  (pavel 21-Jan-1998) */
  fields_[3] = kvm_cnt.v_cache_count * pgsize;
  fields_[FREE_INDEX] = kvm_cnt.v_free_count * pgsize;
  total_ = kvm_cnt.v_page_count * pgsize;
#else
  fields_[FREE_INDEX] = kvm_cnt.v_free_count * pgsize;
  total_ = fields_[0] + fields_[1] + fields_[2] + fields_[FREE_INDEX];
#endif

//  End *BSD-specific code...

  setUsed (total_ - fields_[FREE_INDEX], total_);
}
