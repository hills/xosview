//
//  Copyright (c) 1994, 1995, 2002, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  Modifications to support dynamic addresses by:
//    Michael N. Lipp (mnl@dtro.e-technik.th-darmstadt.de)
//
//  This file may be distributed under terms of the GPL
//

#include "netmeter.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>

static const char PROCNETDEV[] = "/proc/net/dev";
static const char SYSCLASSNET[] = "/sys/class/net";

/*
 * Parse the integer count from the given filename
 *
 * This is quite relaxed about error conditions because the file may
 * have been removed before it was opened, or truncated, in which case
 * the count is zero.
 *
 * Return: 0 if not not avilable, otherwise count (which may be zero)
 */

static unsigned long long getCount( const char *filename ){
  unsigned long long n, count = 0;
  FILE *f;

  f = fopen(filename, "r");
  if (!f)
    return 0;

  if (fscanf(f, "%llu", &n) == 1)
    count = n;

  if (fclose(f) != 0)
    abort();

  return count;
}


NetMeter::NetMeter( XOSView *parent, float max )
  : FieldMeterGraph( parent, 3, "NET", "IN/OUT/IDLE" ){
  _maxpackets = max;
  _lastBytesIn = _lastBytesOut = 0;
  _usesysfs = _ignored = false;

  struct stat buf;
  if ( stat(SYSCLASSNET, &buf) == 0 && S_ISDIR(buf.st_mode) )
    _usesysfs = true;
}

NetMeter::~NetMeter( void ){
}

void NetMeter::checkResources( void ){
  FieldMeterGraph::checkResources();

  setfieldcolor( 0, parent_->getResource( "netInColor" ) );
  setfieldcolor( 1, parent_->getResource( "netOutColor" ) );
  setfieldcolor( 2, parent_->getResource( "netBackground" ) );
  priority_ = atoi( parent_->getResource( "netPriority" ) );
  useGraph_ = parent_->isResourceTrue( "netGraph" );
  dodecay_ = parent_->isResourceTrue( "netDecay" );
  SetUsedFormat( parent_->getResource("netUsedFormat") );
  _netIface = parent_->getResource( "netIface" );
  if (_netIface[0] == '-') {
    _ignored = true;
    _netIface.erase(0, _netIface.find_first_not_of("- "));
  }
}

void NetMeter::checkevent( void ){
  unsigned long long totin = 0, totout = 0;
  fields_[2] = _maxpackets;     // assume no
  fields_[0] = fields_[1] = 0;  // network activity

  IntervalTimerStop();
  if (_usesysfs)
    getSysStats(totin, totout);
  else
    getProcStats(totin, totout);

  double t = IntervalTimeInSecs();
  IntervalTimerStart();

  if (_lastBytesIn == 0 && _lastBytesOut == 0) {  // first run
    _lastBytesIn = totin;
    _lastBytesOut = totout;
  }

  fields_[0] = (totin - _lastBytesIn) / t;
  fields_[1] = (totout - _lastBytesOut) / t;

  _lastBytesIn = totin;
  _lastBytesOut = totout;

  total_ = fields_[0] + fields_[1];
  if (total_ > _maxpackets)
    fields_[2] = 0;
  else {
    total_ = _maxpackets;
    fields_[2] = total_ - fields_[0] - fields_[1];
  }

  setUsed(fields_[0] + fields_[1], total_);
  drawfields();
}

void NetMeter::getSysStats( unsigned long long &totin, unsigned long long &totout ){
  DIR *dir;
  struct dirent *ent;
  char filename[128];
  std::ifstream ifs;

  if ( !(dir = opendir(SYSCLASSNET)) ) {
    std::cerr << "Can not open directory : " << SYSCLASSNET << std::endl;
    parent_->done(1);
    return;
  }

  // walk through /sys/class/net/*/statistics/{r,t}x_bytes
  while ( (ent = readdir(dir)) ) {
    if ( ent->d_type != DT_LNK )
      continue;
    if ( _netIface != "False" &&
         ( (!_ignored && ent->d_name != _netIface) ||
           ( _ignored && ent->d_name == _netIface) ) )
        continue;

    snprintf(filename, 128, "%s/%s/statistics/rx_bytes", SYSCLASSNET, ent->d_name);
    totin += getCount(filename);

    snprintf(filename, 128, "%s/%s/statistics/tx_bytes", SYSCLASSNET, ent->d_name);
    totout += getCount(filename);
  }
  closedir(dir);
}

void NetMeter::getProcStats( unsigned long long &totin, unsigned long long &totout ){
  std::ifstream ifs(PROCNETDEV);
  std::string line, ifname;

  if (!ifs) {
    std::cerr << "Can not open file : " << PROCNETDEV << std::endl;
    parent_->done(1);
    return;
  }

  ifs.ignore(1024, '\n');
  ifs.ignore(1024, '\n');

  while ( !ifs.eof() ) {
    unsigned long long vals[9];
    std::getline(ifs, line);
    if ( !ifs.good() )
      break;

    int colon = line.find_first_of(':');
    ifname = line.substr(0, colon);
    ifname.erase(0, ifname.find_first_not_of(' '));
    if (_netIface != "False") {
      if ( (!_ignored && ifname != _netIface) ||
           ( _ignored && ifname == _netIface) )
        continue;
    }

    std::string l = line.erase(0, colon + 1);
    const char *cur = l.c_str();
    if ( strncmp(cur, " No ", 4) == 0 )
      continue; // xxx: No statistics available.

    char *end = NULL;
    for (int i = 0; i < 9; i++) {
      vals[i] = strtoull(cur, &end, 10);
      cur = end;
    }
    totin += vals[0];
    totout += vals[8];
    XOSDEBUG("%s: %llu bytes received, %llu bytes sent.\n",
             ifname.c_str(), vals[0], vals[8]);
  }
}
