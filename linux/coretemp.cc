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

#define PATH_SIZE 128

static const char SYS_HWMON[] = "/sys/class/hwmon";
static const char SYS_CORETEMP[] = "/sys/devices/platform/coretemp";


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

  unsigned int cpucount = countCpus(_pkg);
  char name[PATH_SIZE];
  std::string dummy;
  std::ifstream file;
  DIR *dir;
  struct dirent *dent;

  dir = opendir(SYS_HWMON);
  if (!dir) {
    std::cerr << "Can not open " << SYS_HWMON << " directory." << std::endl;
    parent_->done(1);
    return;
  }

  while ( (dent = readdir(dir)) ) {
    if ( !strncmp(dent->d_name, ".", 1) ||
         !strncmp(dent->d_name, "..", 2) )
      continue;

    snprintf(name, PATH_SIZE, "%s/%s/device/name", SYS_HWMON, dent->d_name);
    file.open(name);
    if (!file)
      continue;
    file >> dummy;
    file.close();

    if (strncmp(dummy.c_str(), "k8temp", 6) == 0) {
      // each core has two sensors with index starting from 1
      if (_cpu < 0) {  // avg or max
        for (uint i = 1; i <= cpucount; i++) {
          snprintf(name, PATH_SIZE, "%s/%s/device/temp%d_input", SYS_HWMON, dent->d_name, i);
          _cpus.push_back(name);
        }
      }
      else {  // single sensor
        snprintf(name, PATH_SIZE, "%s/%s/device/temp%d_input", SYS_HWMON, dent->d_name, _cpu + 1);
        _cpus.push_back(name);
      }
    }
    else if (strncmp(dummy.c_str(), "coretemp", 8) == 0) {
      int i = 0, cpu = 0;
      // Gather the sysfs indices which are not the same as core numbers.
      // Core numbers are not unique across physical CPUs, but sysfs indices are.
      while (_cpus.size() != cpucount) {
        snprintf(name, PATH_SIZE, "%s.%d/temp%d_label", SYS_CORETEMP, _pkg, i);
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
        snprintf(name, PATH_SIZE, "%s.%d/temp%d_input", SYS_CORETEMP, _pkg, i++);
        if (_cpu < 0)
          _cpus.push_back(name);
        else if (cpu == _cpu) {
          _cpus.push_back(name);
          break;
        }
      }
    }
    else if (strncmp(dummy.c_str(), "k10temp", 7) == 0) {
      _cpu = 1;
      snprintf(name, PATH_SIZE, "%s/%s/device/temp%d_input", SYS_HWMON, dent->d_name, _cpu);
      _cpus.push_back(name);  // K10 has only one sensor per physical CPU
    }

    if (_cpus.empty())
      continue;

    // sysfs node was found
    // get TjMax and use it for total if available
    dummy = _cpus.front();
    dummy.replace(dummy.find_last_of('_'), 6, "_crit");
    file.open(dummy.c_str());
    if (file) {
      file >> total_;
      file.close();
      total_ /= 1000.0;
    }
    else
      total_ = atoi(parent_->getResourceOrUseDefault("coretempHighest", "100"));

    // Use tTarget (when maximum cooling needs to be turned on) as high, if found.
    // On older Cores this MSR is empty and kernel sets this equal to tjMax.
    // This is the same for all cores in package.
    // Not found on k8temp. On k10temp this is fixed value.
    char l[32];
    std::string ttarget = _cpus.front();
    ttarget.replace(ttarget.find_last_of('_'), 6, "_max");
    file.open(ttarget.c_str());
    if (file) {
      file >> _high;
      file.close();
      _high /= 1000.0;
      snprintf(l, 32, "ACT(\260C)/%d/%d", (int)_high, (int)total_);
    }
    else
      _high = total_;

    if (_high == total_) {  // No tTarget
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
    closedir(dir);
    return;
  }
  std::cerr << "You do not seem to have CPU temperature sensor available." << std::endl;
  parent_->done(1);
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

  setUsed( fields_[0], total_ );
}

/* Count sensors available to coretemp in the given package. */
unsigned int CoreTemp::countCpus(int pkg)
{
  glob_t gbuf;
  char s[PATH_SIZE];
  struct stat sbuf;
  int count = 0;
  std::string dummy;
  std::ifstream file;

  snprintf(s, PATH_SIZE, "%s.%d", SYS_CORETEMP, pkg);
  if ( stat(s, &sbuf) == 0 && S_ISDIR(sbuf.st_mode) ) {
    strncat(s, "/temp*_label", PATH_SIZE - strlen(s) - 1);
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
      if ( !strncmp(dent->d_name, ".", 1) ||
           !strncmp(dent->d_name, "..", 2) )
        continue;

      snprintf(s, PATH_SIZE, "%s/%s/device/name", SYS_HWMON, dent->d_name);
      file.open(s);
      if (file.good()) {
        file >> dummy;
        file.close();
        if ( strncmp(dummy.c_str(), "k8temp", 6) == 0 ||
             strncmp(dummy.c_str(), "k10temp", 7) == 0 ) {
          snprintf(s, PATH_SIZE, "%s/%s/device/temp*_input", SYS_HWMON, dent->d_name);
          glob(s, 0, NULL, &gbuf);
          count += gbuf.gl_pathc;
          globfree(&gbuf);
        }
      }
    }
    closedir(dir);
  }
  return count;
}
