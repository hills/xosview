//  
//  Copyright (c) 1999 by Mike Romberg (romberg@fsl.noaa.gov)
//
//  This file may be distributed under terms of the GPL
//

#ifndef _DISKMETER_H_
#define _DISKMETER_H_

#include "fieldmetergraph.h"
#include "xosview.h"
#include "kstats.h"
#include <kstat.h>


class DiskMeter : public FieldMeterGraph {
 public:
    DiskMeter( XOSView *parent, kstat_ctl_t *kc, float max );
    ~DiskMeter( void );

    const char *name( void ) const { return "DiskMeter"; }
    void checkevent( void );
    void checkResources( void );

 protected:
    void getdiskinfo( void );

 private:
    uint64_t _read_prev, _write_prev;
    float _maxspeed;
    kstat_ctl_t *_kc;
    KStatList *_disks;
};

#endif
