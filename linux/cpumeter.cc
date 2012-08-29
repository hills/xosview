//
//  Copyright (c) 1994, 1995, 2002, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "cpumeter.h"
#include "xosview.h"
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sstream>
#include <ctype.h>

static const char STATFILENAME[] = "/proc/stat";
static const char VERSIONFILENAME[] = "/proc/version";
static int cputime_to_field[10] = { 0, 1, 2, 9, 5, 4, 3, 8, 6, 7 };

#define MAX_PROCSTAT_LENGTH 4096

CPUMeter::CPUMeter(XOSView *parent, const char *cpuID)
: FieldMeterGraph( parent, 10, toUpper(cpuID), "USR/NIC/SYS/SI/HI/WIO/GST/NGS/ST/IDLE" ) {
  _lineNum = findLine(cpuID);
  for ( int i = 0 ; i < 2 ; i++ )
    for ( int j = 0 ; j < 10 ; j++ )
      cputime_[i][j] = 0;
  cpuindex_ = 0;
  kernel_ = getkernelversion();
  if (kernel_ < 2006000) {  // 2.4 kernels had four stats
    legend("USR/NIC/SYS/IDLE");
    setNumFields(4);
    cputime_to_field[3] = 3;
  }
  else if (kernel_ < 2006011) {  // steal time appeared on 2.6.11
    legend("USR/NIC/SYS/SI/HI/WIO/IDLE");
    setNumFields(7);
    cputime_to_field[3] = 6;
  }
  else if (kernel_ < 2006024) {  // guest time appeared on 2.6.24
    legend("USR/NIC/SYS/SI/HI/WIO/ST/IDLE");
    setNumFields(8);
    cputime_to_field[3] = 7;
  }
  else if (kernel_ < 2006032) {  // niced guest time appeared on 2.6.32
    legend("USR/NIC/SYS/SI/HI/WIO/GST/ST/IDLE");
    setNumFields(9);
    cputime_to_field[3] = 8;
  }
}

CPUMeter::~CPUMeter( void ){
}

void CPUMeter::checkResources( void ){
  FieldMeterGraph::checkResources();

  setfieldcolor( 0, parent_->getResource( "cpuUserColor" ) );
  setfieldcolor( 1, parent_->getResource( "cpuNiceColor" ) );
  setfieldcolor( 2, parent_->getResource( "cpuSystemColor" ) );
  if (kernel_ >= 2006000) {
    setfieldcolor( 3, parent_->getResource( "cpuSInterruptColor" ) );
    setfieldcolor( 4, parent_->getResource( "cpuInterruptColor" ) );
    setfieldcolor( 5, parent_->getResource( "cpuWaitColor" ) );
  }
  if (kernel_ >= 2006024)
    setfieldcolor( 6, parent_->getResource( "cpuGuestColor" ) );
  if (kernel_ >= 2006032)
    setfieldcolor( 7, parent_->getResource( "cpuNiceGuestColor" ) );
  if (kernel_ >= 2006011)
    setfieldcolor( numfields_-2, parent_->getResource( "cpuStolenColor" ) );
  setfieldcolor( numfields_-1, parent_->getResource( "cpuFreeColor" ) );
  priority_ = atoi (parent_->getResource( "cpuPriority" ) );
  dodecay_ = parent_->isResourceTrue( "cpuDecay" );
  useGraph_ = parent_->isResourceTrue( "cpuGraph" );
  SetUsedFormat (parent_->getResource("cpuUsedFormat"));
}

void CPUMeter::checkevent( void ){
  getcputime();
  drawfields();
}

