//
//  Copyright (c) 2000, 2006 by Leopold Toetsch <lt@toetsch.at>
//
//  Read temperature entries from /proc/sys/dev/sensors/*/*
//  and display actual and high temperature
//  if actual >= high, actual temp changes color to indicate alarm
//
//  File based on btrymeter.* by
//  Copyright (c) 1997 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//
//
#include "lmstemp.h"
#include "xosview.h"
#include <fstream>
#include <math.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

static const char PROC_SENSORS_24[] = "/proc/sys/dev/sensors";
static const char PROC_SENSORS_26[] = "/sys/class/hwmon";


LmsTemp::LmsTemp( XOSView *parent, const char *tempfile, const char *highfile, const char *label, const char *caption)
  : FieldMeter( parent, 3, label, caption, 1, 1, 0 ){
  _highfile = NULL;
  _tempfile = new char[PATH_MAX];
  if (!checksensors(1, PROC_SENSORS_24, tempfile, highfile)) {
    if (!checksensors(0, PROC_SENSORS_26, tempfile, highfile)) {
      std::cerr << label << " : Can not find file ";
      if (tempfile[0] == '/') {
        std::cerr << tempfile;
        if (highfile) {
          if (highfile[0] == '/')
            std::cerr << " or " << highfile;
          else
            std::cerr << ", or " << highfile << " under " << PROC_SENSORS_24 << " or " << PROC_SENSORS_26;
        }
      }
      else {
        if (highfile) {
          if (highfile[0] == '/')
            std::cerr << tempfile << " under " << PROC_SENSORS_24 << " or " << PROC_SENSORS_26 << ", or " << highfile;
          else
            std::cerr << tempfile << " or " << highfile << " under " << PROC_SENSORS_24 << " or " << PROC_SENSORS_26;
        }
        else
          std::cerr << tempfile << " under " << PROC_SENSORS_24 << " or " << PROC_SENSORS_26;
      }
      std::cerr << "." << std::endl;
      parent_->done(1);
    }
  }
}

LmsTemp::~LmsTemp( void ){
  delete[] _tempfile;
  if (_highfile)
    delete[] _highfile;
}

/* this part is adapted from ProcMeter3.2 */
bool LmsTemp::checksensors(int isproc, const char *dir, const char* tempfile, const char *highfile) {
  bool temp_found = false, high_found = false;
  DIR *d1, *d2;
  struct dirent *ent1, *ent2;
  struct stat buf;

  /* First, check if absolute paths were given. */
  if (tempfile[0] == '/') {
    if (stat(tempfile, &buf) == 0 && S_ISREG(buf.st_mode)) {
      strncpy(_tempfile, tempfile, PATH_MAX);
      temp_found = true;
    }
    else
      return false;
  }
  if (!highfile)
    high_found = true;
  else {
    if (highfile[0] == '/') {
      if (stat(highfile, &buf) == 0 && S_ISREG(buf.st_mode)) {
        _highfile = new char[PATH_MAX];
        strncpy(_highfile, highfile, PATH_MAX);
        high_found = true;
      }
      else
        return false;
    }
  }
  if (temp_found && high_found) {
    _isproc = ( strncmp(_tempfile, "/proc", 5) ? 0 : 1 );
    return true;
  }

  /* Then, try to find the given file. */
  d1 = opendir(dir);
  if (!d1)
    return false;
  else {
    char dirname[64];

    while ( !(temp_found && high_found) && (ent1 = readdir(d1)) ) {
      if ( !strncmp(ent1->d_name, ".", 1) ||
           !strncmp(ent1->d_name, "..", 2) )
        continue;

      snprintf(dirname, 64, "%s/%s", dir, ent1->d_name);
      for (int i = 0; i < (isproc ? 1 : 2); i++) {
        if ( stat(dirname, &buf) == 0 && S_ISDIR(buf.st_mode) ) {
          d2 = opendir(dirname);
          if (!d2)
            std::cerr << "The directory " << dirname
                      << "exists but cannot be read." << std::endl;
          else {
            while ((ent2 = readdir(d2))) {
              if (!strncmp(ent2->d_name, ".", 1) ||
                  !strncmp(ent2->d_name, "..", 2) )
                continue;
              char f[80];
              snprintf(f, 80, "%s/%s", dirname, ent2->d_name);
              if ( stat(f, &buf) != 0 || !S_ISREG(buf.st_mode) )
                continue;

              if (isproc) {
                if ( !strncmp(ent2->d_name, tempfile, strlen(tempfile)) ) {
                  temp_found = true;
                  high_found = true;
                  strncpy(_tempfile, f, PATH_MAX);
                }
              }
              else {
                // check for tempfile and tempfile_input
                if ( !strncmp(ent2->d_name, tempfile, strlen(tempfile)) ) {
                  if ( !temp_found && ( strlen(tempfile) == strlen(ent2->d_name) ||
                      !strncmp(ent2->d_name + strlen(tempfile), "_input", 6) ) ) {
                    std::cout << f << std::endl;
                    strncpy(_tempfile, f, PATH_MAX);
                    temp_found = true;
                  }
                  // check for tempfile_max as highfile, if no highfile was given
                  if ( !highfile && !strncmp(ent2->d_name + strlen(tempfile), "_max", 4) ) {
                    std::cout << f << std::endl;
                    _highfile = new char[PATH_MAX];
                    strncpy(_highfile, f, PATH_MAX);
                    high_found = true;
                  }
                }
                // check for highfile and highfile_max
                if ( !high_found && !strncmp(ent2->d_name, highfile, strlen(highfile)) ) {
                  if ( strlen(highfile) == strlen(ent2->d_name) ||
                      !strncmp(ent2->d_name + strlen(highfile), "_max", 4) ) {
                    std::cout << f << std::endl;
                    _highfile = new char[PATH_MAX];
                    strncpy(_highfile, f, PATH_MAX);
                    high_found = true;
                  }
                }
              }
            }
            closedir(d2);
          }
        }

        if (temp_found && high_found) {
          _isproc = isproc;
          closedir(d1);
          return true;
        }
        // Some /sys sensors have the readings in subdirectory /device
        if (!isproc)
          strncat(dirname, "/device", sizeof(dirname) - strlen(dirname) - 1);
      }
    }
  }
  closedir(d1);
  return (temp_found & high_found);
}

