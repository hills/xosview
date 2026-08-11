//
//  Copyright (c) 2026 by Antoni Sawicki ( as@tenoware.com )
//
//  This file may be distributed under terms of the GPL
//

//  The AIX 5.x back end.  libperfstat arrived in AIX 5.1 and is the supported
//  way to ask for these statistics, so unlike the AIX 4.x back end in
//  aix/kmem.cc this one needs no access to kernel memory and no privileges,
//  and its counters are 64 bit and do not wrap.

#include "aixstats.h"

#include <libperfstat.h>
#include <string.h>

//  libperfstat reports memory and paging space in 4K units and disk traffic
//  in 512 byte blocks, whatever the page size of the running kernel.
static const double PERFSTAT_PAGE = 4096.0;
static const double PERFSTAT_BLOCK = 512.0;

//  Load averages are fixed point values scaled by 1<<SBITS.
static const double FSCALE = 65536.0;

static const int MAX_INTERFACES = 64;

static bool cputotal( perfstat_cpu_total_t &c ){
  return perfstat_cpu_total( NULL, &c, sizeof(c), 1 ) == 1;
}

static bool memtotal( perfstat_memory_total_t &m ){
  return perfstat_memory_total( NULL, &m, sizeof(m), 1 ) == 1;
}

bool AIXStats::load( double avg[3] ){
  perfstat_cpu_total_t c;

  if ( !cputotal( c ) )
    return false;

  for ( int i = 0 ; i < 3 ; i++ )
    avg[i] = c.loadavg[i] / FSCALE;

  return true;
}

bool AIXStats::cpu( double ticks[4] ){
  perfstat_cpu_total_t c;

  if ( !cputotal( c ) )
    return false;

  ticks[0] = c.user;
  ticks[1] = c.sys;
  ticks[2] = c.wait;
  ticks[3] = c.idle;

  return true;
}

bool AIXStats::memory( double &total, double &cache, double &free ){
  perfstat_memory_total_t m;

  if ( !memtotal( m ) )
    return false;

  total = m.real_total * PERFSTAT_PAGE;
  cache = m.numperm * PERFSTAT_PAGE;
  free = m.real_free * PERFSTAT_PAGE;

  return true;
}

bool AIXStats::swap( double &total, double &free ){
  perfstat_memory_total_t m;

  if ( !memtotal( m ) )
    return false;

  total = m.pgsp_total * PERFSTAT_PAGE;
  free = m.pgsp_free * PERFSTAT_PAGE;

  return true;
}

bool AIXStats::paging( double &in, double &out ){
  perfstat_memory_total_t m;

  if ( !memtotal( m ) )
    return false;

  in = m.pgspins;
  out = m.pgspouts;

  return true;
}

bool AIXStats::disk( double &read, double &written ){
  perfstat_disk_total_t d;

  if ( perfstat_disk_total( NULL, &d, sizeof(d), 1 ) != 1 )
    return false;

  read = d.rblks * PERFSTAT_BLOCK;
  written = d.wblks * PERFSTAT_BLOCK;

  return true;
}

bool AIXStats::net( const char *iface, bool ignore, double &in, double &out ){
  perfstat_netinterface_t ifs[MAX_INTERFACES];
  perfstat_id_t id;
  int n;

  if ( !iface ){
    perfstat_netinterface_total_t t;

    if ( perfstat_netinterface_total( NULL, &t, sizeof(t), 1 ) != 1 )
      return false;

    in = t.ibytes;
    out = t.obytes;
    return true;
  }

  //  An empty name asks for the list from its first entry onwards.
  strcpy( id.name, "" );
  n = perfstat_netinterface( &id, ifs, sizeof(perfstat_netinterface_t),
                             MAX_INTERFACES );
  if ( n < 0 )
    return false;

  in = out = 0;

  for ( int i = 0 ; i < n ; i++ ){
    if ( ( strcmp( iface, ifs[i].name ) == 0 ) == ignore )
      continue;

    in += ifs[i].ibytes;
    out += ifs[i].obytes;
  }

  return true;
}
