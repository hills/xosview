//
//  Copyright (c) 1994, 1995, 2002, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "ccxmeter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/utsname.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

static const char STATFILENAME[] = "/proc/stat";
static int cputime_to_field[10] = { 0, 1, 2, 9, 5, 4, 3, 8, 6, 7 };

#define MAX_PROCSTAT_LENGTH 4096

CCXMeter::CCXMeter(XOSView *parent, const int ccxID)
: FieldMeterGraph( parent, 10, ("CCX" + std::to_string(ccxID)).c_str(), "USR/NIC/SYS/SI/HI/WIO/GST/NGS/STL/IDLE" ) {

  _ccxId = ccxID;
  _lineNumStart = findLine(ccxID);
  _lineNumEnd = _lineNumStart + ccxSize;

  for ( int i = 0 ; i < 2 ; i++ )
    for ( int j = 0 ; j < 10 ; j++ )
      cputime_[i][j] = 0;
  cpuindex_ = 0;
  kernel_ = getKernelVersion();
  if (kernel_ < 2006000)
    statfields_ = 4;
  else if (kernel_ < 2006011)
    statfields_ = 7;
  else if (kernel_ < 2006024)
    statfields_ = 8;
  else if (kernel_ < 2006032)
    statfields_ = 9;
  else
    statfields_ = 10;
}

CCXMeter::~CCXMeter( void ){
}

