//
//  Copyright (c) 2026 Jonathan Snow
//
//  This file may be distributed under terms of the GPL.
//

#include "netmeter.h"
#include "xosview.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <net/if.h>
#include <net/route.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <vector>

namespace {

struct RouteMessageHeader {
  uint16_t length;
  uint8_t version;
  uint8_t type;
};

}

NetMeter::NetMeter(XOSView *parent, double max)
  : FieldMeterGraph(parent, 3, "NET", "IN/OUT/IDLE"),
    lastBytesIn_(0), lastBytesOut_(0), netBandwidth_(max),
    netIface_("False"), ignored_(false), haveBaseline_(false),
    unavailable_(false) {
  total_ = netBandwidth_;
  IntervalTimerStart();
}

NetMeter::~NetMeter(void) {
}

void NetMeter::checkResources(void) {
  FieldMeterGraph::checkResources();

  setfieldcolor(0, parent_->getResource("netInColor"));
  setfieldcolor(1, parent_->getResource("netOutColor"));
  setfieldcolor(2, parent_->getResource("netBackground"));
  priority_ = atoi(parent_->getResource("netPriority"));
  dodecay_ = parent_->isResourceTrue("netDecay");
  useGraph_ = parent_->isResourceTrue("netGraph");
  SetUsedFormat(parent_->getResource("netUsedFormat"));
  netIface_ = parent_->getResource("netIface");
  ignored_ = !netIface_.empty() && netIface_[0] == '-';
  if (ignored_)
    netIface_.erase(0, netIface_.find_first_not_of("- "));
  haveBaseline_ = false;
}

void NetMeter::checkevent(void) {
  if (unavailable_)
    return;
  uint64_t bytesIn = 0;
  uint64_t bytesOut = 0;

  IntervalTimerStop();
  if (!getstats(bytesIn, bytesOut)) {
    // A disappearing interface disables only this meter.
    unavailable_ = true;
    disableMeter();
    return;
  }
  double elapsed = IntervalTimeInSecs();
  IntervalTimerStart();

  fields_[0] = fields_[1] = 0.0;
  if (haveBaseline_ && elapsed > 0.0) {
    // Aggregates can decrease when a dynamic interface disappears. Treat that
    // as a new baseline rather than unsigned wrap and a false traffic spike.
    if (bytesIn >= lastBytesIn_)
      fields_[0] = static_cast<double>(bytesIn - lastBytesIn_) / elapsed;
    if (bytesOut >= lastBytesOut_)
      fields_[1] = static_cast<double>(bytesOut - lastBytesOut_) / elapsed;
  }

  lastBytesIn_ = bytesIn;
  lastBytesOut_ = bytesOut;
  haveBaseline_ = true;

  total_ = netBandwidth_;
  const double traffic = fields_[0] + fields_[1];
  if (traffic > total_)
    total_ = traffic;
  fields_[2] = total_ - traffic;
  setUsed(traffic, total_);
  drawfields();
}

bool NetMeter::getstats(uint64_t &bytesIn, uint64_t &bytesOut) {
  int mib[6] = {CTL_NET, PF_ROUTE, 0, 0, NET_RT_IFLIST2, 0};
  bytesIn = bytesOut = 0;

  for (unsigned int retry = 0; retry < 4; ++retry) {
    size_t length = 0;
    if (sysctl(mib, 6, NULL, &length, NULL, 0) != 0 || length == 0) {
      std::cerr << "Cannot size Darwin interface list: " << strerror(errno)
                << std::endl;
      return false;
    }

    std::vector<char> buffer(length);
    if (sysctl(mib, 6, &buffer[0], &length, NULL, 0) != 0) {
      if (errno == ENOMEM && retry < 3)
        continue;
      std::cerr << "Cannot get Darwin interface list: " << strerror(errno)
                << std::endl;
      return false;
    }

    char *next = &buffer[0];
    const char *end = next + length;
    while (next < end) {
      if (static_cast<size_t>(end - next) < sizeof(RouteMessageHeader)) {
        std::cerr << "Truncated Darwin interface message" << std::endl;
        return false;
      }

      RouteMessageHeader *header =
          reinterpret_cast<RouteMessageHeader *>(next);
      char *message = next;
      if (header->length == 0 || next + header->length > end) {
        std::cerr << "Invalid Darwin interface message length" << std::endl;
        return false;
      }
      next += header->length;

      if (header->type != RTM_IFINFO2 ||
          header->length < sizeof(struct if_msghdr2))
        continue;

      struct if_msghdr2 *interface =
          reinterpret_cast<struct if_msghdr2 *>(message);
      if (netIface_ != "False") {
        char name[IF_NAMESIZE];
        const char *found = if_indextoname(interface->ifm_index, name);
        const bool matches = found && netIface_ == found;
        if ((!ignored_ && !matches) || (ignored_ && matches))
          continue;
      }

      bytesIn += interface->ifm_data.ifi_ibytes;
      bytesOut += interface->ifm_data.ifi_obytes;
    }
    return true;
  }

  return false;
}