void LmsTemp::checkResources( void ){
  FieldMeter::checkResources();

  _actcolor  = parent_->allocColor( parent_->getResource( "lmstempActColor" ) );
  _highcolor = parent_->allocColor( parent_->getResource( "lmstempHighColor" ) );
  setfieldcolor( 0, _actcolor );
  setfieldcolor( 1, parent_->getResource( "lmstempIdleColor") );
  setfieldcolor( 2, _highcolor );
  total_ = atoi( parent_->getResourceOrUseDefault( "lmstempHighest", "100" ) );
  priority_ = atoi (parent_->getResource( "lmstempPriority" ) );
  SetUsedFormat(parent_->getResource( "lmstempUsedFormat" ) );
}

void LmsTemp::checkevent( void ){
  getlmstemp();

  drawfields();
}

// Note:
// procentry looks like
// high low actual
//
// if actual >= high alarm is triggered, fan starts and high is set to
//   a higher value by BIOS
//   after fan cooled down the chips, high get's reset
// if  this happens display color of actual is set to HighColor
// this could be very machine depended
//
// a typical entry on my machine (Gericom Overdose 2 XXL, PIII 600) looks like:
//
// $ sensors
// max1617-i2c-0-4e
// Adapter: SMBus PIIX4 adapter at 1400
// Algorithm: Non-I2C SMBus adapter
// temp:       52 C (limit:   55 C, hysteresis:  -55 C)
// remote_temp:
//             56 C (limit:   90 C, hysteresis:  -55 C)
//
// after alarm limits are set to 60 / 127 respectively
// low/hysteresis looks broken ;-)
//

void LmsTemp::getlmstemp( void ){
  // dummy, high changed from integer to double to allow it to display
  // the full value, unfit for an int. (See Debian bug #183695)
  double dummy, high;
  bool do_legend = false;

  std::ifstream tempfile( _tempfile );
  if (!tempfile) {
    std::cerr << "Can not open file : " << _tempfile << std::endl;
    parent_->done(1);
    return;
  }

  if (_isproc) {
    tempfile >> high >> dummy >> fields_[0];
  }
  else {
    tempfile >> fields_[0];
    fields_[0] /= 1000;
    if (_highfile) {
      std::ifstream highfile( _highfile );
      if (!highfile) {
        std::cerr << "Can not open file : " << _highfile << std::endl;
        parent_->done(1);
        return;
      }
      highfile >> high;
      high /= 1000;
    }
    else
      high = total_;
  }

  if ( high > total_ || high != _high ) {
    char l[16];
    if ( high > total_ )
      total_ = 10 * (int)((high * 1.25) / 10);
    _high = high;
    if ( _highfile )
      snprintf(l, 16, "ACT/%d/%d", (int)high, (int)total_);
    else
      snprintf(l, 16, "ACT/HIGH/%d", (int)total_);
    legend(l);
    do_legend = true;
  }

  fields_[1] = high - fields_[0];
  if (fields_[1] < 0) { // alarm: T > max
    fields_[1] = 0;
    if (colors_[0] != _highcolor) {
      setfieldcolor( 0, _highcolor );
      do_legend = true;
    }
  }
  else {
    if (colors_[0] != _actcolor) {
      setfieldcolor( 0, _actcolor );
      do_legend = true;
    }
  }

  fields_[2] = total_ - fields_[1] - fields_[0];
  if (fields_[2] < 0)
    fields_[2] = 0;

  setUsed(fields_[0], total_);

  if (do_legend)
    drawlegend();
}