void CCXMeter::checkResources( void ){
  FieldMeterGraph::checkResources();

  unsigned long usercolor = parent_->allocColor(parent_->getResource( "cpuUserColor" ) );
  unsigned long nicecolor = parent_->allocColor(parent_->getResource( "cpuNiceColor" ) );
  unsigned long syscolor  = parent_->allocColor(parent_->getResource( "cpuSystemColor" ) );
  unsigned long sintcolor = parent_->allocColor(parent_->getResource( "cpuSInterruptColor" ) );
  unsigned long intcolor  = parent_->allocColor(parent_->getResource( "cpuInterruptColor" ) );
  unsigned long waitcolor = parent_->allocColor(parent_->getResource( "cpuWaitColor" ) );
  unsigned long gstcolor  = parent_->allocColor(parent_->getResource( "cpuGuestColor" ) );
  unsigned long ngstcolor = parent_->allocColor(parent_->getResource( "cpuNiceGuestColor" ) );
  unsigned long stealcolor= parent_->allocColor(parent_->getResource( "cpuStolenColor" ) );
  unsigned long idlecolor = parent_->allocColor(parent_->getResource( "cpuFreeColor" ) );

  priority_ = atoi(parent_->getResource( "cpuPriority" ) );
  dodecay_ = parent_->isResourceTrue( "cpuDecay" );
  useGraph_ = parent_->isResourceTrue( "cpuGraph" );
  SetUsedFormat(parent_->getResource("cpuUsedFormat") );

  /* Use user-defined fields.
   * Fields         Including if not its own field
   * --------------|------------------------------
   *   USED         all used time, including user and system times
   *     USR        user time, including nice and guest times
   *       NIC      niced time, including niced guest unless guest is present
   *       GST      guest time, including niced guest time
   *         NGS    niced guest time
   *     SYS        system time, including interrupt and stolen times
   *       INT      interrupt time, including soft and hard interrupt times
   *         HI     hard interrupt time
   *         SI     soft interrupt time
   *       STL      stolen time
   *   IDLE         idle time, including io wait time
   *     WIO        io wait time
   *
   * Stolen time is a class of its own in kernel scheduler, in cpufreq it is
   * considered used time. Here it is part of used and system time, but can be
   * separate field as well.
   * Idle field is always present.
   * Either USED or at least USR+SYS must be included.
   */

  const char *f = parent_->getResource( "cpuFields" );
  std::string lgnd, fields(f);
  int field = 0;

  /* Check for possible fields and define field mapping. Assign colors and
   * build legend on the way.
   */
  if (fields.find("USED") != fields.npos) { // USED = USR+NIC+SYS+SI+HI+GST+NGS(+STL)
    if (fields.find("USR") != fields.npos || fields.find("NIC") != fields.npos ||
        fields.find("SYS") != fields.npos || fields.find("INT") != fields.npos ||
        fields.find("HI")  != fields.npos || fields.find("SI")  != fields.npos ||
        fields.find("GST") != fields.npos || fields.find("NGS") != fields.npos) {
      std::cerr << "'USED' cannot be in cpuFields together with either 'USR', "
                << "'NIC', 'SYS', 'INT', 'HI', 'SI', 'GST' or 'NGS'." << std::endl;
      exit(1);
    }
    setfieldcolor(field, usercolor);
    if (kernel_ >= 2006000) // SI and HI
      cputime_to_field[5] = cputime_to_field[6] = field;
    if (kernel_ >= 2006024) // GST
      cputime_to_field[8] = field;
    if (kernel_ >= 2006032) // NGS
      cputime_to_field[9] = field;
    if (kernel_ >= 2006011 && fields.find("STL") == fields.npos)
      cputime_to_field[7] = field; // STL can be separate as well
    // USR, NIC and SYS
    cputime_to_field[0] = cputime_to_field[1] = cputime_to_field[2] = field++;
    lgnd = "USED";
  }
  if (fields.find("USR") != fields.npos) {
    setfieldcolor(field, usercolor);
    // add NIC if not on its own
    if (fields.find("NIC") == fields.npos)
      cputime_to_field[1] = field;
    // add GST if not on its own
    if (kernel_ >= 2006024 && fields.find("GST") == fields.npos)
      cputime_to_field[8] = field;
    // add NGS if not on its own and neither NIC or GST is present
    if (kernel_ >= 2006032 && fields.find("NGS") == fields.npos &&
        fields.find("NIC") == fields.npos && fields.find("GST") == fields.npos)
      cputime_to_field[9] = field;
    cputime_to_field[0] = field++;
    lgnd = "USR";
  }
  else {
    if (fields.find("USED") == fields.npos) {
      std::cerr << "Either 'USED' or 'USR' is mandatory in cpuFields." << std::endl;
      exit(1);
    }
  }
  if (fields.find("NIC") != fields.npos) {
    setfieldcolor(field, nicecolor);
    // add NGS if not on its own and GST is not present
    if (kernel_ >= 2006032 && fields.find("NGS") == fields.npos &&
        fields.find("GST") == fields.npos)
      cputime_to_field[9] = field;
    cputime_to_field[1] = field++;
    lgnd += "/NIC";
  }
  if (fields.find("SYS") != fields.npos) {
    setfieldcolor(field, syscolor);
    // add SI if not on its own and INT is not present
    if (kernel_ >= 2006000 && fields.find("SI") == fields.npos &&
        fields.find("INT") == fields.npos)
      cputime_to_field[6] = field;
    // add HI if not on its own and INT is not present
    if (kernel_ >= 2006000 && fields.find("HI") == fields.npos &&
        fields.find("INT") == fields.npos)
      cputime_to_field[5] = field;
    // add STL if not on its own
    if (kernel_ >= 2006011 && fields.find("STL") == fields.npos)
      cputime_to_field[7] = field;
    cputime_to_field[2] = field++;
    lgnd += "/SYS";
  }
  else {
    if (fields.find("USED") == fields.npos) {
      std::cerr << "Either 'USED' or 'SYS' is mandatory in cpuFields." << std::endl;
      exit(1);
    }
  }
  if (kernel_ >= 2006000) {
    if (fields.find("INT") != fields.npos) { // combine soft and hard interrupt times
      setfieldcolor(field, intcolor);
      cputime_to_field[5] = cputime_to_field[6] = field++;
      lgnd += "/INT";
    } // Maybe should warn if both INT and HI/SI are requested ???
    else { // separate soft and hard interrupt times
      if (fields.find("SI") != fields.npos) {
        setfieldcolor(field, sintcolor);
        cputime_to_field[5] = field++;
        lgnd += "/SI";
      }
      if (fields.find("HI") != fields.npos) {
        setfieldcolor(field, intcolor);
        cputime_to_field[6] = field++;
        lgnd += "/HI";
      }
    }
    if (fields.find("WIO") != fields.npos) {
      setfieldcolor(field, waitcolor);
      cputime_to_field[4] = field++;
      lgnd += "/WIO";
    }
    if (kernel_ >= 2006024 && fields.find("GST") != fields.npos) {
      setfieldcolor(field, gstcolor);
      // add NGS if not on its own
      if (kernel_ >= 2006032 && fields.find("NGS") == fields.npos)
        cputime_to_field[9] = field;
      cputime_to_field[8] = field++;
      lgnd += "/GST";
    }
    if (kernel_ >= 2006032 && fields.find("NGS") != fields.npos) {
      setfieldcolor(field, ngstcolor);
      cputime_to_field[9] = field++;
      lgnd += "/NGS";
    }
    if (kernel_ >= 2006011 && fields.find("STL") != fields.npos) {
      setfieldcolor(field, stealcolor);
      cputime_to_field[7] = field++;
      lgnd += "/STL";
    }
  }
  // always add IDLE field
  setfieldcolor(field, idlecolor);
  // add WIO if not on its own
  if (kernel_ >= 2006000 && fields.find("WIO") == fields.npos)
    cputime_to_field[4] = field;
  cputime_to_field[3] = field++;
  lgnd += "/IDLE";

  legend(lgnd.c_str());
  numfields_ = field; // can't use setNumFields as it destroys the color mapping
}

