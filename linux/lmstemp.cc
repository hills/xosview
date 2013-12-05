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
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

static const char PROC_SENSORS[] = "/proc/sys/dev/sensors";
static const char SYS_SENSORS[]  = "/sys/class/hwmon";


LmsTemp::LmsTemp( XOSView *parent, const char *name, const char *tempfile,
                  const char *highfile, const char *lowfile, const char *label,
                  const char *caption, unsigned int nbr )
  : FieldMeter( parent, 3, label, caption, 1, 1, 0 ){
  _nbr = nbr;
  _scale = 1.0;
  _isproc = false;
  _high = _low = 0.0;
  _name_found = _temp_found = _high_found = _low_found = _has_high = false;

  // Check if high is given as value
  if ( highfile && ('0' <= highfile[0] && highfile[0] <= '9') ) {
    _high = atof(highfile);
    _has_high = _high_found = true;
    highfile = NULL;
  }
  // Check if low is given as value
  if ( lowfile && ('0' <= lowfile[0] && lowfile[0] <= '9') ) {
    _low = atof(lowfile);
    _low_found = true;
    lowfile = NULL;
  }

  if ( !checksensors(name, tempfile, highfile, lowfile) ) {
    if ( !_name_found &&
         (( !_temp_found && tempfile[0] != '/' ) ||
          ( !_high_found && (highfile && highfile[0] != '/') ) ||
          ( !_low_found && (lowfile && lowfile[0] != '/') )) )
      std::cerr << label << " : No sensor named " << name << " was found in "
                << SYS_SENSORS << "." << std::endl;
    else {
      if (!_temp_found && tempfile[0] != '/') {
        std::cerr << label << " : Could not find file " << tempfile << "{,_input}";
        if (name)
          std::cerr << " under " << name << " in " << SYS_SENSORS;
        else
          std::cerr << " under " << PROC_SENSORS << " or " << SYS_SENSORS;
        std::cerr << "." << std::endl;
      }
      if (!_high_found && highfile && highfile[0] != '/') {
        std::cerr << label << " : Could not find file " << highfile;
        if (name)
          std::cerr << " under " << name << " in " << SYS_SENSORS;
        else
          std::cerr << " under " << SYS_SENSORS;
        std::cerr << "." << std::endl;
      }
      if (!_low_found && lowfile && lowfile[0] != '/') {
        std::cerr << label << " : Could not find file " << lowfile;
        if (name)
          std::cerr << " under " << name << " in " << SYS_SENSORS;
        else
          std::cerr << " under " << SYS_SENSORS;
        std::cerr << "." << std::endl;
      }
    }
    parent_->done(1);
  }
}

LmsTemp::~LmsTemp( void ){
}

