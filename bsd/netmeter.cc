//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  NetBSD port:  
//  Copyright (c) 1995 Brian Grayson(bgrayson@pine.ece.utexas.edu)
//
//  This file may be distributed under terms of the GPL
//

#include "netmeter.h"
#include "xosview.h"
#include "Host.h"
#include "netbsd.h"
#include <stdlib.h>		//  For atoi().  BCG
#include <unistd.h>  /*  For gethostname().  BCG */

NetMeter::NetMeter( XOSView *parent, float max )
  : FieldMeterDecay( parent, 3, "NET", "IN/OUT/IDLE" ){
  _timer.start();
  maxpackets_ = max;
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
}

void NetMeter::checkevent( void ){
  _timer.stop();
  total_ = maxpackets_;
  fields_[0] = fields_[1] = 0;


//  Begin NetBSD-specific code.  BCG
  long long nowBytesIn, nowBytesOut;

//  The NetBSDGetNetInOut() function is in netbsd.cc    BCG
  NetBSDGetNetInOut (&nowBytesIn, &nowBytesOut);
  float t = 1000000.0 / _timer.report();
  fields_[0] = (float)(nowBytesIn - _lastBytesIn) * t;
  _lastBytesIn = nowBytesIn;
  fields_[1] = (float)(nowBytesOut - _lastBytesOut) * t;
  _lastBytesOut = nowBytesOut;
//  End NetBSD-specific code.  BCG


  adjust();
  fields_[2] = total_ - fields_[0] - fields_[1];
  used( (int)((100 * (fields_[0] + fields_[1])) / total_) );
  _timer.start();
  drawfields();
}

void NetMeter::adjust(void){
  if (total_ < (fields_[0] + fields_[1]))
    total_ = fields_[0] + fields_[1];
}
