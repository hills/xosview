//
//  Copyright (c) 1999, 2006 by Mike Romberg (mike.romberg@noaa.gov)
//
//  This file may be distributed under terms of the GPL
//

#ifndef _DISKMETER_H_
#define _DISKMETER_H_

#include "fieldmetergraph.h"

class DiskMeter : public FieldMeterGraph
    {
    public:
        DiskMeter( XOSView *parent, float max );
        ~DiskMeter( void );

        const char *name( void ) const { return "DiskMeter"; }
        void checkevent( void );

        void checkResources( void );
    protected:

	// sysfs:
	void update_info(unsigned long long rsum, unsigned long long wsum);
	void getsysfsdiskinfo( void );

        void getdiskinfo( void );
        void getvmdiskinfo( void );
        void updateinfo(unsigned long one, unsigned long two,
          int fudgeFactor);
    private:

	// sysfs:
	unsigned long long sysfs_read_prev_;
	unsigned long long sysfs_write_prev_;
        bool _sysfs;

        unsigned long int read_prev_;
        unsigned long int write_prev_;
        float maxspeed_;
        bool _vmstat;
        const char *_statFileName;
    };

#endif
