//  
//  NetBSD port:  
//  Copyright (c) 1995,1996,1997 Brian Grayson(bgrayson@netbsd.org)
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
#ifndef _DISKMETER_H_
#define _DISKMETER_H_

#define DISKMETER_H_CVSID "$Id$"

#include "fieldmetergraph.h"
#include <sys/types.h>		//  For u_int64_t

class DiskMeter : public FieldMeterGraph {
public:
  DiskMeter( XOSView *parent, float max );
  ~DiskMeter( void );

  const char *name( void ) const { return "DiskMeter"; }
  void checkevent( void );

  void checkResources( void );
protected:
  void getstats( void );
private:
  u_int64_t prevBytes;
  int kernelHasStats_;
  float	maxBandwidth_;
};

#endif
