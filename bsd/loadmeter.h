//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//  Copyright (c) 1995, 1996, 1997 by Brian Grayson (bgrayson@ece.utexas.edu)
//
//  Most of this code was written by Werner Fink <werner@suse.de>
//  Only small changes were made on my part (M.R.)
//  And the near-trivial port to NetBSD was by bgrayson.
//
//  This file may be distributed under terms of the GPL or of the BSD
//    copyright, whichever you choose.  The full copyright notices are
//    contained in the files COPYING.GPL and COPYING.BSD, which you
//    should have received.  If not, contact one of the xosview
//    authors for a copy.
//
// $Id$
//
#ifndef _LOADMETER_H_
#define _LOADMETER_H_

#define LOADMETER_H_CVSID "$Id$"

#include "fieldmeterdecay.h"


class LoadMeter : public FieldMeterDecay {
public:
  LoadMeter( XOSView *parent );
  ~LoadMeter( void );

  const char *name( void ) const { return "LoadMeter"; }  
  void checkevent( void );

  void checkResources( void );
protected:

  void getloadinfo( void );
  unsigned long procloadcol_, warnloadcol_;
private:
  int alarmThreshold;
};


#endif
