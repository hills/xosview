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
#ifndef _PAGEMETER_H_
#define _PAGEMETER_H_

#define PAGEMETER_H_CVSID "$Id$"

#include "fieldmeterdecay.h"
#include <sys/vmmeter.h>

class PageMeter : public FieldMeterDecay {
public:
  PageMeter( XOSView *parent, double total );
  ~PageMeter( void );

  const char *name( void ) const { return "PageMeter"; }  
  void checkevent( void );

  void checkResources( void );
protected:

  void getpageinfo( void );
private:
  struct vmmeter prev_;
};


#endif
