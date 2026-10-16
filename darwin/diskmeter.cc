//
//  Copyright (c) 2026 Jonathan Snow
//
//  This file may be distributed under terms of the GPL.
//

#include "diskmeter.h"
#include "xosview.h"

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOBlockStorageDriver.h>
#include <cstdlib>
#include <iostream>

DiskMeter::DiskMeter(XOSView *parent, double max)
  : FieldMeterGraph(parent, 3, "DISK", "READ/WRITE/IDLE"),
    lastBytesRead_(0), lastBytesWritten_(0), maxBandwidth_(max),
    haveBaseline_(false), unavailable_(false) {
  total_ = maxBandwidth_;
  IntervalTimerStart();
}

DiskMeter::~DiskMeter(void) {
}

void DiskMeter::checkResources(void) {
  FieldMeterGraph::checkResources();

  setfieldcolor(0, parent_->getResource("diskReadColor"));
  setfieldcolor(1, parent_->getResource("diskWriteColor"));
  setfieldcolor(2, parent_->getResource("diskIdleColor"));
  priority_ = atoi(parent_->getResource("diskPriority"));
  dodecay_ = parent_->isResourceTrue("diskDecay");
  useGraph_ = parent_->isResourceTrue("diskGraph");
  SetUsedFormat(parent_->getResource("diskUsedFormat"));
  haveBaseline_ = false;
}

void DiskMeter::checkevent(void) {
  if (unavailable_)
    return;
  uint64_t bytesRead = 0;
  uint64_t bytesWritten = 0;

  IntervalTimerStop();
  if (!getstats(bytesRead, bytesWritten)) {
    // A disappearing device disables only this meter.
    unavailable_ = true;
    disableMeter();
    return;
  }
  double elapsed = IntervalTimeInSecs();
  IntervalTimerStart();

  fields_[0] = fields_[1] = 0.0;
  if (haveBaseline_ && elapsed > 0.0) {
    // A removable device can disappear between samples and reduce the
    // aggregate. Reset that direction instead of drawing a false wrap spike.
    if (bytesRead >= lastBytesRead_)
      fields_[0] = static_cast<double>(bytesRead - lastBytesRead_) / elapsed;
    if (bytesWritten >= lastBytesWritten_)
      fields_[1] =
          static_cast<double>(bytesWritten - lastBytesWritten_) / elapsed;
  }

  lastBytesRead_ = bytesRead;
  lastBytesWritten_ = bytesWritten;
  haveBaseline_ = true;

  total_ = maxBandwidth_;
  const double traffic = fields_[0] + fields_[1];
  if (traffic > total_)
    total_ = traffic;
  fields_[2] = total_ - traffic;
  setUsed(traffic, total_);
  drawfields();
}

bool DiskMeter::getstats(uint64_t &bytesRead, uint64_t &bytesWritten) {
  bytesRead = bytesWritten = 0;

  CFMutableDictionaryRef matching =
      IOServiceMatching("IOBlockStorageDriver");
  if (!matching) {
    std::cerr << "Cannot create Darwin block-storage match dictionary"
              << std::endl;
    return false;
  }

  io_iterator_t iterator = IO_OBJECT_NULL;
  kern_return_t result = IOServiceGetMatchingServices(
      MACH_PORT_NULL, matching, &iterator);
  if (result != KERN_SUCCESS) {
    std::cerr << "Cannot get Darwin block-storage services: " << result
              << std::endl;
    return false;
  }

  io_registry_entry_t service;
  while ((service = IOIteratorNext(iterator)) != IO_OBJECT_NULL) {
    CFTypeRef property = IORegistryEntryCreateCFProperty(
        service, CFSTR(kIOBlockStorageDriverStatisticsKey),
        kCFAllocatorDefault, 0);
    IOObjectRelease(service);
    if (!property)
      continue;

    if (CFGetTypeID(property) == CFDictionaryGetTypeID()) {
      CFDictionaryRef statistics =
          static_cast<CFDictionaryRef>(property);
      CFTypeRef readValue = CFDictionaryGetValue(
          statistics, CFSTR(kIOBlockStorageDriverStatisticsBytesReadKey));
      CFTypeRef writtenValue = CFDictionaryGetValue(
          statistics, CFSTR(kIOBlockStorageDriverStatisticsBytesWrittenKey));

      int64_t value;
      if (readValue && CFGetTypeID(readValue) == CFNumberGetTypeID() &&
          CFNumberGetValue(static_cast<CFNumberRef>(readValue),
                           kCFNumberSInt64Type, &value) && value >= 0)
        bytesRead += static_cast<uint64_t>(value);
      if (writtenValue && CFGetTypeID(writtenValue) == CFNumberGetTypeID() &&
          CFNumberGetValue(static_cast<CFNumberRef>(writtenValue),
                           kCFNumberSInt64Type, &value) && value >= 0)
        bytesWritten += static_cast<uint64_t>(value);
    }
    CFRelease(property);
  }

  IOObjectRelease(iterator);
  return true;
}
