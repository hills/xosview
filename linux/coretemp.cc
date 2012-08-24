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
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>

static const char SYS_HWMON[] = "/sys/class/hwmon";
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
  SetUsedFormat( parent_->getResource( "coretempUsedFormat" ) );

  int cpu = 0;
  unsigned int cpucount = countCpus(_pkg);
  char name[80];
  std::string dummy;
  std::ifstream file;
  DIR *dir;
  struct dirent *dent;

  dir = opendir(SYS_HWMON);
  if (!dir) {
    std::cout << "Can not open " << SYS_HWMON << " directory." << std::endl;
    parent_->done(1);
    return;
  }

  while ( (dent = readdir(dir)) ) {
    if (!strncmp(dent->d_name, ".", 1))
      continue;
    if (!strncmp(dent->d_name, "..", 2))
      continue;

    snprintf(name, 80, "%s/%s/device/name", SYS_HWMON, dent->d_name);
    file.open(name);
    if (!file)
      continue;
    file >> dummy;
    file.close();

    if (strncmp(dummy.c_str(), "k8temp", 6) == 0) {
      _type = 1;
      _node = dent->d_name;
      if (_cpu < 0)  // avg or max
        for (uint i = 1; i <= cpucount; i++)
          _cpus.push_back(i);
      else      // single sensor (each core has two)
        _cpu++; // sysfs indice starts from one
    }
    else if (strncmp(dummy.c_str(), "coretemp", 8) == 0) {
      int i = 0;
      _type = 0;
      // Gather the sysfs indices which are not the same as core numbers.
      // Core numbers are not unique across physical CPUs, but sysfs indices are.
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
    }
    else if (strncmp(dummy.c_str(), "k10temp", 7) == 0) {
      _type = 1;
      _node = dent->d_name;
      _cpu = 1;
      _cpus.push_back(1);  // K10 has only one sensor per physical CPU
    }

    if (_cpus.empty())
      continue;

    // sysfs node was found
    // get TjMax and use it for total if available
    if (_type == 0)
      snprintf(name, 80, "%s.%d/temp%d_crit", SYS_CORETEMP, _pkg, _cpus.front());
    else
      snprintf(name, 80, "%s/%s/device/temp1_crit", SYS_HWMON, dent->d_name);
    file.open(name);
    if (file) {
      file >> total_;
      file.close();
      total_ /= 1000.0;
    }
    else
      total_ = atoi(parent_->getResourceOrUseDefault("coretempHighest", "100"));

    _high = total_;
    closedir(dir);
    return;
  }
  std::cout << "You do not seem to have CPU temperature sensor available." << std::endl;
  parent_->done(1);
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
    if (_type == 0)
      snprintf(name, 80, "%s.%d/temp%d_input", SYS_CORETEMP, _pkg, _cpu);
    else if (_type == 1)
      snprintf(name, 80, "%s/%s/device/temp%d_input", SYS_HWMON, _node.c_str(), _cpu);
    file.open(name);
    if (!file) {
      std::cerr << "Can not open file : " << name << std::endl;
      parent_->done(1);
      return;
    }
    file >> fields_[0];
    file.close();
  }
  else if (_cpu == -1) {  // average
    for (uint i = 0; i < _cpus.size(); i++) {
      if (_type == 0)
        snprintf(name, 80, "%s.%d/temp%d_input", SYS_CORETEMP, _pkg, _cpus[i]);
      else
        snprintf(name, 80, "%s/%s/device/temp%d_input", SYS_HWMON, _node.c_str(), _cpus[i]);
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
      if (_type == 0)
        snprintf(name, 80, "%s.%d/temp%d_input", SYS_CORETEMP, _pkg, _cpus[i]);
      else
        snprintf(name, 80, "%s/%s/device/temp%d_input", SYS_HWMON, _node.c_str(), _cpus[i]);
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
    std::cerr << "Unknown CPU core number " << _cpu << " in coretemp." << std::endl;
    parent_->done(1);
    return;
  }

  // Use tCase (when maximum cooling needs to be turned on) as high.
  // Not found on k8temp, which uses user defined total as high (or default 100).
  if (_type == 0)
    snprintf(name, 80, "%s.%d/temp%d_max", SYS_CORETEMP, _pkg, _cpus.front());
  else
    snprintf(name, 80, "%s/%s/device/temp%d_max", SYS_HWMON, _node.c_str(), _cpus.front());
  file.open(name);
  if (file) {
    file >> high;
    file.close();
  }
  else
    high = _high * 1000.0;

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

/* Count sensors available to coretemp in the given package. */
unsigned int CoreTemp::countCpus(int pkg)
{
  glob_t gbuf;
  char s[80];
  struct stat sbuf;
  int count = 0;
  std::string dummy;
  std::ifstream file;

  snprintf(s, 80, "%s.%d", SYS_CORETEMP, pkg);
  if (stat(s, &sbuf) == 0) {
    strncat(s, "/temp*_label", 80);
    glob(s, 0, NULL, &gbuf);
    // loop through paths in gbuf and check if it is a core or package
    for (uint i = 0; i < gbuf.gl_pathc; i++) {
      file.open(gbuf.gl_pathv[i]);
      file >> dummy;
      file.close();
      if (strncmp(dummy.c_str(), "Core", 4) == 0)
        count++;
    }
    globfree(&gbuf);
  }
  else {
    DIR *dir;
    struct dirent *dent;
    dir = opendir(SYS_HWMON);
    if (!dir)
      return 0;

    // loop through hwmon devices and if AMD sensor if found, count its inputs
    while ( (dent = readdir(dir)) ) {
      if (!strncmp(dent->d_name, ".", 1))
        continue;
      if (!strncmp(dent->d_name, "..", 2))
        continue;

      snprintf(s, 80, "%s/%s/device/name", SYS_HWMON, dent->d_name);
      file.open(s);
      file >> dummy;
      file.close();
      if (strncmp(dummy.c_str(), "k8temp", 6) == 0 || strncmp(dummy.c_str(), "k10temp", 7) == 0) {
        snprintf(s, 80, "%s/%s/device/temp*_input", SYS_HWMON, dent->d_name);
        break;
      }
    }
    closedir(dir);

    glob(s, 0, NULL, &gbuf);
    count = gbuf.gl_pathc;
    globfree(&gbuf);
  }
  return count;
}
