//
//  Copyright (c) 1996 by Massimiliano Ghilardi ( ghilardi@cibs.sns.it )
//
//  This file may be distributed under terms of the GPL
//
//
// $Id$
//
#include "pagemeter.h"
#include "xosview.h"
#include <fstream>
#include <stdlib.h>


static const char STATFILENAME[] = "/proc/stat";
#define MAX_PROCSTAT_LENGTH 2048


PageMeter::PageMeter( XOSView *parent, float max )
  : FieldMeterGraph( parent, 3, "PAGE", "IN/OUT/IDLE" ){
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

void PageMeter::getpageinfo( void ){
  total_ = 0;
  char buf[MAX_PROCSTAT_LENGTH];
  std::ifstream stats( STATFILENAME );

  if ( !stats ){
    std::cerr <<"Cannot open file : " <<STATFILENAME << std::endl;
    exit( 1 );
  }

  do {
    stats >>buf;
  } while (strncasecmp(buf, "swap", 5));

  stats >>pageinfo_[pageindex_][0] >>pageinfo_[pageindex_][1];

  int oldindex = (pageindex_+1)%2;

  for ( int i = 0; i < 2; i++ ) {
    if ( pageinfo_[oldindex][i] == 0 )
      pageinfo_[oldindex][i] = pageinfo_[pageindex_][i];

    fields_[i] = pageinfo_[pageindex_][i] - pageinfo_[oldindex][i];
    total_ += fields_[i];
  }

  if ( total_ > maxspeed_ )
    fields_[2] = 0.0;
  else {
    fields_[2] = maxspeed_ - total_;
    total_ = maxspeed_;
  }

  setUsed (total_ - fields_[2], maxspeed_);
  pageindex_ = (pageindex_ + 1) % 2;
}
