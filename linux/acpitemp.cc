//
//  Copyright (c) 2009 by Tomi Tapper <tomi.o.tapper@jyu.fi>
//
//  File based on lmstemp.* by
//  Copyright (c) 2000, 2006 by Leopold Toetsch <lt@toetsch.at>
//
//  This file may be distributed under terms of the GPL
//
//
//

#include "acpitemp.h"
#include "xosview.h"
#include <fstream>
#include <math.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>


static const char PROC_ACPI_TZ[] = "/proc/acpi/thermal_zone";
static const char SYS_ACPI_TZ[]  = "/sys/devices/virtual/thermal";


ACPITemp::ACPITemp( XOSView *parent, const char *tempfile, const char *highfile, const char *label, const char *caption )
: FieldMeter( parent, 3, label, caption, 1, 1, 0 ) {
  if (!checkacpi(tempfile, highfile)) {
    std::cerr << "Can not find file : ";
    if (tempfile[0] == '/' && highfile[0] == '/')
      std::cerr << tempfile << " or " << highfile;
    else if (tempfile[0] == '/')
      std::cerr << tempfile << ", or " << highfile << " under " << PROC_ACPI_TZ << " or " << SYS_ACPI_TZ;
    else if (highfile[0] == '/')
      std::cerr << tempfile << " under " << PROC_ACPI_TZ << " or " << SYS_ACPI_TZ << ", or " << highfile;
    else
      std::cerr << tempfile << " or " << highfile << " under " << PROC_ACPI_TZ << " or " << SYS_ACPI_TZ;
    std::cerr << "." << std::endl;
    parent_->done(1);
  }
  _high = 0;
  const char *p;
  if ((p = strrchr(caption, '/')) != 0)
    total_ = atoi(p+1);
  else
    total_ = 100;
}

ACPITemp::~ACPITemp( void ) {

}

int ACPITemp::checkacpi( const char *tempfile, const char *highfile ) {
  struct stat buf;
  char temp[PATH_SIZE], high[PATH_SIZE];
  bool temp_found = false, high_found = false;

  if (tempfile[0] == '/') {
    if ( stat(tempfile, &buf) == 0 && S_ISREG(buf.st_mode) )
      temp_found = true;
    else
      return false;
  }
  if (highfile[0] == '/') {
    if ( stat(highfile, &buf) == 0 && S_ISREG(buf.st_mode) )
      high_found = true;
    else
      return false;
  }

  if (temp_found && high_found) {
    strncpy(_tempfile, tempfile, PATH_SIZE);
    strncpy(_highfile, highfile, PATH_SIZE);
    return true;
  }

  snprintf(temp, PATH_SIZE, "%s/%s", SYS_ACPI_TZ, tempfile);
  snprintf(high, PATH_SIZE, "%s/%s", SYS_ACPI_TZ, highfile);

  if ( (stat(temp, &buf) == 0 && S_ISREG(buf.st_mode)) &&
       (stat(high, &buf) == 0 && S_ISREG(buf.st_mode)) ) {
    strncpy(_tempfile, temp, PATH_SIZE);
    strncpy(_highfile, high, PATH_SIZE);
    _usesysfs = true;
    return true;
  }

  _usesysfs = false;
  snprintf(temp, PATH_SIZE, "%s/%s", PROC_ACPI_TZ, tempfile);
  snprintf(high, PATH_SIZE, "%s/%s", PROC_ACPI_TZ, highfile);

  if ( (stat(temp, &buf) == 0 && S_ISREG(buf.st_mode)) &&
       (stat(high, &buf) == 0 && S_ISREG(buf.st_mode)) ) {
    strncpy(_tempfile, temp, PATH_SIZE);
    strncpy(_highfile, high, PATH_SIZE);
    return true;
  }
  return false;
}

void ACPITemp::checkResources( void ) {
  FieldMeter::checkResources();

  _actcolor  = parent_->allocColor( parent_->getResource( "acpitempActColor" ) );
  _highcolor = parent_->allocColor( parent_->getResource( "acpitempHighColor" ) );
  setfieldcolor( 0, _actcolor );
  setfieldcolor( 1, parent_->getResource( "acpitempIdleColor") );
  setfieldcolor( 2, _highcolor );
  priority_ = atoi( parent_->getResource( "acpitempPriority" ) );
  SetUsedFormat( parent_->getResource( "acpitempUsedFormat" ) );
}

void ACPITemp::checkevent( void ) {
  getacpitemp();
  drawfields();
}

void ACPITemp::getacpitemp( void ) {
  std::ifstream temp_file(_tempfile);
  std::ifstream high_file(_highfile);

  if (!temp_file) {
    std::cerr << "Can not open file : " << temp_file << std::endl;
    parent_->done(1);
    return;
  }
  if (!high_file) {
    std::cerr << "Can not open file : " << high_file << std::endl;
    parent_->done(1);
    return;
  }

  double high;
  char l[20];
  std::string s1, s2, s3;

  if (_usesysfs) {
    high_file >> s1;
    high = atof(s1.c_str()) / 1000.0;
    temp_file >> s1;
    fields_[0] = atof(s1.c_str()) / 1000.0;
  }
  else {
    high_file >> s1 >> s2 >> s3;
    high = atof(s3.c_str());
    temp_file >> s1 >> s2;
    fields_[0] = atof(s2.c_str());
  }

  if (high >= total_) {
    total_ = 10 * ceil((high * 1.25) / 10);
    snprintf(l, 20, "ACT/%d/%d", (int)high, (int)total_);
    legend(l);
    drawlegend();
  }

  fields_[1] = high - fields_[0];
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

  if (high != _high) {
    _high = high;
    snprintf(l, 20, "ACT/%d/%d", (int)high, (int)total_);
    legend(l);
    drawlegend();
  }
}
