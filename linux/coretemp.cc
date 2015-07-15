//
//  Copyright (c) 2008-2014 by Tomi Tapper <tomi.o.tapper@jyu.fi>
//
//  Read CPU temperature readings from /sys and display actual temperature.
//  If actual >= high, actual temp changes color to indicate alarm.
//
//  File based on linux/lmstemp.* by
//  Copyright (c) 2000, 2006 by Leopold Toetsch <lt@toetsch.at>
//
//  This file may be distributed under terms of the GPL
//
//

#include "coretemp.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <string>

#define PATH_SIZE 128

static const char SYS_HWMON[] = "/sys/class/hwmon";
static const char SYS_CORETEMP[] = "/sys/devices/platform/coretemp";
static const char SYS_VIATEMP[] = "/sys/devices/platform/via_cputemp";


CoreTemp::CoreTemp( XOSView *parent, const char *label, const char *caption, int pkg, int cpu )
  : FieldMeter( parent, 3, label, caption, 1, 1, 1 ), _pkg(pkg), _cpu(cpu) {
  metric_ = true;
  _high = 0;
}

CoreTemp::~CoreTemp( void ) {

}

void CoreTemp::checkResources( void ) {
  FieldMeter::checkResources();

  _actcolor  = parent_->allocColor( parent_->getResource( "coretempActColor" ) );
  _highcolor = parent_->allocColor( parent_->getResource( "coretempHighColor") );
  setfieldcolor( 0, _actcolor );
  setfieldcolor( 1, parent_->getResource( "coretempIdleColor") );
  setfieldcolor( 2, _highcolor );
  priority_ = atoi( parent_->getResource( "coretempPriority" ) );
  SetUsedFormat( parent_->getResource( "coretempUsedFormat" ) );

  findSysFiles();
  if ( _cpus.empty() ) {  // should not happen at this point
    std::cerr << "BUG: Could not determine sysfs file(s) for coretemp." << std::endl;
    parent_->done(1);
  }

  // Get TjMax and use it for total, if available.
  // Not found on k8temp and via-cputemp.
  std::ifstream file;
  std::string dummy = _cpus.front();
  dummy.replace(dummy.find_last_of('_'), 6, "_crit");
  file.open(dummy.c_str());
  if ( file.good() ) {
    file >> total_;
    file.close();
    total_ /= 1000.0;
  }
  else
    total_ = atoi( parent_->getResourceOrUseDefault("coretempHighest", "100") );

  // Use tTarget/tCtl (when maximum cooling needs to be turned on) as high,
  // if found. On older Cores this MSR is empty and kernel sets this equal to
  // tjMax. Not found on k8temp and via-cputemp. On k10temp this is fixed value.
  char l[32];
  dummy = _cpus.front();
  dummy.replace(dummy.find_last_of('_'), 6, "_max");
  file.open(dummy.c_str());
  if ( file.good() ) {
    file >> _high;
    file.close();
    _high /= 1000.0;
    snprintf(l, 32, "ACT(\260C)/%d/%d", (int)_high, (int)total_);
  }
  else
    _high = total_;

  if (_high == total_) {  // No tTarget/tCtl
    // Use user-defined high, or "no high".
    const char *high = parent_->getResourceOrUseDefault("coretempHigh", NULL);
    if (high) {
      _high = atoi(high);
      snprintf(l, 32, "ACT(\260C)/%d/%d", (int)_high, (int)total_);
    }
    else
      snprintf(l, 32, "ACT(\260C)/HIGH/%d", (int)total_);
  }
  legend(l);
}