void CPUMeter::getcputime( void ){
  total_ = 0;
  std::string tmp;
  std::ifstream stats( STATFILENAME );

  if ( !stats ){
    std::cerr <<"Can not open file : " <<STATFILENAME << std::endl;
    exit( 1 );
  }

  // read until we are at the right line.
  for (int i = 0 ; i < _lineNum ; i++) {
    if (stats.eof())
      break;
    getline(stats, tmp);
  }

  stats >>tmp >>cputime_[cpuindex_][0]
	      >>cputime_[cpuindex_][1]
	      >>cputime_[cpuindex_][2]
	      >>cputime_[cpuindex_][3]
	      >>cputime_[cpuindex_][4]
	      >>cputime_[cpuindex_][5]
	      >>cputime_[cpuindex_][6]
	      >>cputime_[cpuindex_][7]
	      >>cputime_[cpuindex_][8]
	      >>cputime_[cpuindex_][9];

  int oldindex = (cpuindex_+1)%2;
  for ( int i = 0 ; i < numfields_ ; i++ ){
    int field = cputime_to_field[i];
    // counters in /proc/stat do sometimes go backwards
    fields_[field] = ( cputime_[cpuindex_][i] > cputime_[oldindex][i] ? cputime_[cpuindex_][i] - cputime_[oldindex][i] : 0 );
    total_ += fields_[field];
  }

  // Guest time already included in user time
  // Sometimes guest > user though
  if (kernel_ >= 2006024) {
    if (fields_[6] > fields_[0])
      fields_[6] = fields_[0];
    fields_[0] -= fields_[6];
    total_ -= fields_[6];
  }
  // Same applies to niced guest times
  if (kernel_ >= 2006032) {
    if (fields_[7] > fields_[1])
      fields_[7] = fields_[1];
    fields_[1] -= fields_[7];
    total_ -= fields_[7];
  }

  if (total_){
    setUsed (total_ - fields_[numfields_ - 1], total_); // any non-idle time
    cpuindex_ = (cpuindex_ + 1) % 2;
  }
}

int CPUMeter::findLine(const char *cpuID){
  std::ifstream stats( STATFILENAME );

  if ( !stats ){
    std::cerr <<"Can not open file : " <<STATFILENAME << std::endl;
    exit( 1 );
  }

  int line = -1;
  std::string buf;
  while (!stats.eof()){
    getline(stats, buf);
    if (!stats.eof()){
      line++;
      if (!strncmp(cpuID, buf.data(), strlen(cpuID))
        && buf[strlen(cpuID)] == ' ')
        return line;
    }
  }
  return -1;
}

// Checks for the SMP kernel patch by forissier@isia.cma.fr.
// http://www-isia.cma.fr/~forissie/smp_kernel_patch/
// If it finds that this patch has been applied to the current kernel
// then returns the number of cpus that are on this machine.
int CPUMeter::countCPUs(void){
  std::ifstream stats( STATFILENAME );

  if ( !stats ){
    std::cerr <<"Can not open file : " <<STATFILENAME << std::endl;
    exit( 1 );
  }

  int cpuCount = 0;
  std::string buf;
  while (getline(stats, buf))
      if (!strncmp(buf.data(), "cpu", 3) && buf[3] != ' ')
          cpuCount++;

  return cpuCount;
}

const char *CPUMeter::cpuStr(int num){
  static char buffer[32];
  std::ostringstream str;

  if (num != 0)
    snprintf(buffer, sizeof(buffer), "cpu%d", num - 1);
  else
    strcpy(buffer, "cpu");

  return buffer;
}

const char *CPUMeter::toUpper(const char *str){
  static char buffer[MAX_PROCSTAT_LENGTH];
  strncpy(buffer, str, MAX_PROCSTAT_LENGTH);
  for (char *tmp = buffer ; *tmp != '\0' ; tmp++)
    *tmp = toupper(*tmp);

  return buffer;
}

int CPUMeter::getkernelversion(void){
  std::ifstream f(VERSIONFILENAME);
  if (!f) {
    std::cerr << "Can not get kernel version from " << VERSIONFILENAME << "." << std::endl;
    exit(1);
  }

  std::string tmp, version;
  int major = 0, minor = 0, micro = 0;

  f >> tmp >> tmp >> version;
  sscanf(version.c_str(), "%d.%d.%d", &major, &minor, &micro);

  return ( major*1000000 + minor*1000 + micro);
}
