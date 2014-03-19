//
//  Copyright (c) 1999, 2006 by Mike Romberg (mike.romberg@noaa.gov)
//
//  This file may be distributed under terms of the GPL
//

#include "diskmeter.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <fstream>
#include <iostream>
#include <string>

#define MAX_PROCSTAT_LENGTH 4096


DiskMeter::DiskMeter( XOSView *parent, float max ) : FieldMeterGraph(
  parent, 3, "DISK", "READ/WRITE/IDLE"), _vmstat(false),
  _statFileName("/proc/stat")
{
    read_prev_ = 0;
    write_prev_ = 0;
    maxspeed_ = max;

    _sysfs=_vmstat=false;
    struct stat buf;

    // first - try sysfs:
    if (stat("/sys/block", &buf) == 0
      && buf.st_mode & S_IFDIR) {

        _sysfs = true;
        _statFileName = "/sys/block";
        XOSDEBUG("diskmeter: using sysfs /sys/block\n");
        getsysfsdiskinfo();

    } else  // try vmstat:
    if (stat("/proc/vmstat", &buf) == 0
      && buf.st_mode & S_IFREG) {

        _vmstat = true;
        _sysfs  = false;
        _statFileName = "/proc/vmstat";
        getvmdiskinfo();

    } else // fall back to stat
        getdiskinfo();


}

DiskMeter::~DiskMeter( void )
    {
    }

void DiskMeter::checkResources( void )
    {
    FieldMeterGraph::checkResources();

    setfieldcolor( 0, parent_->getResource("diskReadColor") );
    setfieldcolor( 1, parent_->getResource("diskWriteColor") );
    setfieldcolor( 2, parent_->getResource("diskIdleColor") );
    priority_ = atoi (parent_->getResource( "diskPriority" ) );
    dodecay_ = parent_->isResourceTrue("diskDecay" );
    useGraph_ = parent_->isResourceTrue( "diskGraph" );
    SetUsedFormat(parent_->getResource("diskUsedFormat"));
    }

void DiskMeter::checkevent( void )
    {
    if (_vmstat)
        getvmdiskinfo();
    else if ( _sysfs )
        getsysfsdiskinfo();
    else
        getdiskinfo();

    drawfields();

    }

// IMHO the logic here is quite broken - but for backward compat UNCHANGED:
void DiskMeter::updateinfo(unsigned long one, unsigned long two,
  int fudgeFactor)
    {
    // assume each "unit" is 1k.
    // This is true for ext2, but seems to be 512 bytes
    // for vfat and 2k for cdroms
    // work in 512-byte blocks

    // tw: strange, on my system, a ext2fs (read and write)
    // unit seems to be 2048. kernel 2.2.12 and the file system
    // is on a SW-RAID5 device (/dev/md0).

    // So this is a FIXME - but how ???

    float itim = IntervalTimeInMicrosecs();
    unsigned long read_curr = one * fudgeFactor;  // FIXME!
    unsigned long write_curr = two * fudgeFactor; // FIXME!

    // avoid strange values at first call
    if(read_prev_ == 0) read_prev_ = read_curr;
    if(write_prev_ == 0) write_prev_ = write_curr;

    // calculate rate in bytes per second
    fields_[0] = ((read_curr - read_prev_) * 1e6 * 512) / itim;
    fields_[1] = ((write_curr - write_prev_) * 1e6 * 512) / itim;

    // fix overflow (conversion bug?)
    if (fields_[0] < 0.0)
        fields_[0] = 0.0;
    if (fields_[1] < 0.0)
        fields_[1] = 0.0;

    if (fields_[0] + fields_[1] > total_)
       	total_ = fields_[0] + fields_[1];

    fields_[2] = total_ - (fields_[0] + fields_[1]);

    read_prev_ = read_curr;
    write_prev_ = write_curr;

    setUsed((fields_[0]+fields_[1]), total_);
    IntervalTimerStart();
    }

void DiskMeter::getvmdiskinfo(void)
{
    IntervalTimerStop();
    total_ = maxspeed_;
    char buf[MAX_PROCSTAT_LENGTH];
    std::ifstream stats(_statFileName);
    unsigned long one, two;

    if ( !stats )
        {
        std::cerr <<"Can not open file : " << _statFileName << std::endl;
        exit( 1 );
        }


    stats >> buf;
    // kernel >= 2.5
    while (!stats.eof() && strncmp(buf, "pgpgin", 7))
        {
        stats.ignore(1024, '\n');
        stats >> buf;
        }

    // read first value
    stats >> one;

    while (!stats.eof() && strncmp(buf, "pgpgout", 7))
        {
        stats.ignore(1024, '\n');
        stats >> buf;
        }

    // read second value
    stats >> two;

    updateinfo(one, two, 4);
}

void DiskMeter::getdiskinfo( void )
{
    IntervalTimerStop();
    total_ = maxspeed_;
    char buf[MAX_PROCSTAT_LENGTH];
    std::ifstream stats(_statFileName);

    if ( !stats )
    {
        std::cerr <<"Can not open file : " << _statFileName << std::endl;
        exit( 1 );
    }

    // Find the line with 'page'
    stats >> buf;
    while (strncmp(buf, "disk_io:", 8))
    {
        stats.ignore(MAX_PROCSTAT_LENGTH, '\n');
        stats >> buf;
        if (stats.eof())
            break;
    }

    // read values
    unsigned long one=0, two=0;
    unsigned long junk,read1,write1;
    stats >> buf;
    while (7 == sscanf(buf,"(%lu,%lu):(%lu,%lu,%lu,%lu,%lu)",&junk,&junk,&junk,&junk,&read1,&junk,&write1))
    {
        one += read1;
        two += write1;
        stats >> buf;
    }

    updateinfo(one, two, 1);
}

