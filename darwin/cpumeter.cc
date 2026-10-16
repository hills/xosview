//
//  Copyright (c) 1994, 1995 Mike Romberg
//  Copyright (c) 2026 Jonathan Snow (Darwin support)
//
//  This file may be distributed under terms of the GPL
//

#include "cpumeter.h"
#include "xosview.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mach/mach.h>
#include <mach/processor_info.h>
#include <unistd.h>

namespace {

uint64_t tickDelta(natural_t current, natural_t previous) {
  return static_cast<natural_t>(current - previous);
}

}

CPUMeter::CPUMeter(XOSView *parent, unsigned int cpu)
  : FieldMeterGraph(parent, 4, "CPU", "USR/NICE/SYS/FREE"), cpu_(cpu),
    initialized_(false), unavailable_(false) {
  if (cpu_ > 0) {
    char titleBuffer[16];
    snprintf(titleBuffer, sizeof(titleBuffer), "CPU%u", cpu_ - 1);
    title(titleBuffer);
  }
}

CPUMeter::~CPUMeter(void) {
}

void CPUMeter::checkResources(void) {
  FieldMeterGraph::checkResources();

  setfieldcolor(0, parent_->getResource("cpuUserColor"));
  setfieldcolor(1, parent_->getResource("cpuNiceColor"));
  setfieldcolor(2, parent_->getResource("cpuSystemColor"));
  setfieldcolor(3, parent_->getResource("cpuFreeColor"));
  priority_ = atoi(parent_->getResource("cpuPriority"));
  dodecay_ = parent_->isResourceTrue("cpuDecay");
  useGraph_ = parent_->isResourceTrue("cpuGraph");
  SetUsedFormat(parent_->getResource("cpuUsedFormat"));
}

void CPUMeter::checkevent(void) {
  if (!unavailable_ && getcputime())
    drawfields();
}

bool CPUMeter::getcputime(void) {
  natural_t cpuCount = 0;
  processor_info_array_t info = NULL;
  mach_msg_type_number_t infoCount = 0;
  kern_return_t result = host_processor_info(
      mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &cpuCount, &info, &infoCount);

  if (result != KERN_SUCCESS) {
    std::cerr << "Cannot get Mach processor load information: "
              << mach_error_string(result) << std::endl;
    unavailable_ = true;
    disableMeter();
    return false;
  }

  processor_cpu_load_info_t loads =
      reinterpret_cast<processor_cpu_load_info_t>(info);
  if (cpu_ > cpuCount) {
    std::cerr << "Requested CPU " << cpu_ - 1 << " is unavailable" << std::endl;
    vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(info),
                  infoCount * sizeof(integer_t));
    unavailable_ = true;
    disableMeter();
    return false;
  }

  if (previous_.size() != cpuCount) {
    previous_.assign(cpuCount, TickSet());
    initialized_ = false;
  }

  uint64_t deltas[CPU_STATE_MAX] = {};
  for (natural_t processor = 0; processor < cpuCount; ++processor) {
    bool selected = cpu_ == 0 || processor == cpu_ - 1;
    for (int state = 0; state < CPU_STATE_MAX; ++state) {
      natural_t current = loads[processor].cpu_ticks[state];
      if (selected && initialized_)
        deltas[state] += tickDelta(current, previous_[processor][state]);
      previous_[processor][state] = current;
    }
  }

  vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(info),
                infoCount * sizeof(integer_t));

  if (!initialized_) {
    initialized_ = true;
    return false;
  }

  fields_[0] = deltas[CPU_STATE_USER];
  fields_[1] = deltas[CPU_STATE_NICE];
  fields_[2] = deltas[CPU_STATE_SYSTEM];
  fields_[3] = deltas[CPU_STATE_IDLE];
  total_ = fields_[0] + fields_[1] + fields_[2] + fields_[3];
  if (total_ <= 0.0)
    return false;

  setUsed(total_ - fields_[3], total_);
  return true;
}

unsigned int CPUMeter::countCPUs(void) {
  long count = sysconf(_SC_NPROCESSORS_ONLN);
  if (count < 1)
    return 1;
  return static_cast<unsigned int>(count);
}
