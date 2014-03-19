//
//  Copyright (c) 1994, 1995, 2006, 2008 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//  Most of this code was written by Werner Fink <werner@suse.de>.
//  Only small changes were made on my part (M.R.)
//

#include "loadmeter.h"
#include "cpumeter.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <string>

static const char LOADFILENAME[] = "/proc/loadavg";
static const char SPEEDFILENAME[] = "/proc/cpuinfo";


LoadMeter::LoadMeter( XOSView *parent )
  : FieldMeterGraph( parent, 2, "LOAD", "PROCS/MIN", 1, 1, 0 ){
  lastalarmstate = -1;
  total_ = 2.0;
  old_cpu_speed_= cur_cpu_speed_=0;
  do_cpu_speed = 0;

}

LoadMeter::~LoadMeter( void ){
}

void LoadMeter::checkResources( void ){
  FieldMeterGraph::checkResources();

  procloadcol_ = parent_->allocColor(parent_->getResource( "loadProcColor" ));
  warnloadcol_ = parent_->allocColor(parent_->getResource( "loadWarnColor" ));
  critloadcol_ = parent_->allocColor(parent_->getResource( "loadCritColor" ));

  setfieldcolor( 0, procloadcol_ );
  setfieldcolor( 1, parent_->getResource( "loadIdleColor" ) );
  priority_ = atoi (parent_->getResource( "loadPriority" ) );
  useGraph_ = parent_->isResourceTrue( "loadGraph" );
  dodecay_ = parent_->isResourceTrue( "loadDecay" );
  SetUsedFormat (parent_->getResource("loadUsedFormat"));

  const char *warn = parent_->getResource("loadWarnThreshold");
  if (strncmp(warn, "auto", 2) == 0) {
      warnThreshold = CPUMeter::countCPUs();
  } else {
      warnThreshold = atoi(warn);
  }

  const char *crit = parent_->getResource("loadCritThreshold");
  if (strncmp(crit, "auto", 2) == 0) {
      critThreshold = warnThreshold * 4;
  } else {
      critThreshold = atoi(crit);
  }

  do_cpu_speed  = parent_->isResourceTrue( "loadCpuSpeed" );

  if (dodecay_){
    //  Warning:  Since the loadmeter changes scale occasionally, old
    //  decay values need to be rescaled.  However, if they are rescaled,
    //  they could go off the edge of the screen.  Thus, for now, to
    //  prevent this whole problem, the load meter can not be a decay
    //  meter.  The load is a decaying average kind of thing anyway,
    //  so having a decaying load average is redundant.
    std::cerr << "Warning:  The loadmeter can not be configured as a decay\n"
         << "  meter.  See the source code (" << __FILE__ << ") for further\n"
         << "  details.\n";
    dodecay_ = 0;
  }
}

void LoadMeter::checkevent( void ){
  getloadinfo();
  if ( do_cpu_speed ) {
    getspeedinfo();
    if ( old_cpu_speed_ != cur_cpu_speed_ ) {
      // update the legend:
      char l[32];
      snprintf(l, 32, "PROCS/MIN %d MHz", cur_cpu_speed_);
      legend(l);
      drawlegend();
    }
  }

  drawfields();
}


void LoadMeter::getloadinfo( void ){
  std::ifstream loadinfo( LOADFILENAME );

  if ( !loadinfo ){
    std::cerr <<"Can not open file : " <<LOADFILENAME << std::endl;
    parent_->done(1);
    return;
  }

  loadinfo >> fields_[0];

  if ( fields_[0] <  warnThreshold ) alarmstate = 0;
  else
  if ( fields_[0] >= critThreshold ) alarmstate = 2;
  else
  /* if fields_[0] >= warnThreshold */ alarmstate = 1;

  if ( alarmstate != lastalarmstate ){
    if ( alarmstate == 0 ) setfieldcolor( 0, procloadcol_ );
    else
    if ( alarmstate == 1 ) setfieldcolor( 0, warnloadcol_ );
    else
    /* if alarmstate == 2 */ setfieldcolor( 0, critloadcol_ );
    drawlegend();
    lastalarmstate = alarmstate;
  }

  // Adjust total to next power-of-two of the current load.
  if ( (fields_[0]*5.0 < total_ && total_ > 1.0) || fields_[0] > total_ ) {
    unsigned int i = fields_[0];
    i |= i >> 1; i |= i >> 2; i |= i >> 4; i |= i >> 8; i |= i >> 16;  // i = 2^n - 1
    total_ = i + 1;
  }

  fields_[1] = (float) (total_ - fields_[0]);

  setUsed(fields_[0], (float) 1.0);
}

// just check /proc/cpuinfo for the speed of cpu
// (average multi-cpus on different speeds)
// (yes - i know about devices/system/cpu/cpu*/cpufreq )
void LoadMeter::getspeedinfo( void ){
  std::ifstream speedinfo(SPEEDFILENAME);
  std::string line, val;
  unsigned int total_cpu = 0, ncpus = 0;

  while ( speedinfo.good() ) {
    std::getline(speedinfo, line);
    if ( strncmp(line.c_str(), "cpu MHz", 7) == 0 ) {
      val = line.substr(line.find_last_of(':') + 1);
      XOSDEBUG("SPEED: %s\n", val.c_str());
      total_cpu += atoi(val.c_str());
      ncpus++;
    }
  }

  old_cpu_speed_ = cur_cpu_speed_;
  if (ncpus > 0)
    cur_cpu_speed_ = total_cpu / ncpus;
  else
    cur_cpu_speed_ = 0;
}
