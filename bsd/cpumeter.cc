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
#include <sys/dkstat.h>         //  For CPUSTATES #define.  BCG
#include <stdlib.h>		//  For use of atoi  BCG
#include "general.h"
#include "cpumeter.h"
#include "kernel.h"             //  For NetBSD-specific icky kvm_ code.  BCG

CVSID("$Id$");
CVSID_DOT_H(CPUMETER_H_CVSID);

CPUMeter::CPUMeter( XOSView *parent )
#if defined(XOSVIEW_FREEBSD) || defined(XOSVIEW_BSDI)
: FieldMeterGraph( parent, 5, "CPU", "USR/NICE/SYS/INT/FREE" ){
#define FREE_INDEX 4
#else
: FieldMeterGraph( parent, 4, "CPU", "USR/NICE/SYS/FREE" ){
#define FREE_INDEX 3
#endif
  for ( int i = 0 ; i < 2 ; i++ )
    for ( int j = 0 ; j < 4 ; j++ )
      cputime_[i][j] = 0;
  cpuindex_ = 0;
  //  The setting of the priority will be done in checkResources().  BCG
  dodecay_ = 0;

  BSDCPUInit();
}

CPUMeter::~CPUMeter( void ){
}

void CPUMeter::checkResources( void ){
  FieldMeterGraph::checkResources();

  setfieldcolor( 0, parent_->getResource("cpuUserColor") );
  setfieldcolor( 1, parent_->getResource("cpuNiceColor") );
  setfieldcolor( 2, parent_->getResource("cpuSystemColor") );
#if defined(XOSVIEW_FREEBSD) || defined(XOSVIEW_BSDI)
  setfieldcolor( 3, parent_->getResource("cpuInterruptColor") );
  setfieldcolor( 4, parent_->getResource("cpuFreeColor") );
#else
  setfieldcolor( 3, parent_->getResource("cpuFreeColor") );
#endif
  priority_ = atoi (parent_->getResource("cpuPriority"));
  dodecay_ = parent_->isResourceTrue("cpuDecay");
  useGraph_ = parent_->isResourceTrue("cpuGraph");
  SetUsedFormat (parent_->getResource("cpuUsedFormat"));
}

void CPUMeter::checkevent( void ){
  getcputime();
  drawfields();
}

void CPUMeter::getcputime( void ){
  total_ = 0;
  static double lastTotal = 0, lastLastTotal = -1;

  //  Begin NetBSD-specific code...  BCG
  long tempCPU[CPUSTATES];

  BSDGetCPUTimes (tempCPU);

  cputime_[cpuindex_][0] = tempCPU[0];
  cputime_[cpuindex_][1] = tempCPU[1];
#if defined(XOSVIEW_FREEBSD) || defined(XOSVIEW_BSDI)
  // FreeBSD seems at least to be filling cp_time[CP_INTR].  So, we add that
  // as another field. (pavel 25-Jan-1998)
  cputime_[cpuindex_][2] = tempCPU[2];
  cputime_[cpuindex_][3] = tempCPU[3];
  cputime_[cpuindex_][4] = tempCPU[4];
#else /* XOSVIEW_FREEBSD */
//  Merge System and Interrupt here...  NetBSD does not yet (Nov 95) support
//  the interrupt ticks, even though most code has been written to allow
//  it -- interrupt ticks are credited to system by the kernel.  BCG
  cputime_[cpuindex_][2] = tempCPU[2] + tempCPU[3];
  cputime_[cpuindex_][3] = tempCPU[4];
#endif /* XOSVIEW_FREEBSD */
  //  End NetBSD-specific code...  BCG

  
  int oldindex = (cpuindex_+1)%2;
  for ( int i = 0 ; i <= FREE_INDEX ; i++ ){
    fields_[i] = cputime_[cpuindex_][i] - cputime_[oldindex][i];
    total_ += fields_[i];
  }
  setUsed (total_ - fields_[FREE_INDEX], total_);

  /*  Check for an incorrect kernel.  The CPU tick count should
   *  change 100 times per second, so if there have been three
   *  samples where there was no change, then that's a problem.  */
  if (lastTotal == 0 && lastLastTotal == 0 && total_ == 0)
  {
    static int firstTime = 1;
    if (firstTime) {
      fprintf(stderr,
"  Warning:  the CPU tick counters are not changing.  This could
be due to running a kernel besides /netbsd (or the equivalent for FreeBSD).
  If this is the case, re-run xosview with the -N kernel-name option.
  If not, then this is a bug.  Please send a message to
bgrayson@ece.utexas.edu, in addition to any send-pr bug reports
(or in lieu of -- it ought to get fixed faster if you contact me
directly).  Thanks!\n");
      firstTime = 0;
    }
  }
  lastLastTotal = lastTotal;
  lastTotal = total_;

  cpuindex_ = (cpuindex_ + 1) % 2;
}
