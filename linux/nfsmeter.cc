//
//  Copyright (c) 1994, 1995, 2002, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  Modifications to support dynamic addresses by:
//    Michael N. Lipp (mnl@dtro.e-technik.th-darmstadt.de)
//
//  This file may be distributed under terms of the GPL
//

#include "nfsmeter.h"
#include <string.h>
#include <stdio.h>
#include <fstream>
// #include <iostream>

#ifndef MAX
#define MAX(_a, _b) ((_a) > (_b) ? (_a) : (_b))
#endif

static const char *NFSSVCSTAT = "/proc/net/rpc/nfsd";
static const char * NFSCLTSTAT = "/proc/net/rpc/nfs";

NFSMeter::NFSMeter(XOSView *parent, const char *name, int nfields,
		const char *fields, const char *statfile)
  : FieldMeterGraph( parent, nfields, name, fields ){
	_statfile = statfile;
	_statname = name;
}

NFSMeter::~NFSMeter( void ){
}

void NFSMeter::checkResources( void ){
  FieldMeterGraph::checkResources();
}

NFSDStats::NFSDStats(XOSView *parent)
  : NFSMeter(parent, "NFSD", 4, "BAD/UDP/TCP/IDLE", NFSSVCSTAT ){
	starttimer();
}

NFSDStats::~NFSDStats( void ) {
}

void NFSDStats::checkResources( void ){
  NFSMeter::checkResources();

  setfieldcolor( 0, parent_->getResource( "NFSDStatBadCallsColor" ) );
  setfieldcolor( 1, parent_->getResource( "NFSDStatUDPColor" ) );
  setfieldcolor( 2, parent_->getResource( "NFSDStatTCPColor" ) );
  setfieldcolor( 3, parent_->getResource( "NFSDStatIdleColor" ) );

  useGraph_ = parent_->isResourceTrue( "NFSDStatGraph" );
  dodecay_ = parent_->isResourceTrue( "NFSDStatDecay" );
  SetUsedFormat (parent_->getResource("NFSDStatUsedFormat"));
  //useGraph_ = 1;
  //dodecay_ = 1;
  //SetUsedFormat ("autoscale");
  //SetUsedFormat ("percent");
}
void NFSDStats::checkevent(void)
{
	char buf[4096], name[64];
	unsigned long netcnt, netudpcnt, nettcpcnt, nettcpconn;
	unsigned long calls, badcalls;
	int found;

    std::ifstream ifs(_statfile);

    if (!ifs) {
        // cerr <<"Can not open file : " <<_statfile <<endl;
        // parent_->done(1);
        return;
	}

	fields_[0] = fields_[1] = fields_[2] = 0;  // network activity
    stoptimer();

	name[0] = '\0';
	found = 0;
	while (!ifs.eof() && found != 2) {
		ifs.getline(buf, 4096, '\n');
		if (strncmp("net", buf, strlen("net")) == 0) {
			sscanf(buf, "%s %lu %lu %lu %lu\n", name,
				&netcnt, &netudpcnt, &nettcpcnt, &nettcpconn);
			found++;
		}
		if (strncmp("rpc", buf, strlen("rpc")) == 0) {
			sscanf(buf, "%s %lu %lu\n", name, &calls, &badcalls);
			found++;
		}
	}

    float t = 1000000.0 / usecs();

    if (t < 0)
        t = 0.1;

	maxpackets_ = MAX(netcnt, calls) - _lastNetCnt;
	if (maxpackets_ == 0) {
		maxpackets_ = netcnt;
	} else {
		fields_[0] = (badcalls - _lastBad) * t;
		fields_[1] = (netudpcnt - _lastUdp) * t;
		fields_[2] = (nettcpcnt - _lastTcp) * t;
	}

    total_ = fields_[0] + fields_[1] + fields_[2];
    if (total_ > maxpackets_)
        fields_[3] = 0;
    else {
        total_ = maxpackets_;
        fields_[3] = total_ - fields_[0] - fields_[1] - fields_[2];
	}

    if (total_)
        setUsed(fields_[0] + fields_[1] + fields_[2], total_);

    starttimer();
    drawfields();

	_lastNetCnt = MAX(netcnt, calls);
    _lastTcp = nettcpcnt;
    _lastUdp = netudpcnt;
    _lastBad = badcalls;
}

NFSStats::NFSStats(XOSView *parent)
  : NFSMeter(parent, "NFS", 4, "RETRY/AUTH/CALL/IDLE", NFSCLTSTAT ){
	starttimer();
}

NFSStats::~NFSStats( void ) {
}

void NFSStats::checkResources( void ){
  NFSMeter::checkResources();

  setfieldcolor( 0, parent_->getResource( "NFSStatReTransColor" ) );
  setfieldcolor( 1, parent_->getResource( "NFSStatAuthRefrshColor" ) );
  setfieldcolor( 2, parent_->getResource( "NFSStatCallsColor" ) );
  setfieldcolor( 3, parent_->getResource( "NFSStatIdleColor" ) );

  useGraph_ = parent_->isResourceTrue( "NFSStatGraph" );
  dodecay_ = parent_->isResourceTrue( "NFSStatDecay" );
  SetUsedFormat (parent_->getResource("NFSStatUsedFormat"));
  //SetUsedFormat ("autoscale");
  //SetUsedFormat ("percent");
}

void NFSStats::checkevent(void)
{
	char buf[4096], name[64];
	unsigned long calls = 0, retrns = 0, authrefresh = 0, maxpackets_;

    std::ifstream ifs(_statfile);

    if (!ifs) {
        // cerr <<"Can not open file : " <<_statfile <<endl;
        // parent_->done(1);
        return;
	}

	fields_[0] = fields_[1] = fields_[2] = 0;
    stoptimer();

	name[0] = '\0';
	while (!ifs.eof()) {
		ifs.getline(buf, 4096, '\n');
		if (strncmp("rpc", buf, strlen("rpc")))
			continue;
		sscanf(buf, "%s %lu %lu %lu\n", name, &calls, &retrns, &authrefresh);
		break;
	}

    float t = 1000000.0 / usecs();

    if (t < 0)
        t = 0.1;

	maxpackets_ = calls - _lastcalls;
	if (maxpackets_ == 0) {
		maxpackets_ = calls;
	} else {
		fields_[2] = (calls - _lastcalls) * t;
		fields_[1] = (authrefresh - _lastauthrefresh) * t;
		fields_[0] = (retrns - _lastretrns) * t;
	}

    total_ = fields_[0] + fields_[1] + fields_[2];
    if (total_ > maxpackets_)
        fields_[3] = 0;
    else {
        total_ = maxpackets_;
        fields_[3] = total_ - fields_[2] - fields_[1] - fields_[0];
	}

    if (total_)
        setUsed(fields_[0] + fields_[1] + fields_[2], total_);

    starttimer();
    drawfields();

	_lastcalls = calls;
	_lastretrns = retrns;
	_lastauthrefresh = authrefresh;
}
