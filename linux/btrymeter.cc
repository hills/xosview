//
//  Copyright (c) 1997 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//
// $Id$
//
#include "btrymeter.h"
#include "xosview.h"
#include <fstream>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>

static const char APMFILENAME[] = "/proc/apm";
static const char ACPIBATTERYDIR[] = "/proc/acpi/battery";

BtryMeter::BtryMeter( XOSView *parent )
  : FieldMeter( parent, 2, "BTRY", "AVAIL/USED", 1, 1, 0 ){
}

BtryMeter::~BtryMeter( void ){
}

void BtryMeter::checkResources( void ){
  FieldMeter::checkResources();

  setfieldcolor( 0, parent_->getResource( "batteryLeftColor" ) );
  setfieldcolor( 1, parent_->getResource( "batteryUsedColor" ) );

  priority_ = atoi (parent_->getResource( "batteryPriority" ) );
  SetUsedFormat(parent_->getResource( "batteryUsedFormat" ) );
}

void BtryMeter::checkevent( void ){
  getpwrinfo();

  drawfields();
}


void BtryMeter::getpwrinfo( void ){
  if (getapminfo() || getacpiinfo())
    return;

  std::cerr <<"Cannot get battery information" << std::endl;
  std::cerr <<"If using APM, have you loaded the `apm' module?" << std::endl;
  std::cerr <<"If using ACPI, have you loaded the `battery' module?" << std::endl;
  parent_->done(1);
}


bool BtryMeter::getapminfo( void ){
  std::ifstream loadinfo( APMFILENAME );

  if ( !loadinfo ){
    XOSDEBUG("Can not open file : %s\n", APMFILENAME);
    return false;
  }

  char buff[256];
  loadinfo >> buff >> buff >> buff >> buff >> buff >> buff >> fields_[0];

  total_ = 100;

  fields_[1] = total_ - fields_[0];

  setUsed (fields_[0], total_);

  return true;
}


bool BtryMeter::getacpiinfo( void ){
  DIR *dir = opendir(ACPIBATTERYDIR);
  if (dir==NULL) {
    XOSDEBUG("Cannot open directory : %s\n", ACPIBATTERYDIR);
    return false;
  }

  bool found = false;
  std::string abs_battery_dir = ACPIBATTERYDIR;
  for (struct dirent *dirent; (dirent = readdir(dir)) != NULL; ) {
    if (strncmp(dirent->d_name, ".", 1) == 0
    	|| strncmp(dirent->d_name, "..", 2) == 0)
      continue;

    std::string abs_battery_name = abs_battery_dir + "/" + dirent->d_name;
    DIR *battery = opendir(abs_battery_name.c_str());
    if (battery==NULL) continue; // not a directory? try skipping to next...

    XOSDEBUG("Found battery : %s\n", dirent->d_name);
    closedir(battery); // FIXME: check for error!

    if (getacpiinfofield(abs_battery_name + "/info", "design capacity",
			 total_)
	&& getacpiinfofield(abs_battery_name + "/state", "remaining capacity",
			    fields_[0])) {
      found = true;
      break;
    }
  }

  closedir(dir);

  if (!found) return false;

  fields_[1] = total_ - fields_[0];
  setUsed (fields_[0], total_);
  return true;
}

bool BtryMeter::getacpiinfofield(const std::string& filename,
				 const std::string& fieldname,
				 float& value){
  std::ifstream loadinfo(filename.c_str());
  if (!loadinfo)
    return false; // how come?

  XOSDEBUG("Reading battery info from : %s\n", filename.c_str());

  bool got_it = false;
  char line_buf[256];
  while (loadinfo.getline(line_buf, 256)) {
    char *pfield = strtok(line_buf, ":");
    char *pvalue = strtok(NULL, " :");
    if (pfield == NULL || pvalue == NULL)
      continue;

    if (fieldname != pfield)
      continue;

    value = atof(pvalue);
    got_it = true;
  }
  return got_it;
}
