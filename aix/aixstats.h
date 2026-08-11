//
//  Copyright (c) 2026 by Antoni Sawicki ( as@tenoware.com )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _AIXSTATS_H_
#define _AIXSTATS_H_

//  The meters read every statistic through this interface, which has two
//  interchangeable implementations selected by the build target:
//
//    aix/perfstat.cc  libperfstat, the supported interface, on AIX 5.1 and up
//    aix/kmem.cc      /dev/kmem at addresses resolved with knlist(), for the
//                     AIX 4.x releases that predate libperfstat
//
//  Every call returns false when its statistic can not be obtained.  Meters
//  disable themselves in that case rather than fail to start, so a kernel
//  that does not export one of them loses a single meter.
//
//  Counters that accumulate are returned as doubles holding a byte, page or
//  tick count since boot.  Callers difference successive samples themselves,
//  and must cope with a sample that moves backwards: the AIX 4.x kernel keeps
//  several of these counters in 32 bits and they wrap on a busy machine.

namespace AIXStats {
  //  Load averages over the last 1, 5 and 15 minutes, in processes.
  bool load( double avg[3] );

  //  Cumulative cpu ticks: user, system, io wait, idle.
  bool cpu( double ticks[4] );

  //  Real memory in bytes.  cache counts file pages, which are resident but
  //  which the kernel hands back under memory pressure.
  bool memory( double &total, double &cache, double &free );

  //  Paging space in bytes.
  bool swap( double &total, double &free );

  //  Cumulative pages moved in from and out to paging space.
  bool paging( double &in, double &out );

  //  Cumulative bytes read from and written to all disks.
  bool disk( double &read, double &written );

  //  Cumulative bytes received and sent.  iface names a single interface to
  //  report on, or is null to total every interface; when ignore is set the
  //  named interface is the only one left out.
  bool net( const char *iface, bool ignore, double &in, double &out );
}

#endif
