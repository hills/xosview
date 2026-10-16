//
//  Copyright (c) 2026 Jonathan Snow
//
//  This file may be distributed under terms of the GPL.
//

#include "memmeter.h"
#include "xosview.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mach/host_info.h>
#include <mach/mach.h>
#include <mach/mach_error.h>
#include <mach/vm_statistics.h>
#include <sys/sysctl.h>

MemMeter::MemMeter(XOSView *parent)
  : FieldMeterGraph(parent, 5, "MEM", "ACT/INACT/WIRE/OTHER/FREE"),
    unavailable_(false) {
}

MemMeter::~MemMeter(void) {
}

void MemMeter::checkResources(void) {
  FieldMeterGraph::checkResources();

  setfieldcolor(0, parent_->getResource("memActiveColor"));
  setfieldcolor(1, parent_->getResource("memInactiveColor"));
  setfieldcolor(2, parent_->getResource("memWiredColor"));
  setfieldcolor(3, parent_->getResource("memOtherColor"));
  setfieldcolor(4, parent_->getResource("memFreeColor"));
  priority_ = atoi(parent_->getResource("memPriority"));
  dodecay_ = parent_->isResourceTrue("memDecay");
  useGraph_ = parent_->isResourceTrue("memGraph");
  SetUsedFormat(parent_->getResource("memUsedFormat"));
}

void MemMeter::checkevent(void) {
  if (unavailable_)
    return;
  getmeminfo();
  if (!unavailable_)
    drawfields();
}

void MemMeter::getmeminfo(void) {
  mach_port_t host = mach_host_self();
  vm_statistics64_data_t stats;
  mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
  kern_return_t result = host_statistics64(
      host, HOST_VM_INFO64, reinterpret_cast<host_info64_t>(&stats), &count);
  if (result != KERN_SUCCESS) {
    std::cerr << "Cannot get Darwin VM statistics: "
              << mach_error_string(result) << std::endl;
    mach_port_deallocate(mach_task_self(), host);
    unavailable_ = true;
    disableMeter();
    return;
  }

  vm_size_t pageSize;
  result = host_page_size(host, &pageSize);
  mach_port_deallocate(mach_task_self(), host);
  if (result != KERN_SUCCESS) {
    std::cerr << "Cannot get Darwin VM page size: "
              << mach_error_string(result) << std::endl;
    unavailable_ = true;
    disableMeter();
    return;
  }

  uint64_t physicalBytes = 0;
  size_t physicalBytesSize = sizeof(physicalBytes);
  if (sysctlbyname("hw.memsize", &physicalBytes, &physicalBytesSize,
                   NULL, 0) != 0) {
    std::cerr << "Cannot get Darwin physical memory size: "
              << strerror(errno) << std::endl;
    unavailable_ = true;
    disableMeter();
    return;
  }

  const uint64_t active = static_cast<uint64_t>(stats.active_count) * pageSize;
  const uint64_t inactive =
      static_cast<uint64_t>(stats.inactive_count) * pageSize;
  const uint64_t wired = static_cast<uint64_t>(stats.wire_count) * pageSize;
  const uint64_t compressed =
      static_cast<uint64_t>(stats.compressor_page_count) * pageSize;
  // free_count already includes speculative_count on Darwin.
  const uint64_t free = static_cast<uint64_t>(stats.free_count) * pageSize;
  const uint64_t classified = active + inactive + wired + compressed + free;
  const uint64_t other = physicalBytes > classified
                           ? physicalBytes - classified
                           : 0;
  const uint64_t represented = classified + other;
  fields_[0] = static_cast<double>(active);
  fields_[1] = static_cast<double>(inactive);
  fields_[2] = static_cast<double>(wired);
  fields_[3] = static_cast<double>(compressed + other);
  fields_[4] = static_cast<double>(free);
  total_ = static_cast<double>(represented);
  // Match the established Unix/Linux xosview meaning of USED: everything
  // except currently free physical memory, including active and inactive
  // pages that may still be reclaimable by the VM system.
  setUsed(static_cast<double>(represented - free), total_);
}
