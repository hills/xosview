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
#include "general.h"
#include "netmeter.h"
#include "xosview.h"
#include "Host.h"
#include "kernel.h"
#include <stdlib.h>		//  For atoi().  BCG
#include <unistd.h>  /*  For gethostname().  BCG */

CVSID("$Id$");
CVSID_DOT_H(NETMETER_H_CVSID);
CVSID_DOT_H2(TIMER_H_CVSID);
CVSID_DOT_H3(TIMEVAL_H_CVSID);

NetMeter::NetMeter( XOSView *parent, float max )
  : FieldMeterDecay( parent, 3, "NET", "IN/OUT/IDLE" ){
  IntervalTimerStart();
  netBandwidth_ = max;
  total_ = netBandwidth_;
  _lastBytesIn = _lastBytesOut = 0;
  NetBSDGetNetInOut (&_lastBytesIn, &_lastBytesOut);

  char hostname[100];
  gethostname(hostname, 99);
  _thisHost = new Host(hostname);
}

NetMeter::~NetMeter( void ){
  delete _thisHost;
}

void NetMeter::checkResources( void ){
  FieldMeter::checkResources();

  setfieldcolor( 0, parent_->getResource("netInColor") );
  setfieldcolor( 1, parent_->getResource("netOutColor") );
  setfieldcolor( 2, parent_->getResource("netBackground") );
  priority_ = atoi (parent_->getResource("netPriority") );
  dodecay_ = !strcmp (parent_->getResource("netDecay"),"True");
  SetUsedFormat (parent_->getResource("netUsedFormat"));
}

void NetMeter::checkevent( void ){
  IntervalTimerStop();

//  Reset total_ to expected maximum.  If it is too low, it
//  will be adjusted in adjust().  bgrayson
  total_ = netBandwidth_;

  fields_[0] = fields_[1] = 0;

//  Begin NetBSD-specific code.  BCG
  long long nowBytesIn, nowBytesOut;

//  The NetBSDGetNetInOut() function is in kernel.cc    BCG
  NetBSDGetNetInOut (&nowBytesIn, &nowBytesOut);
  float t = (1e6) / IntervalTimeInMicrosecs();
  fields_[0] = (float)(nowBytesIn - _lastBytesIn) * t;
  _lastBytesIn = nowBytesIn;
  fields_[1] = (float)(nowBytesOut - _lastBytesOut) * t;
  _lastBytesOut = nowBytesOut;
//  End NetBSD-specific code.  BCG

  adjust();
  fields_[2] = total_ - fields_[0] - fields_[1];
    /*  The fields_ values have already been scaled into bytes/sec by
     *  the manipulations (* t) above.  */
  setUsed (fields_[0]+fields_[1], total_);
  IntervalTimerStart();
  drawfields();
}

void NetMeter::adjust(void){
  if (total_ < (fields_[0] + fields_[1]))
    total_ = fields_[0] + fields_[1];
}
