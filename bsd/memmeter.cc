//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  NetBSD port:  
//  Copyright (c) 1995, 1996 Brian Grayson(bgrayson@ece.utexas.edu)
//
//  This file may be distributed under terms of the GPL
//
//  The NetBSD memmeter was improved by Tom Pavel (pavel@slac.stanford.edu)
//    to provide active and inactive values, rather than just "used."

#include "memmeter.h"
#include "xosview.h"

#include <sys/param.h>
#include <sys/sysctl.h>
#include <vm/vm_param.h>
#include <stdlib.h>		//  For atoi().  BCG

//  Once we figure out how to get the buffers field, change the next line.
#define FREE_INDEX 3

MemMeter::MemMeter( XOSView *parent )
: FieldMeterDecay( parent, 4, "MEM", "ACT/INACT/SHAR/FREE" ){
//: FieldMeterDecay( parent, 3, "MEM", "USED/SHAR/FREE" ){
}

MemMeter::~MemMeter( void ){
}

void MemMeter::checkResources( void ){
  FieldMeter::checkResources();

  //  The Active and Inactive colors are new.
  setfieldcolor( 0, parent_->getResource("memActiveColor") );
  setfieldcolor( 1, parent_->getResource("memInactiveColor") );
  setfieldcolor( 2, parent_->getResource("memSharedColor") );
  setfieldcolor( FREE_INDEX, parent_->getResource("memFreeColor") );
  priority_ = atoi (parent_->getResource("memPriority"));
  dodecay_ = !strcmp (parent_->getResource("memDecay"),"True");
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

  total_ = meminfo.t_free+meminfo.t_rm;
  fields_[FREE_INDEX] = meminfo.t_free;
  fields_[2] = meminfo.t_rmshr;
  fields_[1] = meminfo.t_rm - meminfo.t_arm - meminfo.t_rmshr;
  fields_[0] = meminfo.t_arm;
//  End NetBSD-specific code...

  FieldMeter::used( (int)((100 * (total_ - fields_[FREE_INDEX])) / total_) );
}
