//
//  Copyright (c) 1994, 1995 Mike Romberg
//  Copyright (c) 2026 Jonathan Snow (Darwin support)
//
//  This file may be distributed under terms of the GPL
//

#include "loadmeter.h"
#include "xosview.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>

namespace {

float threshold(const char *resource, float automatic) {
  if (strncmp(resource, "auto", 2) == 0)
    return automatic;
  return strtof(resource, NULL);
}

}

LoadMeter::LoadMeter(XOSView *parent)
  : FieldMeterGraph(parent, 2, "LOAD", "PROCS/MIN", 1, 1, 0),
    warnThreshold_(1.0), critThreshold_(4.0), alarmstate_(0),
    lastalarmstate_(-1), unavailable_(false) {
  total_ = 2.0;
}

LoadMeter::~LoadMeter(void) {
}

void LoadMeter::checkResources(void) {
  FieldMeterGraph::checkResources();

  procloadcol_ = parent_->allocColor(parent_->getResource("loadProcColor"));
  warnloadcol_ = parent_->allocColor(parent_->getResource("loadWarnColor"));
  critloadcol_ = parent_->allocColor(parent_->getResource("loadCritColor"));

  setfieldcolor(0, procloadcol_);
  setfieldcolor(1, parent_->getResource("loadIdleColor"));
  priority_ = atoi(parent_->getResource("loadPriority"));
  useGraph_ = parent_->isResourceTrue("loadGraph");
  dodecay_ = parent_->isResourceTrue("loadDecay");
  SetUsedFormat(parent_->getResource("loadUsedFormat"));

  long cpuCount = sysconf(_SC_NPROCESSORS_ONLN);
  if (cpuCount < 1)
    cpuCount = 1;
  warnThreshold_ = threshold(parent_->getResource("loadWarnThreshold"),
                             static_cast<float>(cpuCount));
  critThreshold_ = threshold(parent_->getResource("loadCritThreshold"),
                             static_cast<float>(cpuCount * 4));

  if (dodecay_) {
    std::cerr << "Warning: the load meter cannot be a decay meter.\n";
    dodecay_ = 0;
  }
}

void LoadMeter::checkevent(void) {
  if (unavailable_)
    return;
  getloadinfo();
  if (!unavailable_)
    drawfields();
}

void LoadMeter::getloadinfo(void) {
  double loadAverage;
  if (getloadavg(&loadAverage, 1) != 1) {
    std::cerr << "Cannot get the system load average" << std::endl;
    // An unavailable collector should not terminate other meters.
    unavailable_ = true;
    disableMeter();
    return;
  }

  fields_[0] = static_cast<float>(loadAverage);

  if (fields_[0] < warnThreshold_)
    alarmstate_ = 0;
  else if (fields_[0] >= critThreshold_)
    alarmstate_ = 2;
  else
    alarmstate_ = 1;

  if (alarmstate_ != lastalarmstate_) {
    if (alarmstate_ == 0)
      setfieldcolor(0, procloadcol_);
    else if (alarmstate_ == 1)
      setfieldcolor(0, warnloadcol_);
    else
      setfieldcolor(0, critloadcol_);
    drawlegend();
    lastalarmstate_ = alarmstate_;
  }

  if ((fields_[0] * 5.0 < total_ && total_ > 1.0) || fields_[0] > total_) {
    unsigned int scale = static_cast<unsigned int>(fields_[0]);
    scale |= scale >> 1;
    scale |= scale >> 2;
    scale |= scale >> 4;
    scale |= scale >> 8;
    scale |= scale >> 16;
    total_ = scale + 1;
  }

  fields_[1] = total_ - fields_[0];
  setUsed(fields_[0], 1.0);
}