/* Find absolute paths of files to be used. */
void CoreTemp::findSysFiles( void ) {
  int cpu = 0;
  unsigned int i = 0, cpucount = countCores(_pkg);
  char name[PATH_SIZE];
  std::string dummy;
  std::ifstream file;
  glob_t gbuf;
  DIR *dir;
  struct dirent *dent;
  struct stat buf;

  // Intel and VIA CPUs.
  // Platform device sysfs node changed in kernel 3.15 -> try both paths.
  snprintf(name, PATH_SIZE, "%s.%d/temp*_label", SYS_CORETEMP, _pkg);
  glob(name, 0, NULL, &gbuf);
  snprintf(name, PATH_SIZE, "%s.%d/hwmon/hwmon*/temp*_label", SYS_CORETEMP, _pkg);
  glob(name, GLOB_APPEND, NULL, &gbuf);
  snprintf(name, PATH_SIZE, "%s.%d/temp*_label", SYS_VIATEMP, _pkg);
  glob(name, GLOB_APPEND, NULL, &gbuf);
  snprintf(name, PATH_SIZE, "%s.%d/hwmon/hwmon*/temp*_label", SYS_VIATEMP, _pkg);
  glob(name, GLOB_APPEND, NULL, &gbuf);
  for (i = 0; i < gbuf.gl_pathc; i++) {
    file.open(gbuf.gl_pathv[i]);
    file >> dummy >> cpu;  // "Core n" or "Physical id n"
    file.close();
    if ( strncmp(dummy.c_str(), "Core", 4) == 0 ) {
      strcpy(strrchr(gbuf.gl_pathv[i], '_'), "_input");
      if (_cpu < 0 || cpu == _cpu)
        _cpus.push_back(gbuf.gl_pathv[i]);
    }
  }
  globfree(&gbuf);
  if ( !_cpus.empty() )
    return;

  // AMD CPUs.
  if ( !(dir = opendir(SYS_HWMON)) ) {
    std::cerr << "Can not open " << SYS_HWMON << " directory." << std::endl;
    parent_->done(1);
    return;
  }
  while ( (dent = readdir(dir)) ) {
    if ( !strncmp(dent->d_name, ".", 1) ||
         !strncmp(dent->d_name, "..", 2) )
      continue;

    snprintf(name, PATH_SIZE, "%s/%s/name", SYS_HWMON, dent->d_name);
    file.open(name);
    if (!file) {
      // Older kernels place the name in device directory.
      snprintf(name, PATH_SIZE, "%s/%s/device/name", SYS_HWMON, dent->d_name);
      file.open(name);
      if (!file)
        continue;
    }
    file >> dummy;
    file.close();

    if ( strncmp(dummy.c_str(), "k8temp", 6) == 0 ||
         strncmp(dummy.c_str(), "k10temp", 7) == 0 ) {
      if (cpu++ != _pkg)
        continue;
      // K8 core has two sensors with index starting from 1
      // K10 has only one sensor per physical CPU
      if (_cpu < 0) {  // avg or max
        for (i = 1; i <= cpucount; i++) {
          snprintf(name, PATH_SIZE, "%s/%s/temp%d_input",
                   SYS_HWMON, dent->d_name, i);
          if (!( stat(name, &buf) == 0 && S_ISREG(buf.st_mode) ))
            snprintf(name, PATH_SIZE, "%s/%s/device/temp%d_input",
                     SYS_HWMON, dent->d_name, i);
          _cpus.push_back(name);
        }
      }
      else {  // single sensor
        snprintf(name, PATH_SIZE, "%s/%s/temp%d_input",
                 SYS_HWMON, dent->d_name, _cpu + 1);
        if (!( stat(name, &buf) == 0 && S_ISREG(buf.st_mode) ))
          snprintf(name, PATH_SIZE, "%s/%s/device/temp%d_input",
                   SYS_HWMON, dent->d_name, i);
        _cpus.push_back(name);
      }
    }
  }
  closedir(dir);
}

void CoreTemp::checkevent( void ) {
  getcoretemp();
  drawfields();
}

