//  
//  Copyright (c) 1999 by Mike Romberg (romberg@fsl.noaa.gov)
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

        void getdiskinfo( void );
    private:
        unsigned long int prev_;
        float maxspeed_;
    };

#endif