// sysfs version - works with long-long !!
void DiskMeter::update_info(const diskmap &reads, const diskmap &writes)
{
    float itim = IntervalTimeInMicrosecs();
    // the sum of all disks
    unsigned long long all_bytes_read = 0, all_bytes_written = 0;
    unsigned int sect_size = 512; // from linux-3.10/Documentation/block/stat.txt

    // avoid strange values at first call
    // (by this - the first value displayed becomes zero)
    if (sysfs_read_prev_.empty())
    {
        sysfs_read_prev_  = reads;
        sysfs_write_prev_ = writes;
        itim = 1;	// itim is garbage here too. Valgrind complains.
    }

    for (diskmap::const_iterator it = reads.begin(); it != reads.end(); it++)
    {
        if (it->second < sysfs_read_prev_[it->first]) // counter wrapped
            all_bytes_read += ULONG_MAX - sysfs_read_prev_[it->first] + it->second;
        else
            all_bytes_read += it->second - sysfs_read_prev_[it->first];
    }
    for (diskmap::const_iterator it = writes.begin(); it != writes.end(); it++)
    {
        if (it->second < sysfs_write_prev_[it->first]) // counter wrapped
            all_bytes_written += ULONG_MAX - sysfs_write_prev_[it->first] + it->second;
        else
            all_bytes_written += it->second - sysfs_write_prev_[it->first];
    }

    all_bytes_read *= sect_size;
    all_bytes_written *= sect_size;
    XOSDEBUG("disk: read: %llu, written: %llu\n", all_bytes_read, all_bytes_written);

    // convert rate from bytes/microsec into bytes/second
    fields_[0] = all_bytes_read * ( 1e6 / itim );
    fields_[1] = all_bytes_written * ( 1e6 / itim );

    // fix overflow (conversion bug?)
    if (fields_[0] < 0.0)
        fields_[0] = 0.0;
    if (fields_[1] < 0.0)
        fields_[1] = 0.0;

    // bump up max total:
    if (fields_[0] + fields_[1] > total_)
        total_ = fields_[0] + fields_[1];

    fields_[2] = total_ - (fields_[0] + fields_[1]);

    // save old vals for next round
    sysfs_read_prev_  = reads;
    sysfs_write_prev_ = writes;

    setUsed(fields_[0] + fields_[1], total_);
    IntervalTimerStart();
}

// XXX: sysfs - read Documentation/iostats.txt !!!
// extract stats from /sys/block/*/stat
// each disk reports an unsigned long, which can WRAP around
void DiskMeter::getsysfsdiskinfo( void )
{
        // field-3: sects read since boot (but can wrap!)
        // field-7: sects written since boot (but can wrap!)
        // just sum up everything in /sys/block/*/stat

  std::string sysfs_dir = _statFileName;
  std::string disk, tmp;
  std::ifstream diskstat;
  struct stat buf;
  char line[128];
  unsigned long vals[7];
  diskmap reads, writes;

  IntervalTimerStop();
  total_ = maxspeed_;
  sysfs_dir += '/';

  DIR *dir = opendir(_statFileName);
  if (dir == NULL) {
    XOSDEBUG("sysfs: Cannot open directory : %s\n", _statFileName);
    return;
  }

  // visit every /sys/block/*/stat and sum up the values:
  for (struct dirent *dirent; (dirent = readdir(dir)) != NULL; ) {
    if (strncmp(dirent->d_name, ".", 1) == 0 ||
        strncmp(dirent->d_name, "..", 2) == 0 ||
        strncmp(dirent->d_name, "loop", 4) == 0 ||
        strncmp(dirent->d_name, "ram", 3) == 0)
      continue;

    disk = sysfs_dir + dirent->d_name;
    if (stat(disk.c_str(), &buf) == 0 && buf.st_mode & S_IFDIR) {
      // only scan for real HW (raid, md, and lvm all mapped on them)
      tmp = disk + "/device";
      if (lstat(tmp.c_str(), &buf) != 0 || (buf.st_mode & S_IFLNK) == 0)
        continue;

      // is a dir, locate 'stat' file in it
      disk += "/stat";
      diskstat.open(disk.c_str());
      if (diskstat.good()) {
        diskstat.getline(line, 128);
        char *cur = line, *end = NULL;
        for (int i = 0; i < 7; i++) {
          vals[i] = strtoul(cur, &end, 10);
          cur = end;
        }
        reads[dirent->d_name]  = vals[2];
        writes[dirent->d_name] = vals[6];

        XOSDEBUG("disk stat: %s | read: %lu, written: %lu\n", disk.c_str(), vals[2], vals[6]);
        diskstat.close();
        diskstat.clear();
      }
      else
        XOSDEBUG("disk stat open: %s - errno=%d\n", disk.c_str(), errno);
    }
    else
      XOSDEBUG("disk is not dir: %s - errno=%d\n", disk.c_str(), errno);
  } // for
  closedir(dir);
  update_info(reads, writes);
}
