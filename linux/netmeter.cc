//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  Modifications to support dynamic addresses by:
//    Michael N. Lipp (mnl@dtro.e-technik.th-darmstadt.de)
//
//  This file may be distributed under terms of the GPL
//
//
// $Id$
//

//-----------------------------------------------------------------------
//
// To use this meter, ipaccounting needs to be configured in the kernel and
// accounting needs to be set up.  Here are a couple of lines from my
// rc.local which add ip accounting on all packets to and from my ip
// address (192.168.0.3):
// 
// /sbin/ipfw add accounting all iface 192.168.0.3 from 192.168.0.3 to 0/0
// /sbin/ipfw add accounting all iface 192.168.0.3 from 0/0 to 192.168.0.3
//
// If you have more than one ip address you can add lines similar to the
// ones above for the other addresses and this class will combine them in
// its display.
//-----------------------------------------------------------------------


#include "netmeter.h"
#include "xosview.h"

#include <unistd.h>
#include <fstream.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <netinet/in.h>

static const char NETFILENAME[] = "/proc/net/ip_acct";


NetMeter::NetMeter( XOSView *parent, float max )
  : FieldMeterDecay( parent, 3, "NET", "IN/OUT/IDLE" ){
  _timer.start();
  maxpackets_ = max;
  _lastBytesIn = _lastBytesOut = 0;
}

NetMeter::~NetMeter( void ){
  close (_ipsock);
}

void NetMeter::checkResources( void ){
  FieldMeterDecay::checkResources();

  setfieldcolor( 0, parent_->getResource( "netInColor" ) );
  setfieldcolor( 1, parent_->getResource( "netOutColor" ) );
  setfieldcolor( 2, parent_->getResource( "netBackground" ) );
  priority_ = atoi (parent_->getResource( "netPriority" ) );
  dodecay_ = !strcmp (parent_->getResource( "netDecay" ), "True" );

  _ipsock = socket(AF_INET, SOCK_DGRAM, 0);
  if (_ipsock == -1) {
    cerr <<"Can not open socket : " <<strerror( errno ) <<endl;
    parent_->done(1);
    return;
  }
}

void NetMeter::checkevent( void ){
  _timer.stop();
  fields_[2] = maxpackets_;     // assume no
  fields_[0] = fields_[1] = 0;  // network activity

  ifstream ifs(NETFILENAME);
  if (!ifs){
    cerr <<"Can not open file : " <<NETFILENAME <<endl;
    parent_->done(1);
    return;
  }

  struct ifconf ifc;
  char buff[1024];
  ifc.ifc_len = sizeof(buff);
  ifc.ifc_buf = buff;
  if (ioctl(_ipsock, SIOCGIFCONF, &ifc) < 0) {
    cerr <<"Can not get interface list : " <<strerror( errno ) <<endl;
    parent_->done(1);
    return;
  }

  char c;
  unsigned long sa, da, sm, dm, bytes;
  unsigned long tot_in = 0, tot_out = 0;

  ifs.ignore(1024, '\n');

  while (!ifs.eof()){
    ifs >> hex >> sa >> c >> sm >> c >> c >> da >> c >> dm;
    for (int index = 0 ; index < 7 ; index++)
      ifs.ignore(9999, ' ');
    ifs >> dec >> bytes;

    ifs.ignore(9999, '\n');

    if (!ifs.eof()){
      struct ifreq *ifr = ifc.ifc_req;
      for (register i = ifc.ifc_len / sizeof(struct ifreq); --i >= 0; ifr++) {
        unsigned long adr 
          = ntohl(((struct sockaddr_in *)&ifr->ifr_addr)->sin_addr.s_addr);
 	if (sm == 0 && da == adr) {
 	  tot_in += bytes;
 	  break;
 	}
 	if (dm == 0 && sa == adr) {
 	  tot_out += bytes;
 	  break;
 	}
      }
    }
  }

  // This will happen when a dynamic connection (SLIP/PPP) goes down.
  if ((tot_in < _lastBytesIn) || (tot_out < _lastBytesOut)){
    fields_[0] = fields_[1] = 0;
    _lastBytesIn = tot_in;
    _lastBytesOut = tot_out;
  }
  else {  
    float t = 1000000.0 / _timer.report();

    if (t < 0)  // can happen when system clock is reset. (ntp, timed, etc)
      t = 0.1;

    fields_[0] = (tot_in - _lastBytesIn) * t;
    fields_[1] = (tot_out - _lastBytesOut) * t;

    _lastBytesIn = tot_in;
    _lastBytesOut = tot_out;
  }

  adjust();
  if (total_)
    used( (int)((100 * (fields_[0] + fields_[1])) / total_) );
  _timer.start();
  drawfields();
}

void NetMeter::adjust(void){
  total_ = fields_[0] + fields_[1];

  if (total_ > maxpackets_)
    fields_[2] = 0;
  else {
    total_ = maxpackets_;
    fields_[2] = total_ - fields_[0] - fields_[1];
  }
}
