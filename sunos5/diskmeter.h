//  
//  Copyright (c) 1999 by Mike Romberg (romberg@fsl.noaa.gov)
//
//  This file may be distributed under terms of the GPL
//

#ifndef _DISKMETER_H_
#define _DISKMETER_H_

#include "fieldmetergraph.h"
#include <kstat.h>
#define NPARTS 100

class DiskMeter : public FieldMeterGraph 
    {
    public:
        DiskMeter( XOSView *parent, kstat_ctl_t *_kc, float max );
        ~DiskMeter( void );

        const char *name( void ) const { return "DiskMeter"; }
        void checkevent( void );
        
        void checkResources( void );
    protected:

        void getdiskinfo( void );
    private:
        u_longlong_t read_prev_;
        u_longlong_t write_prev_;
        float maxspeed_;
	int npart;
	kstat_ctl_t *kc;
	kstat_t *part[NPARTS];
    };

#endif
