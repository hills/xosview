//
//  Copyright (c) 1999, 2006 by Mike Romberg (mike.romberg@noaa.gov)
//
//  This file may be distributed under terms of the GPL
//

#ifndef _TAPEMETER_H_
#define _TAPEMETER_H_

#include "fieldmetergraph.h"
#include "xosview.h"
#include <string>

static const char TAPESTATDIR[] = "/sys/class/scsi_tape";

class TapeMeter : public FieldMeterGraph
    {
    public:
        TapeMeter( XOSView *parent, float max, const std::string &dev );
        ~TapeMeter( void );

        const char *name( void ) const { return "TapeMeter"; }
        void checkevent( void );

        void checkResources( void );

        // number of tape drives (stN entries) found in TAPESTATDIR
        static int countTapes( void );
        // device name ("st0") of the n-th tape drive
        static std::string driveName( int i );
    protected:

        void getsysfstapeinfo( void );
        // "LTO5" from the device model, or "ST0" as fallback
        static std::string driveLabel( const std::string &dev );
    private:

        unsigned long long read_prev_, write_prev_;
        float maxspeed_;
        std::string dev_;
        bool first_;
    };

#endif