bool LmsTemp::checksensors( const char *name, const char *tempfile,
                            const char *highfile, const char *lowfile ) {
// Logic:
//  0) tempfile must always be found, highfile and lowfile only if given
//  1) absolute path in any filename must match as is
//  2) if name is given, only files in that sysfs node can match
//  3) any filename matches as is
//  4) tempfile + "_input" matches and tempfile + "_max" and/or
//     tempfile + "_min" matches
  DIR *dir;
  struct dirent *ent;
  struct stat buf;
  std::string dirname, f, f2, n;

  /* First, check if absolute paths were given. */
  if (tempfile[0] == '/') {
    if ( stat(tempfile, &buf) == 0 && S_ISREG(buf.st_mode) ) {
      _tempfile = tempfile;
      _temp_found = true;
    }
    else
      std::cerr << title() << " : Could not find file " << tempfile << "." << std::endl;
  }
  if (highfile && highfile[0] == '/') {
    if ( stat(highfile, &buf) == 0 && S_ISREG(buf.st_mode) ) {
      _highfile = highfile;
      _high_found = true;
    }
    else
      std::cerr << title() << " : Could not find file " << highfile << "." << std::endl;
  }
  if (lowfile && lowfile[0] == '/') {
    if ( stat(lowfile, &buf) == 0 && S_ISREG(buf.st_mode) ) {
      _lowfile = lowfile;
      _low_found = true;
    }
    else
      std::cerr << title() << " : Could not find file " << lowfile << "." << std::endl;
  }

  if ( _temp_found && (_high_found || !highfile) && (_low_found || !lowfile) ) {
    _isproc = ( strncmp(_tempfile.c_str(), "/proc", 5) ? false : true );
    return true;
  }

  /* Then, try to find the given file. */
  /* Try /proc first. */
  if ( (dir = opendir(PROC_SENSORS)) ) {
    while ( !_temp_found && (ent = readdir(dir)) ) {
      if ( !strncmp(ent->d_name, ".", 1) ||
           !strncmp(ent->d_name, "..", 2) )
        continue;

      dirname = PROC_SENSORS;
      dirname += '/'; dirname += ent->d_name;
      if ( stat(dirname.c_str(), &buf) == 0 && S_ISDIR(buf.st_mode) ) {
        f = dirname + '/' + tempfile;
        if ( stat(f.c_str(), &buf) == 0 && S_ISREG(buf.st_mode) ) {
          _temp_found = true;
          _tempfile = f;
          _isproc = true;
        }
      }
    }
    closedir(dir);
    if (_temp_found)
      return true;
  }

  /* Next, try /sys. */
  if ( !(dir = opendir(SYS_SENSORS)) )
    return false;

  while ( !(_temp_found && (_high_found || !highfile) && (_low_found || !lowfile) ) &&
           (ent = readdir(dir)) ) {
    if ( !strncmp(ent->d_name, ".", 1) ||
         !strncmp(ent->d_name, "..", 2) )
      continue;

    // Try every node under /sys/class/hwmon
    dirname = SYS_SENSORS;
    dirname += '/'; dirname += ent->d_name;

    int i = 0;
    do {
      if ( stat(dirname.c_str(), &buf) == 0 && S_ISDIR(buf.st_mode) ) {
        // Try to get the sensor's name
        f = dirname + "/name";
        std::ifstream namefile( f.c_str() );
        if ( namefile.good() ) {
          namefile >> n;
          namefile.close();
        }
        if (!name || n == name) {
          // Either no name was given, or the name matches.
          // Check if the files exist here.
          _name_found = true;
          if (highfile && !_high_found) {
            f = dirname + '/' + highfile;
            if ( stat(f.c_str(), &buf) == 0 && S_ISREG(buf.st_mode) ) {
              _high_found = true;
              _highfile = f;
            }
          }
          if (lowfile && !_low_found) {
            f = dirname + '/' + lowfile;
            if ( stat(f.c_str(), &buf) == 0 && S_ISREG(buf.st_mode) ) {
              _low_found = true;
              _lowfile = f;
            }
          }
          f = dirname + '/' + tempfile;
          if ( stat(f.c_str(), &buf) == 0 && S_ISREG(buf.st_mode) ) {
            _temp_found = true;
            _tempfile = f;
          }
          else {
            f2 = f + "_input";
            if ( stat(f2.c_str(), &buf) == 0 && S_ISREG(buf.st_mode) ) {
              _temp_found = true;
              _tempfile = f2;
              f2 = f + "_max";
              if ( !_high_found && !highfile &&
                   stat(f2.c_str(), &buf) == 0 && S_ISREG(buf.st_mode) ) {
                _high_found = true;
                _highfile = f2;
              }
              f2 = f + "_min";
              if ( !_low_found && !lowfile &&
                   stat(f2.c_str(), &buf) == 0 && S_ISREG(buf.st_mode) ) {
                _low_found = true;
                _lowfile = f2;
              }
            }
          }
        }
      }

      // Some /sys sensors have the files in subdirectory /device
      dirname += "/device";
    } while ( ++i < 2 && !( _temp_found && (_high_found || !highfile) &&
                           (_low_found || !lowfile) ) );
  }
  closedir(dir);
  // No highfile or lowfile is OK.
  if (!highfile)
    _high_found = true;
  if (!lowfile)
    _low_found = true;

  return (_temp_found & _high_found & _low_found);
}

/* Adapted from libsensors. */
void LmsTemp::determineScale( void ){
  char type[16], subtype[32];
  int n;
  std::string basename = _tempfile.substr(_tempfile.find_last_of('/') + 1);
  if ( sscanf(basename.c_str(), "%[a-z]%d_%s", type, &n, subtype) == 3 ) {
    if (( !strncmp(type, "in", strlen(type)) &&
          !strncmp(subtype, "input", strlen(subtype)) ) ||
        ( !strncmp(type, "temp", strlen(type)) &&
          !strncmp(subtype, "input", strlen(subtype)) ) ||
        ( !strncmp(type, "temp", strlen(type)) &&
          !strncmp(subtype, "offset", strlen(subtype)) ) ||
        ( !strncmp(type, "curr", strlen(type)) &&
          !strncmp(subtype, "input", strlen(subtype)) ) ||
        ( !strncmp(type, "power", strlen(type)) &&
          !strncmp(subtype, "average_interval", strlen(subtype)) ) ||
        ( !strncmp(type, "cpu", strlen(type)) &&
          !strncmp(subtype, "vid", strlen(subtype)) )) {
      _scale = 1000.0;
      return;
    }
    if (( !strncmp(type, "power", strlen(type)) &&
          !strncmp(subtype, "average", strlen(subtype)) ) ||
        ( !strncmp(type, "energy", strlen(type)) &&
          !strncmp(subtype, "input", strlen(subtype)) )) {
      _scale = 1000000.0;
      return;
    }
  }
  _scale = 1.0;
}

