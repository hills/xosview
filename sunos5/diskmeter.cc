//  
//  Rewritten for Solaris by Arno Augustin 1999
//  augustin@informatik.uni-erlangen.de
//

#include "diskmeter.h"
#include <stdlib.h>


DiskMeter::DiskMeter( XOSView *parent, kstat_ctl_t *kc, float max )
    : FieldMeterGraph( parent, 3, "DISK", "READ/WRITE/IDLE" )
{
    _kc = kc;
    _read_prev = _write_prev = 0;
    _maxspeed = max;
    _disks = KStatList::getList(_kc, KStatList::DISKS);
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
    priority_ = atoi( parent_->getResource("diskPriority") );
    dodecay_ = parent_->isResourceTrue("diskDecay");
    useGraph_ = parent_->isResourceTrue("diskGraph");
    SetUsedFormat( parent_->getResource("diskUsedFormat") );
}

void DiskMeter::checkevent( void )
{
    getdiskinfo();
    drawfields();
}

void DiskMeter::getdiskinfo( void )
{
    total_ = _maxspeed;
    kstat_io_t kio;
    uint64_t read_curr = 0, write_curr = 0;
    _disks->update(_kc);

    IntervalTimerStop();
    for (unsigned int i = 0; i < _disks->count(); i++) {
        if ( kstat_read(_kc, (*_disks)[i], &kio) == -1 )
            continue;
        XOSDEBUG("%s: %llu bytes read %llu bytes written.\n",
                 (*_disks)[i]->ks_name, kio.nread, kio.nwritten);
        read_curr += kio.nread;
        write_curr += kio.nwritten;
    }
    if (_read_prev == 0)
        _read_prev = read_curr;
    if (_write_prev == 0)
        _write_prev = write_curr;

    double t = IntervalTimeInSecs();
    fields_[0] = (double)(read_curr - _read_prev) / t;
    fields_[1] = (double)(write_curr - _write_prev) / t;

    IntervalTimerStart();
    _read_prev = read_curr;
    _write_prev = write_curr;

    if (fields_[0] < 0)
        fields_[0] = 0;
    if (fields_[1] < 0)
        fields_[1] = 0;
    if (fields_[0] + fields_[1] > total_)
        total_ = fields_[0] + fields_[1];
    fields_[2] = total_ - (fields_[0] + fields_[1]);
    setUsed(fields_[0] + fields_[1], total_);
}
