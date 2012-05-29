//
//  Copyright (c) 1995, 1996, 1997-2002 by Brian Grayson (bgrayson@netbsd.org)
//
//  This file was written by Brian Grayson for the NetBSD and xosview
//    projects.
//  This file may be distributed under terms of the GPL or of the BSD
//    license, whichever you choose.  The full license notices are
//    contained in the files COPYING.GPL and COPYING.BSD, which you
//    should have received.  If not, contact one of the xosview
//    authors for a copy.
//

#include <err.h>        //  For err() and warn(), etc.  BCG
#include <stdlib.h>	//  For use of atoi  BCG
#include "diskmeter.h"
#include "kernel.h"     //  For NetBSD-specific icky (but handy) kvm_ code.  BCG

DiskMeter::DiskMeter( XOSView *parent, float max )
#if __FreeBSD_version >= 500000
: FieldMeterGraph( parent, 3, "DISK", "READ/WRITE/IDLE" ){
#else
: FieldMeterGraph( parent, 2, "DISK", "XFER/IDLE" ){
#endif
  //  The setting of the priority will be done in checkResources().  BCG
  dodecay_ = 0;

  kernelHasStats_ = BSDDiskInit();
  if (!kernelHasStats_)
  {
    warnx(
  "!!! The kernel does not seem to have the symbols needed for the DiskMeter."
	 );
#if defined(XOSVIEW_NETBSD)
    warnx(
  "  (A 1.2 kernel or above is probably needed)"
	 );
#else
# if defined(XOSVIEW_FREEBSD)
    warnx(
  "  (You are probably running a kernel newer than 3.0-19980804-SNAP)"
	 );
# else
    warnx(
  "  (You are probably running a newer or older kernel than any yet used with xosview)"
	 );
# endif
#endif
  warnx("!!! The DiskMeter has been disabled.");
    disableMeter ();
  }
#if __FreeBSD_version < 500000
  prevBytes = 0;
#endif
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
#if __FreeBSD_version >= 500000
    fields_[2] = total_;
#else
    fields_[1] = total_;
#endif
  }
}

DiskMeter::~DiskMeter( void ){
}

void DiskMeter::checkResources( void ){
  FieldMeterGraph::checkResources();

  if (kernelHasStats_) {
#if __FreeBSD_version >= 500000
    setfieldcolor( 0, parent_->getResource("diskReadColor") );
    setfieldcolor( 1, parent_->getResource("diskWriteColor") );
    setfieldcolor( 2, parent_->getResource("diskIdleColor") );
#else
    setfieldcolor( 0, parent_->getResource("diskUsedColor") );
    setfieldcolor( 1, parent_->getResource("diskIdleColor") );
#endif
    priority_ = atoi (parent_->getResource("diskPriority"));
    dodecay_ = parent_->isResourceTrue("diskDecay");
    useGraph_ = parent_->isResourceTrue("diskGraph");
    SetUsedFormat (parent_->getResource("diskUsedFormat"));
  }
  fields_[0] = 0.0;
  fields_[1] = 0.0;
#if __FreeBSD_version >= 500000
  fields_[2] = 0.0;
#endif
}

void DiskMeter::checkevent( void ){
  getstats();
  drawfields();
}

void DiskMeter::getstats( void ){
  IntervalTimerStop();
#if __FreeBSD_version >= 500000
  u_int64_t reads = 0, writes = 0;
#else
  unsigned long long currBytes = 0;
#endif
//  Reset to desired full-scale settings.
  total_ = maxBandwidth_;

  if (kernelHasStats_) {
#if __FreeBSD_version >= 500000
    BSDGetDiskXFerBytes (&reads, &writes);
#else
    BSDGetDiskXFerBytes (&currBytes);
#endif
#if DEBUG
    printf ("currBytes is %#x %#0x\n", (int) (currBytes >> 32), (int)
	    (currBytes & 0xffffffff));
#endif
  }
  /*  Adjust this to bytes/second.  */
#if __FreeBSD_version >= 500000
  fields_[0] = reads / IntervalTimeInSecs();
  fields_[1] = writes / IntervalTimeInSecs();
#else
  fields_[0] = (currBytes-prevBytes)/IntervalTimeInSecs();
#endif
  /*  Adjust in case of first call.  */
#if __FreeBSD_version >= 500000
  if (fields_[0] < 0)
    fields_[0] = 0.0;
  if (fields_[1] < 0)
    fields_[1] = 0.0;
#else
  if (fields_[0] < 0)
    fields_[0] = 0.0;
#endif
//  Adjust total_ if needed.
#if __FreeBSD_version >= 500000
  if (fields_[0] + fields_[1] > total_)
    total_ = fields_[0] + fields_[1];

  fields_[2] = total_ - ( fields_[0] + fields_[1] );
  if (fields_[0] < 0.0)
    fprintf (stderr, "diskmeter: fields[0] of %f is < 0!\n", fields_[0]);
  if (fields_[1] < 0.0)
    fprintf (stderr, "diskmeter: fields[1] of %f is < 0!\n", fields_[1]);
  if (fields_[2] < 0.0)
    fprintf (stderr, "diskmeter: fields[2] of %f is < 0!\n", fields_[2]);

  setUsed (fields_[0] + fields_[1], total_);
#else
  if (fields_[0] > total_)
    total_ = fields_[0];

  fields_[1] = total_ - fields_[0];
  if (fields_[0] < 0.0)
    fprintf (stderr, "diskmeter: fields[0] of %f is < 0!\n", fields_[0]);
  if (fields_[1] < 0.0)
    fprintf (stderr, "diskmeter: fields[1] of %f is < 0!\n", fields_[1]);

  setUsed ( fields_[0], total_);
#endif

#ifdef HAVE_DEVSTAT
  /*  The devstat library provides a differential value already,
   *  so we should compare against 0 each time.  */
#if __FreeBSD_version < 500000
  prevBytes = 0;
#endif
#else
  prevBytes = currBytes;
#endif
  IntervalTimerStart();
}
