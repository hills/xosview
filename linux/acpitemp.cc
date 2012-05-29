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
    std::cerr << "Can not find " << tempfile << " or " << highfile << " in " << PROC_ACPI_TZ << " or " << SYS_ACPI_TZ << std::endl;
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
  char temp[80], high[80];
  bool temp_found = false, high_found = false;

  if (tempfile[0] == '/') {
    if (stat(tempfile, &buf) == 0)
      temp_found = true;
    else
      return false;
  }
  if (highfile[0] == '/') {
    if (stat(highfile, &buf) == 0)
      high_found = true;
    else
      return false;
  }

  if (temp_found && high_found) {
    strcpy(_tempfile, tempfile);
    strcpy(_highfile, highfile);
    return true;
  }

  snprintf(temp, 80, "%s/%s", SYS_ACPI_TZ, tempfile);
  snprintf(high, 80, "%s/%s", SYS_ACPI_TZ, highfile);

  if ( (stat(temp, &buf) != 0 || !S_ISREG(buf.st_mode)) || (stat(high, &buf) != 0 || !S_ISREG(buf.st_mode)) )
    _usesysfs = false;
  else {
    strcpy(_tempfile, temp);
    strcpy(_highfile, high);
    _usesysfs = true;
    return true;
  }

  snprintf(temp, 80, "%s/%s", PROC_ACPI_TZ, tempfile);
  snprintf(high, 80, "%s/%s", PROC_ACPI_TZ, highfile);

  if ( (stat(temp, &buf) != 0 || !S_ISREG(buf.st_mode)) || (stat(high, &buf) != 0 || !S_ISREG(buf.st_mode)) )
    return false;

  strcpy(_tempfile, temp);
  strcpy(_highfile, high);
  return true;
}

void ACPITemp::checkResources( void ) {
  FieldMeter::checkResources();

  setfieldcolor( 0, parent_->getResource( "acpitempActColor" ) );
  setfieldcolor( 1, parent_->getResource( "acpitempIdleColor") );
  setfieldcolor( 2, parent_->getResource( "acpitempHighColor" ) );

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
    setfieldcolor( 0, parent_->getResource( "acpitempHighColor" ) );
  }
  else
    setfieldcolor( 0, parent_->getResource( "acpitempActColor" ) );

  fields_[2] = total_ - fields_[1] - fields_[0];
  setUsed( fields_[0], total_ );

  if (high != _high) {
    _high = high;
    snprintf(l, 20, "ACT/%d/%d", (int)high, (int)total_);
    legend(l);
    drawlegend();
  }
}
