//  
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
#include "diskmeter.h"
#include "xosview.h"
#include <sys/dkstat.h>
#include <err.h>        //  For err() and warn(), etc.  BCG
#include "netbsd.h"     //  For NetBSD-specific icky (but handy) kvm_ code.  BCG
#include <stdlib.h>	//  For use of atoi  BCG

CVSID("$Id$");
CVSID_DOT_H(DISKMETER_H_CVSID);

DiskMeter::DiskMeter( XOSView *parent, float max )
: FieldMeterDecay( parent, 2, "DISK", "XFER/IDLE" ){
  //  The setting of the priority will be done in checkResources().  BCG
  dodecay_ = 0;

  kernelHasStats_ = NetBSDDiskInit();
  if (!kernelHasStats_)
  {
    warnx(
  "The kernel does not seem to have the symbols needed for the DiskMeter.\n"
  "  (A 1.2 kernel or above is probably needed)\n"
  "The DiskMeter has been disabled.\n");
    disableMeter ();
  }
  prevBytes = 0;
  maxBandwidth_ = max;
  total_ = max;
  getstats();
    /*  Since at the first call, it will look like we transferred a
	gazillion bytes, let's reset total_ again and do another
	call.  This will force total_ to be something reasonable.  */
  total_ = max;
  IntervalTimerStart();
  getstats();
    /*  By doing this check, we eliminate the startup situation where
	all fields are 0, and total is 0, leading to nothing being drawn
	on the meter.  So, make it look like nothing was transferred,
	out of a total of 1 bytes.  */
  if (total_ < 1.0)
  {
    total_ = (float)1.0;
    fields_[1] = total_;
  }
}

DiskMeter::~DiskMeter( void ){
}

void DiskMeter::checkResources( void ){
  FieldMeter::checkResources();

  if (kernelHasStats_) {
    setfieldcolor( 0, parent_->getResource("diskUsedColor") );
    setfieldcolor( 1, parent_->getResource("diskIdleColor") );
    priority_ = atoi (parent_->getResource("diskPriority"));
    dodecay_ = !strcmp (parent_->getResource("diskDecay"),"True");
    SetUsedFormat (parent_->getResource("diskUsedFormat"));
  }
  fields_[0] = 0.0;
  fields_[1] = 0.0;
}

void DiskMeter::checkevent( void ){
  getstats();
  drawfields();
}

void DiskMeter::getstats( void ){
  IntervalTimerStop();
  u_int64_t currBytes = 0;
//  Reset to desired full-scale settings.
  total_ = maxBandwidth_;

  if (kernelHasStats_) {
    NetBSDGetDiskXFerBytes (&currBytes);
#if DEBUG
    printf ("currBytes is %#x %#0x\n", (int) (currBytes >> 32), (int)
	    (currBytes & 0xffffffff));
#endif
  }
  fields_[0] = (currBytes-prevBytes)/1.0;
//  Adjust total_ if needed.
  if (fields_[0] > total_)
    total_ = fields_[0];

  fields_[1] = total_ - fields_[0];
  if (fields_[0] < 0.0)
    fprintf (stderr, "fields[0] of %f is < 0!\n", fields_[0]);
  if (fields_[1] < 0.0)
    fprintf (stderr, "fields[1] of %f is < 0!\n", fields_[1]);

    /*  For bytes/sec, we need to scale by our priority and the
     *  sampling time.  */
    
  setUsed ( fields_[0]*IntervalTimeInMicrosecs()/1e6, total_);
  prevBytes = currBytes;
  IntervalTimerStart();
}
