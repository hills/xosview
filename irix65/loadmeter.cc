//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//  Most of this code was written by Werner Fink <werner@suse.de>.
//  Only small changes were made on my part (M.R.)
//  Small changes for Irix 6.5 port Stefan Eilemann <eilemann@gmail.com>
//

#include "loadmeter.h"
#include "xosview.h"
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

#ifndef FSCALE
#define FSCALE  (1 << 8)
#endif

using namespace std;

LoadMeter::LoadMeter(XOSView *parent)
        : FieldMeterGraph( parent, 2, "LOAD", "PROCS/MIN", 1, 1, 0 )
{
    lastalarmstate = -1;
    total_ = 2.0;

    if (gethostname (hostname, 255) != 0) {
        perror ("gethostname");
        parent_->done(1);
        return;
    }
}

LoadMeter::~LoadMeter(void)
{
}

void LoadMeter::checkResources(void)
{
    FieldMeterGraph::checkResources();

    procloadcol_ = parent_->allocColor(parent_->getResource( "loadProcColor" ));
    warnloadcol_ = parent_->allocColor(parent_->getResource( "loadWarnColor" ));
    critloadcol_ = parent_->allocColor(parent_->getResource( "loadCritColor" ));

    setfieldcolor(0, parent_->getResource( "loadProcColor" ));
    setfieldcolor(1, parent_->getResource("loadIdleColor"));
    priority_ = atoi (parent_->getResource("loadPriority"));
    useGraph_ = parent_->isResourceTrue("loadGraph");
    dodecay_ = parent_->isResourceTrue( "loadDecay" );
    SetUsedFormat(parent_->getResource("loadUsedFormat"));

    warnThreshold = atoi (parent_->getResource("loadWarnThreshold"));
    critThreshold = atoi (parent_->getResource("loadCritThreshold"));


    if (dodecay_){
        //  Warning:  Since the loadmeter changes scale occasionally, old
        //  decay values need to be rescaled.  However, if they are rescaled,
        //  they could go off the edge of the screen.  Thus, for now, to
        //  prevent this whole problem, the load meter can not be a decay
        //  meter.  The load is a decaying average kind of thing anyway,
        //  so having a decaying load average is redundant.
           std::cerr << "Warning:  The loadmeter can not be configured as a decay\n"
             << "  meter. See the source code (" <<__FILE__<< ") for further\n"
             << "  details.\n";
        dodecay_ = 0;
    }
}

void LoadMeter::checkevent(void)
{
    getloadinfo();
    drawfields();
}

void LoadMeter::getloadinfo(void)
{
    if (rstat (hostname, &res) != 0) {
           std::cerr << hostname <<endl;
        perror ("rstat");
        return;
    }

    fields_[0] = (float) res.avenrun[0]/FSCALE;
    
    if ( fields_[0] < warnThreshold ) 
        alarmstate = 0;
    else
        if ( fields_[0] >= critThreshold )
            alarmstate = 2;
        else
            alarmstate = 1;
  
    if ( alarmstate != lastalarmstate )
    {
        if ( alarmstate == 0 )
            setfieldcolor( 0, procloadcol_ );
        else
            if ( alarmstate == 1 )
                setfieldcolor( 0, warnloadcol_ );
            else
                setfieldcolor( 0, critloadcol_ );
	drawlegend();
        lastalarmstate = alarmstate;
    }
  
    if ( fields_[0]*5.0<total_ )
        total_ = fields_[0];
    else
        if ( fields_[0]>total_ )
            total_ = fields_[0]*5.0;
      
    if ( total_ < 1.0)
        total_ = 1.0;
    
    fields_[1] = (float) (total_ - fields_[0]);

    setUsed(fields_[0], (float) 1.0);
}
