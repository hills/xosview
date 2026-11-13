//
//  Copyright (c) 1999, 2006 by Mike Romberg (mike.romberg@noaa.gov)
//
//  This file may be distributed under terms of the GPL
//
//  Linux SCSI tape (st) drive throughput meter.
//  Reads byte counters from /sys/class/scsi_tape/st<N>/stats/.
//  These are the same counters tapestat(1) uses (see
//  Documentation/scsi/st.rst in the kernel tree).
//

#include "tapemeter.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>


TapeMeter::TapeMeter( XOSView *parent, float max, const std::string &dev )
  : FieldMeterGraph( parent, 3, driveLabel(dev).c_str(), "READ/WRITE/IDLE" ),
    read_prev_(0), write_prev_(0), maxspeed_(max), dev_(dev), first_(true)
{
}

TapeMeter::~TapeMeter( void )
    {
    }

void TapeMeter::checkResources( void )
    {
    FieldMeterGraph::checkResources();

    setfieldcolor( 0, parent_->getResource("tapeReadColor") );
    setfieldcolor( 1, parent_->getResource("tapeWriteColor") );
    setfieldcolor( 2, parent_->getResource("tapeIdleColor") );
    priority_ = atoi (parent_->getResource( "tapePriority" ) );
    dodecay_ = parent_->isResourceTrue("tapeDecay" );
    useGraph_ = parent_->isResourceTrue( "tapeGraph" );
    SetUsedFormat(parent_->getResource("tapeUsedFormat"));
    }

void TapeMeter::checkevent( void )
    {
    getsysfstapeinfo();
    drawfields();
    }

// return true if dirent names a rewinding tape device (st0, st1, ...).
// Each physical drive also gets nstN, stNa, stNl, stNm, nstNa, ...
// entries which alias the same device and report identical counters,
// so they must be skipped to avoid counting traffic multiple times.
static int isTapeDrive( const char *name )
    {
    if (name[0] != 's' || name[1] != 't' || name[2] == '\0')
        return 0;
    for (const char *p = name + 2; *p; p++)
        if (*p < '0' || *p > '9')
            return 0;
    return 1;
    }

// compare drive names numerically ("st2" before "st10")
static bool tapeDriveLess( const std::string &a, const std::string &b )
    {
    return atoi(a.c_str() + 2) < atoi(b.c_str() + 2);
    }

static std::vector<std::string> tapeDrives( void )
    {
    std::vector<std::string> drives;
    DIR *dir = opendir(TAPESTATDIR);
    if (dir == NULL)
        return drives;
    for (struct dirent *dirent; (dirent = readdir(dir)) != NULL; )
        if (isTapeDrive(dirent->d_name))
            drives.push_back(dirent->d_name);
    closedir(dir);
    std::sort(drives.begin(), drives.end(), tapeDriveLess);
    return drives;
    }

int TapeMeter::countTapes( void )
    {
    return tapeDrives().size();
    }

std::string TapeMeter::driveName( int i )
    {
    std::vector<std::string> drives = tapeDrives();
    if (i < 0 || (unsigned int)i >= drives.size())
        return "";
    return drives[i];
    }

// Build a short title for the drive from its SCSI INQUIRY model,
// e.g. HP "Ultrium 3-SCSI" -> "LTO3", IBM "ULT3580-HH5" -> "LTO5".
// Falls back to the uppercased device name ("st0" -> "ST0").
std::string TapeMeter::driveLabel( const std::string &dev )
    {
    std::string path = std::string(TAPESTATDIR) + '/' + dev + "/device/model";
    std::ifstream modelfile(path.c_str());
    std::string model;
    if (modelfile)
        std::getline(modelfile, model);

    std::string up;
    up.reserve(model.size());
    for (std::string::const_iterator it = model.begin(); it != model.end(); ++it)
        up += toupper((unsigned char)*it);

    // look for the LTO generation after "ULTRIUM" or "-HH"
    size_t pos = up.find("ULTRIUM");
    if (pos == std::string::npos)
        pos = up.find("-HH");
    if (pos != std::string::npos) {
        pos = up.find_first_of("0123456789", pos);
        if (pos != std::string::npos) {
            size_t end = up.find_first_not_of("0123456789", pos);
            return "LTO" + up.substr(pos, end - pos);
        }
    }

    // fallback: "st0" -> "ST0"
    up.clear();
    up.reserve(dev.size());
    for (std::string::const_iterator it = dev.begin(); it != dev.end(); ++it)
        up += toupper((unsigned char)*it);
    return up;
    }

// read counters from /sys/class/scsi_tape/st<N>/stats/
// read_byte_cnt and write_byte_cnt are cumulative byte counters
void TapeMeter::getsysfstapeinfo( void )
    {
    std::string statdir = std::string(TAPESTATDIR) + '/' + dev_ + "/stats/";
    std::string statfile;
    std::ifstream tapestat;
    unsigned long long read_curr = read_prev_, write_curr = write_prev_;

    IntervalTimerStop();
    total_ = maxspeed_;

    statfile = statdir + "read_byte_cnt";
    tapestat.open(statfile.c_str());
    if (tapestat.good()) {
      tapestat >> read_curr;
      tapestat.close();
      tapestat.clear();
    }
    else
      XOSDEBUG("tape stat open: %s - errno=%d\n", statfile.c_str(), errno);

    statfile = statdir + "write_byte_cnt";
    tapestat.open(statfile.c_str());
    if (tapestat.good()) {
      tapestat >> write_curr;
      tapestat.close();
      tapestat.clear();
    }
    else
      XOSDEBUG("tape stat open: %s - errno=%d\n", statfile.c_str(), errno);

    XOSDEBUG("tape stat: %s | read: %llu, written: %llu\n",
      statdir.c_str(), read_curr, write_curr);

    float itim = IntervalTimeInMicrosecs();

    // avoid strange values at first call
    // (by this - the first value displayed becomes zero)
    if (first_)
    {
        read_prev_  = read_curr;
        write_prev_ = write_curr;
        first_ = false;
        itim = 1;	// itim is garbage here too. Valgrind complains.
    }

    // counter wrapped
    if (read_curr < read_prev_)
        fields_[0] = (double)(ULLONG_MAX - read_prev_ + read_curr) * 1e6 / itim;
    else
        fields_[0] = (double)(read_curr - read_prev_) * 1e6 / itim;
    if (write_curr < write_prev_)
        fields_[1] = (double)(ULLONG_MAX - write_prev_ + write_curr) * 1e6 / itim;
    else
        fields_[1] = (double)(write_curr - write_prev_) * 1e6 / itim;

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
    read_prev_  = read_curr;
    write_prev_ = write_curr;

    setUsed(fields_[0] + fields_[1], total_);
    IntervalTimerStart();
    }