void CoreTemp::getcoretemp( void ) {
  std::ifstream file;
  double dummy;
  fields_[0] = 0.0;

  if (_cpu >= 0) {  // only one core
    file.open(_cpus.back().c_str());
    if (!file) {
      std::cerr << "Can not open file : " << _cpus.back().c_str() << std::endl;
      parent_->done(1);
      return;
    }
    file >> fields_[0];
    file.close();
  }
  else if (_cpu == -1) {  // average
    for (uint i = 0; i < _cpus.size(); i++) {
      file.open(_cpus[i].c_str());
      if (!file) {
        std::cerr << "Can not open file : " << _cpus[i].c_str() << std::endl;
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
      file.open(_cpus[i].c_str());
      if (!file) {
        std::cerr << "Can not open file : " << _cpus[i].c_str() << std::endl;
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

  fields_[0] /= 1000.0;
  setUsed( fields_[0], total_ );

  if (fields_[0] < 0)
    fields_[0] = 0.0;
  fields_[1] = _high - fields_[0];
  if (fields_[1] < 0) { // alarm: T > high
    fields_[1] = 0;
    if (colors_[0] != _highcolor) {
      setfieldcolor( 0, _highcolor );
      drawlegend();
    }
  }
  else {
    if (colors_[0] != _actcolor) {
      setfieldcolor( 0, _actcolor );
      drawlegend();
    }
  }

  fields_[2] = total_ - fields_[1] - fields_[0];
  if (fields_[2] < 0)
    fields_[2] = 0;
}

/* Count sensors available to coretemp in the given package. */
unsigned int CoreTemp::countCores( unsigned int pkg )
{
  glob_t gbuf;
  char s[PATH_SIZE];
  unsigned int i, count = 0, cpu = 0;
  DIR *dir;
  struct dirent *dent;
  struct stat buf;
  std::string dummy;
  std::ifstream file;

  // Intel or VIA CPU.
  // Platform device sysfs node changed in kernel 3.15 -> try both paths.
  snprintf(s, PATH_SIZE, "%s.%d/temp*_label", SYS_CORETEMP, pkg);
  glob(s, 0, NULL, &gbuf);
  snprintf(s, PATH_SIZE, "%s.%d/hwmon/hwmon*/temp*_label", SYS_CORETEMP, pkg);
  glob(s, GLOB_APPEND, NULL, &gbuf);
  snprintf(s, PATH_SIZE, "%s.%d/temp*_label", SYS_VIATEMP, pkg);
  glob(s, GLOB_APPEND, NULL, &gbuf);
  snprintf(s, PATH_SIZE, "%s.%d/hwmon/hwmon*/temp*_label", SYS_VIATEMP, pkg);
  glob(s, GLOB_APPEND, NULL, &gbuf);
  // loop through paths in gbuf and check if it is a core or package
  for (i = 0; i < gbuf.gl_pathc; i++) {
    file.open(gbuf.gl_pathv[i]);
    file >> dummy;
    file.close();
    if ( strncmp(dummy.c_str(), "Core", 4) == 0 )
      count++;
  }
  globfree(&gbuf);
  if (count > 0)
    return count;

  // AMD CPU.
  if ( !(dir = opendir(SYS_HWMON)) )
    return 0;
  // loop through hwmon devices and when AMD sensor is found, count its inputs
  while ( (dent = readdir(dir)) ) {
    if ( !strncmp(dent->d_name, ".", 1) ||
         !strncmp(dent->d_name, "..", 2) )
      continue;
    snprintf(s, PATH_SIZE, "%s/%s/name", SYS_HWMON, dent->d_name);
    if (!( stat(s, &buf) == 0 && S_ISREG(buf.st_mode) ))
      // Older kernels place the name in device directory.
      snprintf(s, PATH_SIZE, "%s/%s/device/name", SYS_HWMON, dent->d_name);

    file.open(s);
    if ( file.good() ) {
      file >> dummy;
      file.close();
      if ( strncmp(dummy.c_str(), "k8temp", 6) == 0 ||
           strncmp(dummy.c_str(), "k10temp", 7) == 0 ) {
        if (cpu++ < pkg)
          continue;
        snprintf(s, PATH_SIZE, "%s/%s/temp*_input", SYS_HWMON, dent->d_name);
        glob(s, 0, NULL, &gbuf);
        snprintf(s, PATH_SIZE, "%s/%s/device/temp*_input", SYS_HWMON, dent->d_name);
        glob(s, GLOB_APPEND, NULL, &gbuf);
        count += gbuf.gl_pathc;
        globfree(&gbuf);
      }
    }
  }
  closedir(dir);
  return count;
}

/* Count physical CPUs with sensors. */
unsigned int CoreTemp::countCpus( void )
{
  glob_t gbuf;
  char s[PATH_SIZE];
  unsigned int count = 0;
  DIR *dir;
  struct dirent *dent;
  struct stat buf;
  std::string dummy;
  std::ifstream file;

  // Count Intel and VIA packages.
  snprintf(s, PATH_SIZE, "%s.*", SYS_CORETEMP);
  glob(s, 0, NULL, &gbuf);
  snprintf(s, PATH_SIZE, "%s.*", SYS_VIATEMP);
  glob(s, GLOB_APPEND, NULL, &gbuf);
  count += gbuf.gl_pathc;
  globfree(&gbuf);
  if (count > 0)
    return count;

  // Count AMD packages.
  if ( !(dir = opendir(SYS_HWMON)) )
    return 0;
  while ( (dent = readdir(dir)) ) {
    if ( !strncmp(dent->d_name, ".", 1) ||
         !strncmp(dent->d_name, "..", 2) )
      continue;
    snprintf(s, PATH_SIZE, "%s/%s/name", SYS_HWMON, dent->d_name);
    if (!( stat(s, &buf) == 0 && S_ISREG(buf.st_mode) ))
      // Older kernels place the name in device directory.
      snprintf(s, PATH_SIZE, "%s/%s/device/name", SYS_HWMON, dent->d_name);
    file.open(s);
    if ( file.good() ) {
      file >> dummy;
      file.close();
      if ( strncmp(dummy.c_str(), "k8temp", 6) == 0 ||
           strncmp(dummy.c_str(), "k10temp", 7) == 0 )
        count++;
    }
  }
  closedir(dir);
  return count;
}
