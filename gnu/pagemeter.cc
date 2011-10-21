//
//  Copyright (c) 1996, 2004 by Massimiliano Ghilardi ( ghilardi@cibs.sns.it )
//  2007 by Samuel Thibault ( samuel.thibault@ens-lyon.org )
//
//  This file may be distributed under terms of the GPL
//

#include "pagemeter.h"
#include "xosview.h"
#include <fstream>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <error.h>

extern "C" {
#include <mach/mach_traps.h>
#include <mach/mach_interface.h>
}

PageMeter::PageMeter( XOSView *parent, float max )
  : FieldMeterGraph( parent, 3, "PAGE", "IN/OUT/IDLE" ) {
  for ( int i = 0 ; i < 2 ; i++ )
    for ( int j = 0 ; j < 2 ; j++ )
      pageinfo_[j][i] = 0;

  maxspeed_ = max;
  pageindex_ = 0;
}

PageMeter::~PageMeter( void ){
}

void PageMeter::checkResources( void ){
  FieldMeterGraph::checkResources();

  setfieldcolor( 0, parent_->getResource( "pageInColor" ) );
  setfieldcolor( 1, parent_->getResource( "pageOutColor" ) );
  setfieldcolor( 2, parent_->getResource( "pageIdleColor" ) );
  priority_ = atoi (parent_->getResource( "pagePriority" ) );
  maxspeed_ *= priority_ / 10.0;
  dodecay_ = parent_->isResourceTrue( "pageDecay" );
  useGraph_ = parent_->isResourceTrue( "pageGraph" );
  SetUsedFormat (parent_->getResource("pageUsedFormat"));
}

void PageMeter::checkevent( void ){
  getpageinfo();
  drawfields();
}

void PageMeter::updateinfo(void)
    {
    int oldindex = (pageindex_+1)%2;
    for ( int i = 0; i < 2; i++ )
        {
        if ( pageinfo_[oldindex][i] == 0 )
            pageinfo_[oldindex][i] = pageinfo_[pageindex_][i];

        fields_[i] = pageinfo_[pageindex_][i] - pageinfo_[oldindex][i];
        total_ += fields_[i];
        }

    if ( total_ > maxspeed_ )
        fields_[2] = 0.0;
    else
        {
        fields_[2] = maxspeed_ - total_;
        total_ = maxspeed_;
        }

    setUsed (total_ - fields_[2], maxspeed_);
    pageindex_ = (pageindex_ + 1) % 2;
    }

void PageMeter::getpageinfo(void) {
  struct vm_statistics vmstats;

  total_ = 0;
  kern_return_t err;

  err = vm_statistics (mach_task_self(), &vmstats);
  if (err) {
    error (0, err, "vm_statistics");
    parent_->done(1);
    return;
  }

  pageinfo_[pageindex_][0] = vmstats.pageins;
  pageinfo_[pageindex_][1] = vmstats.pageouts;

  updateinfo();
}
