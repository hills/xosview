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
  _highfile = new char[PATH_MAX];
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
  _high = 0;
  const char *p;
  if ((p = strrchr(caption,'/')) != 0)
    total_ = atoi(p+1);
  else
    total_ = 100;
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
  int j = (isproc ? 1 : 2);

  if (tempfile[0] == '/') {
    if (stat(tempfile, &buf) == 0)
      temp_found = true;
    else
      return false;
  }
  if (!highfile)
    high_found = true;
  else {
    if (highfile[0] == '/') {
      if (stat(highfile, &buf) == 0)
        high_found = true;
      else
        return false;
    }
  }
  if (temp_found && high_found) {
    strcpy(_tempfile, tempfile);
    if (highfile)
      strcpy(_highfile, highfile);
    else {
      delete[] _highfile;
      _highfile = NULL;
    }
    _isproc = strncmp(_tempfile, "/proc", 5) == 0 ? 1 : 0;
    return true;
  }

  d1=opendir(dir);
  if(!d1)
    return false;
  else
    {
      char dirname[64];

    while (!temp_found && !high_found && (ent1 = readdir(d1))) {
	  if(!strncmp(ent1->d_name,".", 1))
	    continue;
	  if(!strncmp(ent1->d_name,"..", 2))
	    continue;

	  snprintf(dirname, 64, "%s/%s",dir ,ent1->d_name);
      for (int i = 0; i < j; i++) {
        if (stat(dirname, &buf) == 0 && S_ISDIR(buf.st_mode)) {
	      d2=opendir(dirname);
	      if(!d2)
		std::cerr << "The directory " <<dirname
                  <<"exists but cannot be read.\n";
	      else
		{
		  while((ent2=readdir(d2)))
		    {

		      char f[80];
		      snprintf(f, 80, "%s/%s",dirname,ent2->d_name);
		      if(stat(f,&buf)!=0 || !S_ISREG(buf.st_mode))
			continue;

              if (isproc) {
                if (!strncmp(ent2->d_name, tempfile, strlen(tempfile))) {
                  temp_found = true;
                  high_found = true;
                  strcpy(_tempfile, f);
                }
              }
              else {
                if (!strncmp(ent2->d_name, tempfile, strlen(tempfile)) &&
                    !strncmp(ent2->d_name + strlen(tempfile), "_input", 6)) {
                  strcpy(_tempfile, f);
                  temp_found = true;
                }
                if (!high_found && (!strncmp(ent2->d_name, highfile, strlen(highfile)) &&
                    !strncmp(ent2->d_name + strlen(highfile), "_max", 4))) {
                  strcpy(_highfile, f);
                  high_found = true;
                }
              }
              if (temp_found && high_found) {
			  _isproc = isproc;
                closedir(d2);
                closedir(d1);
                return true;
			}
		    }
		  closedir(d2);
		}
	    }

        // Some /sys sensors have the readings in subdirectory /device
        if (!isproc)
          strncat(dirname, "/device", sizeof(dirname) - strlen(dirname) - 1);
      }
	}
    }
  closedir(d1);
  return (temp_found && high_found);
}

void LmsTemp::checkResources( void ){
  FieldMeter::checkResources();

  _actcolor  = parent_->allocColor( parent_->getResource( "lmstempActColor" ) );
  _highcolor = parent_->allocColor( parent_->getResource( "lmstempHighColor" ) );
  setfieldcolor( 0, _actcolor );
  setfieldcolor( 1, parent_->getResource( "lmstempIdleColor") );
  setfieldcolor( 2, _highcolor );
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
  if(fields_[1] < 0) { // alarm: T > max
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