void LmsTemp::updateLegend( void ){
  char l[16];
  if (_has_high) {
    if (total_ < 10)
      snprintf(l, 16, "ACT/%.1f/%.1f", _high, total_);
    else
      snprintf(l, 16, "ACT/%d/%d", (int)_high, (int)total_);
  }
  else {
    if (total_ < 10)
      snprintf(l, 16, "ACT/HIGH/%.1f", total_);
    else
      snprintf(l, 16, "ACT/HIGH/%d", (int)total_);
  }
  legend(l);
}

void LmsTemp::checkResources( void ){
  FieldMeter::checkResources();

  char s[32];
  const char *tmp = NULL;
  _actcolor  = parent_->allocColor( parent_->getResource( "lmstempActColor" ) );
  _highcolor = parent_->allocColor( parent_->getResource( "lmstempHighColor" ) );
  _lowcolor  = parent_->allocColor( parent_->getResource( "lmstempLowColor" ) );
  setfieldcolor( 0, _actcolor );
  setfieldcolor( 1, parent_->getResource( "lmstempIdleColor") );
  setfieldcolor( 2, _highcolor );
  tmp = parent_->getResourceOrUseDefault( "lmstempHighest", "100" );
  snprintf(s, 32, "lmstempHighest%d", _nbr);
  total_ = atof( parent_->getResourceOrUseDefault(s, tmp) );
  priority_ = atoi( parent_->getResource( "lmstempPriority" ) );
  tmp = parent_->getResource( "lmstempUsedFormat" );
  snprintf(s, 32, "lmstempUsedFormat%d", _nbr);
  SetUsedFormat( parent_->getResourceOrUseDefault(s, tmp) );

  // Sanity checks.
  if (_has_high) {
    if (_high > total_) {
      std::cerr << "Warning: lmshigh" << _nbr << " (" << _high
                << ") was greater than lmstempHighest" << _nbr << " ("
                << total_ << ")." << std::endl;
      _high = total_;
    }
    if (_low > _high) {
      std::cerr << "Warning: lmslow" << _nbr << " (" << _low
                << ") was greater than lmshigh" << _nbr << " (" << _high
                << ")." << std::endl;
      _low = _high;
    }
  }
  else
    _high = total_;

  if ( !_highfile.empty() )
    _has_high = true;

  determineScale();
  updateLegend();
}

void LmsTemp::checkevent( void ){
  getlmstemp();

  drawfields();
}

void LmsTemp::getlmstemp( void ){
  double high = _high;
  bool do_legend = false;

  std::ifstream tempfile( _tempfile.c_str() );
  if (!tempfile) {
    std::cerr << "Can not open file : " << _tempfile << std::endl;
    parent_->done(1);
    return;
  }

  if (_isproc)
    tempfile >> high >> _low >> fields_[0];
  else {
    tempfile >> fields_[0];
    fields_[0] /= _scale;
    if ( !_highfile.empty() ) {
      std::ifstream highfile( _highfile.c_str() );
      if (!highfile) {
        std::cerr << "Can not open file : " << _highfile << std::endl;
        parent_->done(1);
        return;
      }
      highfile >> high;
      high /= _scale;
    }
    if ( !_lowfile.empty() ) {
      std::ifstream lowfile( _lowfile.c_str() );
      if (!lowfile) {
        std::cerr << "Can not open file : " << _lowfile << std::endl;
        parent_->done(1);
        return;
      }
      lowfile >> _low;
      _low /= _scale;
    }
  }
  if (high != _high) {
    if (high > total_)
      total_ = 10 * (int)((high * 1.25) / 10);
    _high = high;
    updateLegend();
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
  else if (fields_[0] < _low) { // alarm: T < min
    if (colors_[0] != _lowcolor) {
      setfieldcolor( 0, _lowcolor );
      do_legend = true;
    }
  }
  else {
    if (colors_[0] != _actcolor) {
      setfieldcolor( 0, _actcolor );
      do_legend = true;
    }
  }

  setUsed(fields_[0], total_);
  fields_[2] = total_ - fields_[1] - fields_[0];
  if (fields_[2] < 0)
    fields_[2] = 0;
  if (fields_[0] > total_)
    fields_[0] = total_;

  if (do_legend)
    drawlegend();
}
