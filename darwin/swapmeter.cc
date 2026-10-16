//
//  Copyright (c) 2026 Jonathan Snow
//
//  This file may be distributed under terms of the GPL.
//

#include "swapmeter.h"
#include "xosview.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/sysctl.h>

SwapMeter::SwapMeter(XOSView *parent)
  : FieldMeterGraph(parent, 2, "SWAP", "USED/FREE"), unavailable_(false) {
}

SwapMeter::~SwapMeter(void) {
}

void SwapMeter::checkResources(void) {
  FieldMeterGraph::checkResources();

  setfieldcolor(0, parent_->getResource("swapUsedColor"));
  setfieldcolor(1, parent_->getResource("swapFreeColor"));
  priority_ = atoi(parent_->getResource("swapPriority"));
  dodecay_ = parent_->isResourceTrue("swapDecay");
  useGraph_ = parent_->isResourceTrue("swapGraph");
  SetUsedFormat(parent_->getResource("swapUsedFormat"));
}

void SwapMeter::checkevent(void) {
  if (unavailable_)
    return;
  getswapinfo();
  if (!unavailable_)
    drawfields();
}

void SwapMeter::getswapinfo(void) {
  int mib[2] = {CTL_VM, VM_SWAPUSAGE};
  struct xsw_usage usage;
  size_t usageSize = sizeof(usage);
  if (sysctl(mib, 2, &usage, &usageSize, NULL, 0) != 0) {
    std::cerr << "Cannot get Darwin swap usage: " << strerror(errno)
              << std::endl;
    unavailable_ = true;
    disableMeter();
    return;
  }

  if (usage.xsu_total == 0) {
    // Darwin allocates swap files on demand. Keep an empty, drawable meter
    // when no swap space currently exists.
    total_ = 1.0;
    fields_[0] = 0.0;
    fields_[1] = 1.0;
    setUsed(0.0, total_);
    return;
  }

  total_ = static_cast<double>(usage.xsu_total);
  fields_[0] = static_cast<double>(usage.xsu_used);
  fields_[1] = static_cast<double>(usage.xsu_avail);
  setUsed(fields_[0], total_);
}
