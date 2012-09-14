//
//  Copyright (c) 1999, 2006 by Mike Romberg (mike.romberg@noaa.gov)
//
//  This file may be distributed under terms of the GPL
//

#include "diskmeter.h"
#include "xosview.h"
#include <fstream>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>


#define MAX_PROCSTAT_LENGTH 2048

DiskMeter::DiskMeter( XOSView *parent, float max ) : FieldMeterGraph(
  parent, 3, "DISK", "READ/WRITE/IDLE"), _vmstat(false),
  _statFileName("/proc/stat")
{
    read_prev_ = 0;
    write_prev_ = 0;
    maxspeed_ = max;

    _sysfs=_vmstat=false;
    sysfs_read_prev_=sysfs_write_prev_=0L;
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
// (no dependency on sector-size here )
void DiskMeter::update_info(unsigned long long rsum, unsigned long long wsum)
{
    float itim = IntervalTimeInMicrosecs();

    // avoid strange values at first call
    // (by this - the first value displayed becomes zero)
    if(sysfs_read_prev_ == 0L)
    { 
        sysfs_read_prev_  = rsum;
        sysfs_write_prev_ = wsum;
        itim = 1; 	// itim is garbage here too. Valgrind complains.
    }

    // convert rate from bytes/microsec into bytes/second
    fields_[0] = ((rsum - sysfs_read_prev_ ) * 1e6 ) / itim;
    fields_[1] = ((wsum - sysfs_write_prev_) * 1e6 ) / itim;

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
    sysfs_read_prev_  = rsum;
    sysfs_write_prev_ = wsum;

    setUsed((fields_[0]+fields_[1]), total_);
    IntervalTimerStart();
}



// XXX: sysfs - read Documentation/iostats.txt !!!
// extract stats from /sys/block/*/stat
// each disk reports a 32bit u_int, which can WRAP around
// XXX: currently sector-size is fixed 512bytes
//      (would need a sysfs-val for sect-size)

void DiskMeter::getsysfsdiskinfo( void )
{
        // field-3: sects read since boot (but can wrap!)
        // field-7: sects written since boot (but can wrap!)
        // just sum up everything in /sys/block/*/stat

  std::string sysfs_dir = _statFileName;
  std::string disk;
  struct stat buf;
  std::ifstream diskstat;

  // the sum of all disks:
  unsigned long long all_bytes_read,all_bytes_written;

  // ... while this is only one disk's value:
  unsigned long sec_read,sec_written;
  unsigned long sect_size;

  unsigned long dummy;

  IntervalTimerStop();
  total_ = maxspeed_;

  DIR *dir = opendir(_statFileName);
  if (dir==NULL) {
    XOSDEBUG("sysfs: Cannot open directory : %s\n", _statFileName);
    return;
  }

  // reset all sums
  all_bytes_read=all_bytes_written=0L;
  sect_size=0L;

  // visit every /sys/block/*/stat and sum up the values:

  for (struct dirent *dirent; (dirent = readdir(dir)) != NULL; ) {
    if (strncmp(dirent->d_name, ".", 1) == 0
        || strncmp(dirent->d_name, "..", 2) == 0)
      continue;

    disk = sysfs_dir + "/" + dirent->d_name;

    if (stat(disk.c_str(), &buf) == 0 && buf.st_mode & S_IFDIR) {
       // is a dir, locate 'stat' file in it
       disk = disk + "/stat";
       if (stat(disk.c_str(), &buf) == 0 && buf.st_mode & S_IFREG) {
                XOSDEBUG("disk stat: %s\n",disk.c_str() );
                diskstat.open(disk.c_str());
                if ( diskstat.good() ) {
                   sec_read=sec_written=0L;
                   diskstat >> dummy >> dummy >> sec_read >> dummy >> dummy >> dummy >> sec_written;

                   sect_size = 512; // XXX: not always true

                   // XXX: ignoring wrap around case for each disk
                   // (would require saving old vals for each disk etc..)
                   all_bytes_read    += (unsigned long long) sec_read * (unsigned long long) sect_size;
                   all_bytes_written += (unsigned long long) sec_written * (unsigned long long) sect_size;

                   XOSDEBUG("disk stat: %s | read: %ld, written: %ld\n",disk.c_str(),sec_read,sec_written );
                   diskstat.close(); diskstat.clear();
                } else {
                  XOSDEBUG("disk stat open: %s - errno=%d\n",disk.c_str(),errno );
                }
       } else {
        XOSDEBUG("disk stat is not file: %s - errno=%d\n",disk.c_str(),errno );
       }
    } else {
        XOSDEBUG("disk is not dir: %s - errno=%d\n",disk.c_str(),errno );
    }
  } // for
  closedir(dir);
  XOSDEBUG("disk: read: %lld, written: %lld\n",all_bytes_read, all_bytes_written );
  update_info(all_bytes_read, all_bytes_written);
}
