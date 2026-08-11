//
//  Copyright (c) 2026 by Antoni Sawicki ( as@tenoware.com )
//
//  This file may be distributed under terms of the GPL
//

//  The AIX 4.x back end.  These releases predate libperfstat, so every
//  statistic is read straight out of /dev/kmem at an address resolved with
//  knlist().  See aix/perfstat.cc for the AIX 5.x back end.

#include "aixstats.h"
#include "vmker.h"

#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/vminfo.h>
#include <sys/iostat.h>
#include <sys/socket.h>
#include <net/if.h>
#include <nlist.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <iostream>

//  AIX 4.1 ships no prototype for knlist(); readx() is declared in unistd.h.
extern "C" {
  int knlist( struct nlist *, int, int );
}

static const unsigned long MAX_DISKS = 256;
static const int MAX_INTERFACES = 64;

//  avenrun holds the load averages as fixed point values scaled by 1<<16,
//  which is what uptime(1) and friends divide by.
static const double FSCALE = 65536.0;

static const int NOT_OPENED = -2;

static int kmemfd_ = NOT_OPENED;

static int kmem( void ){
  if ( kmemfd_ == NOT_OPENED ){
    kmemfd_ = open( "/dev/kmem", O_RDONLY );
    if ( kmemfd_ < 0 )
      std::cerr << "Can not open /dev/kmem.  xosview must run as root or be\n"
                << "  installed setgid system to read kernel statistics."
                << std::endl;
  }
  return kmemfd_;
}

//  Address of a kernel symbol, or 0 if it can not be resolved.  Callers keep
//  the result in a function static, so an unresolvable symbol is reported
//  once rather than on every sample.
static unsigned long symbol( const char *name ){
  struct nlist nl[2];

  memset( nl, 0, sizeof(nl) );
  nl[0].n_name = (char *)name;
  nl[1].n_name = NULL;

  if ( knlist( nl, 1, sizeof(struct nlist) ) != 0 || nl[0].n_value == 0 ){
    std::cerr << "Can not resolve kernel symbol '" << name << "'." << std::endl;
    return 0;
  }

  return nl[0].n_value;
}

//  Copy size bytes from kernel address addr.  Returns false on failure.
static bool kread( unsigned long addr, void *buf, int size ){
  int fd = kmem();
  int upper_2gb = 0;

  if ( fd < 0 || addr == 0 )
    return false;

  //  Addresses above 2GB are reached by seeking to addr % 2GB and passing 1
  //  as the extension argument of readx().  See the AIX kmem(4) man page.
  if ( addr > 0x7fffffff ){
    upper_2gb = 1;
    addr &= 0x7fffffff;
  }

  if ( lseek( fd, addr, SEEK_SET ) == -1 )
    return false;

  return readx( fd, (char *)buf, size, upper_2gb ) == size;
}

static bool vmkerstats( struct vmker &vmk ){
  static unsigned long addr = symbol( "vmker" );

  return kread( addr, &vmk, sizeof(vmk) );
}

bool AIXStats::load( double avg[3] ){
  static unsigned long addr = symbol( "avenrun" );
  int avenrun[3];

  if ( !kread( addr, avenrun, sizeof(avenrun) ) )
    return false;

  for ( int i = 0 ; i < 3 ; i++ )
    avg[i] = avenrun[i] / FSCALE;

  return true;
}

bool AIXStats::cpu( double ticks[4] ){
  static unsigned long addr = symbol( "sysinfo" );
  struct sysinfo si;

  if ( !kread( addr, &si, sizeof(si) ) )
    return false;

  ticks[0] = si.cpu[CPU_USER];
  ticks[1] = si.cpu[CPU_KERNEL];
  ticks[2] = si.cpu[CPU_WAIT];
  ticks[3] = si.cpu[CPU_IDLE];

  return true;
}

bool AIXStats::memory( double &total, double &cache, double &free ){
  struct vmker vmk;
  double pagesize = getpagesize();

  if ( !vmkerstats( vmk ) )
    return false;

  total = (double)vmk.totalmem * pagesize;
  cache = (double)vmk.numperm * pagesize;
  free = (double)vmk.freemem * pagesize;

  return true;
}

bool AIXStats::swap( double &total, double &free ){
  struct vmker vmk;
  double pagesize = getpagesize();

  if ( !vmkerstats( vmk ) )
    return false;

  total = (double)vmk.totalvmem * pagesize;
  free = (double)vmk.freevmem * pagesize;

  return true;
}

bool AIXStats::paging( double &in, double &out ){
  static unsigned long addr = symbol( "vmminfo" );
  struct vminfo vmi;

  if ( !kread( addr, &vmi, sizeof(vmi) ) )
    return false;

  in = vmi.pgspgins;
  out = vmi.pgspgouts;

  return true;
}

bool AIXStats::disk( double &read, double &written ){
  //  The kernel iostat struct heads a chain of per disk dkstat entries, the
  //  same ones iostat(1) reports.  Block counts are scaled by the per disk
  //  block size to get bytes.
  static unsigned long addr = symbol( "iostat" );
  struct iostat ios;
  unsigned long next;

  if ( !kread( addr, &ios, sizeof(ios) ) )
    return false;

  read = written = 0;
  next = (unsigned long)ios.dkstatp;

  for ( unsigned long i = 0 ; next && i < ios.dk_cnt && i < MAX_DISKS ; i++ ){
    struct dkstat dk;

    if ( !kread( next, &dk, sizeof(dk) ) )
      break;

    read += (double)dk.dk_rblks * dk.dk_bsize;
    written += (double)dk.dk_wblks * dk.dk_bsize;
    next = (unsigned long)dk.dknextp;
  }

  return true;
}

bool AIXStats::net( const char *iface, bool ignore, double &in, double &out ){
  //  Interfaces are counted by walking the kernel ifnet chain.
  static unsigned long addr = symbol( "ifnet" );
  unsigned long next;

  if ( !kread( addr, &next, sizeof(next) ) )
    return false;

  in = out = 0;

  for ( int i = 0 ; next && i < MAX_INTERFACES ; i++ ){
    struct ifnet ifn;

    if ( !kread( next, &ifn, sizeof(ifn) ) )
      break;
    next = (unsigned long)ifn.if_next;

    if ( iface ){
      //  if_name points at a string still living in kernel memory.
      char name[16], found[32];

      name[0] = '\0';
      if ( !kread( (unsigned long)ifn.if_name, name, sizeof(name) ) )
        continue;
      name[sizeof(name) - 1] = '\0';
      snprintf( found, sizeof(found), "%s%d", name, ifn.if_unit );

      if ( ( strcmp( iface, found ) == 0 ) == ignore )
        continue;
    }

    //  These counters are 32 bit and wrap on a busy link; the caller notices
    //  because the total it keeps moves backwards.
    in += (unsigned int)ifn.if_ibytes;
    out += (unsigned int)ifn.if_obytes;
  }

  return true;
}
