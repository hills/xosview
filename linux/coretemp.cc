//
//  Copyright (c) 2008 by Tomi Tapper <tomi.o.tapper@jyu.fi>
//
//  Read coretemp reading from /sys and display actual temperature.
//  If actual >= high, actual temp changes color to indicate alarm.
//
//  File based on linux/lmstemp.* by
//  Copyright (c) 2000, 2006 by Leopold Toetsch <lt@toetsch.at>
//
//  This file may be distributed under terms of the GPL
//
//
//
#include "coretemp.h"
#include "xosview.h"
#include <stdlib.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <fstream>

static const char SYS_CORETEMP[] = "/sys/devices/platform/coretemp";

CoreTemp::CoreTemp( XOSView *parent, const char *label, const char *caption, int pkg, int cpu )
  : FieldMeter( parent, 3, label, caption, 1, 1, 1 ) {
  _pkg = pkg;
  _cpu = cpu;
  _high = 0;
  checkResources();
}

CoreTemp::~CoreTemp( void ) {

}

void CoreTemp::checkResources( void ) {
  FieldMeter::checkResources();

  setfieldcolor( 0, parent_->getResource( "coretempActColor" ) );
  setfieldcolor( 1, parent_->getResource( "coretempIdleColor") );
  setfieldcolor( 2, parent_->getResource( "coretempHighColor") );

  priority_ = atoi( parent_->getResource( "coretempPriority" ) );

  int i = 0, cpu = 0;
  unsigned int cpucount = countCpus(_pkg);
  char name[80];
  std::string dummy;
  std::ifstream file;

  // gather the sysfs indices which are not the same as core numbers
  while (_cpus.size() != cpucount) {
    snprintf(name, 80, "%s.%d/temp%d_label", SYS_CORETEMP, _pkg, i);
    file.open(name);
    if (!file) {
      i++;
      continue;
    }
    file >> dummy >> cpu;  // "Core n" or "Physical id n"
    file.close();
    if (strncmp(dummy.c_str(), "Core", 1) != 0) {
      i++;
      continue;
    }
    if (cpu == _cpu) {
      _cpu = i;           // for one core
      _cpus.push_back(i);
      break;
    }
    _cpus.push_back(i++); // for avg and max
  }

  // get TjMax and use it for total (always available?)
  snprintf(name, 80, "%s.%d/temp%d_crit", SYS_CORETEMP, _pkg, _cpus.front());
  file.open(name);
  file >> total_;
  total_ /= 1000.0;

  SetUsedFormat( parent_->getResource( "coretempUsedFormat" ) );
}

void CoreTemp::checkevent( void ) {
  getcoretemp();
  drawfields();
}

void CoreTemp::getcoretemp( void ) {
  char name[80];
  std::ifstream file;
  double dummy, high;

  fields_[0] = 0.0;
  high = 0.0;

  if (_cpu >= 0) {  // only one core
    snprintf(name, 80, "%s.%d/temp%d_input", SYS_CORETEMP, _pkg, _cpu);
    file.open(name);
    if (!file) {
      std::cerr << "Can not open file : " << name << std::endl;
      parent_->done(1);
      return;
    }
    file >> fields_[0];
    file.close();
    snprintf(name, 80, "%s.%d/temp%d_max", SYS_CORETEMP, _pkg, _cpu);
    file.open(name);
    if (!file) {
      std::cerr << "Can not open file : " << name << std::endl;
      parent_->done(1);
      return;
    }
    file >> high;
    file.close();
  }
  else {
    if (_cpu == -1) {  // average
      for (uint i = 0; i < _cpus.size(); i++) {
        snprintf(name, 80, "%s.%d/temp%d_input", SYS_CORETEMP, _pkg, _cpus[i]);
        file.open(name);
        if (!file) {
          std::cerr << "Can not open file : " << name << std::endl;
          parent_->done(1);
          return;
        }
        file >> dummy;
        file.close();
        fields_[0] += dummy;
      }
      fields_[0] /= (double)_cpus.size();
    }
    else if (_cpu == -2) {  // maximum
      for (uint i = 0; i < _cpus.size(); i++) {
        snprintf(name, 80, "%s.%d/temp%d_input", SYS_CORETEMP, _pkg, _cpus[i]);
        file.open(name);
        if (!file) {
          std::cerr << "Can not open file : " << name << std::endl;
          parent_->done(1);
          return;
        }
        file >> dummy;
        file.close();
        if (dummy > fields_[0])
          fields_[0] = dummy;
      }
    }
    else {  // should not happen
      std::cerr << "Unknown CPU core number in coretemp." << std::endl;
      parent_->done(1);
      return;
    }
    // high is likely the same for all cores
    snprintf(name, 80, "%s.%d/temp%d_max", SYS_CORETEMP, _pkg, _cpus.front());
    file.open(name);
    if (!file) {
      std::cerr << "Can not open file : " << name << std::endl;
      parent_->done(1);
      return;
    }
    file >> high;
    file.close();
  }

  fields_[0] /= 1000.0;
  high /= 1000.0;
  fields_[1] = high - fields_[0];
  if (fields_[1] < 0) { // alarm: T > high
    fields_[1] = 0;
    setfieldcolor( 0, parent_->getResource( "coretempHighColor" ) );
  }
  else
    setfieldcolor( 0, parent_->getResource( "coretempActColor" ) );

  fields_[2] = total_ - fields_[1] - fields_[0];
  if (fields_[2] < 0) { // alarm: T > TjMax
    fields_[2] = 0;
    setfieldcolor( 0, parent_->getResource( "coretempHighColor" ) );
  }
  else
    setfieldcolor( 0, parent_->getResource( "coretempActColor" ) );

  setUsed( fields_[0], total_ );

  if (high != _high) {
    _high = high;
    snprintf(name, 80, "ACT/%d/%d", (int)high, (int)total_);
    legend(name);
    drawlegend();
  }
}

/* Count CPUs available to coretemp in the given package. */
unsigned int CoreTemp::countCpus(int pkg)
{
  glob_t buf;
  char s[80];
  snprintf(s, 80, "%s.%d/temp*_input", SYS_CORETEMP, pkg);
  glob(s, 0, NULL, &buf);
  int count = buf.gl_pathc;
  globfree(&buf);
  return count;
}