void CCXMeter::checkevent( void ){
  getcputime();
  drawfields();
}

void CCXMeter::getcputime( void ){
  total_ = 0;
  std::string tmp;
  std::ifstream stats( STATFILENAME );
  char *end = NULL;

  if ( !stats ){
    std::cerr <<"Can not open file : " <<STATFILENAME << std::endl;
    exit( 1 );
  }

  // Zero out the cpuindex counters
  for (int i=0; i<10; i++) {
    cputime_[cpuindex_][i] = 0;
  }

  // read until we are at the first cpu in the ccx
  for (int i = 0 ; i < _lineNumStart ; i++) {
    if (stats.eof())
      return;
    stats.ignore(1024, '\n');
  }

  // read each line and aggregate the counts for the ccx
  for (int i = 0; i < ccxSize; i++) {
    std::getline(stats, tmp);
    if (stats.eof())
      return;

    std::stringstream ss(tmp);
    std::string field;
    bool firstField = true;
    long long col = 0;
    while (std::getline(ss, field, ' ')) {
      if (firstField) {
        firstField = false;
        continue;
      }
      if (!firstField) {
        //std::cout << "[" << _ccxId << "][" << col << "] " << field << std::endl;
        cputime_[cpuindex_][col++] += std::stoll(field);
      }
    }
  }

  // Guest time already included in user time.
  cputime_[cpuindex_][0] -= cputime_[cpuindex_][8];
  // Same applies to niced guest time.
  cputime_[cpuindex_][1] -= cputime_[cpuindex_][9];

  int oldindex = (cpuindex_+1)%2;
  // zero all the fields
  memset(fields_, 0, numfields_*sizeof(fields_[0]));
  for ( int i = 0 ; i < statfields_ ; i++ ){
    int time = cputime_[cpuindex_][i] - cputime_[oldindex][i];
    if (time < 0)    // counters in /proc/stat do sometimes go backwards
      time = 0;
    fields_[cputime_to_field[i]] += time;
    total_ += time;
  }


  if (total_){
    setUsed (total_ - fields_[numfields_ - 1], total_); // any non-idle time
    cpuindex_ = (cpuindex_ + 1) % 2;
  }
}

int CCXMeter::findLine(const int ccxId){
  std::string cpuStartId = "cpu" + std::to_string(ccxId * ccxSize);

  std::ifstream stats( STATFILENAME );

  if ( !stats ) {
    std::cerr <<"Can not open file : " <<STATFILENAME << std::endl;
    exit( 1 );
  }

  int line = -1;
  std::string buf;
  while (!stats.eof()) {
    getline(stats, buf);
    if (!stats.eof()) {
        line++;
        if (buf.rfind(cpuStartId) == 0) {
            return line;
        }
    }
  }
  return -1;
}

// Returns the number of ryzen ccxs that are on this machine.
int CCXMeter::countCCXs(void){
  std::ifstream stats( STATFILENAME );

  if ( !stats ){
    std::cerr <<"Can not open file : " <<STATFILENAME << std::endl;
    exit( 1 );
  }

  int cpuCount = 0;
  std::string buf;
  while (getline(stats, buf)) {
    if (!strncmp(buf.data(), "cpu", 3) && buf[3] != ' ') {
      cpuCount++;
    }
  }

  return cpuCount / ccxSize;
}


int CCXMeter::getKernelVersion(void){
  static int major = 0, minor = 0, micro = 0;
  if (!major) {
    struct utsname myosrelease;
    uname(&myosrelease);
    sscanf(myosrelease.release, "%d.%d.%d", &major, &minor, &micro);
  }
  return (major*1000000 + minor*1000 + micro);
}
