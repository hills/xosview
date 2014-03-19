//
//  Copyright (c) 1996, 2004 by Massimiliano Ghilardi ( ghilardi@cibs.sns.it )
//
//  This file may be distributed under terms of the GPL
//

#include "pagemeter.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fstream>
#include <iostream>

#define MAX_PROCSTAT_LENGTH 4096


PageMeter::PageMeter( XOSView *parent, float max )
  : FieldMeterGraph( parent, 3, "PAGE", "IN/OUT/IDLE" ),
  _vmstat(false), _statFileName("/proc/stat"){
  for ( int i = 0 ; i < 2 ; i++ )
    for ( int j = 0 ; j < 2 ; j++ )
      pageinfo_[j][i] = 0;

  maxspeed_ = max;
  pageindex_ = 0;

  struct stat buf;
  if (stat("/proc/vmstat", &buf) == 0
    && buf.st_mode & S_IFREG)
      {
      _vmstat = true;
      _statFileName = "/proc/vmstat";
      }
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
 if (_vmstat)
     getvmpageinfo();
 else
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

void PageMeter::getvmpageinfo(void)
    {
    total_ = 0;
    char buf[MAX_PROCSTAT_LENGTH];
    bool found_in = false, found_out = false;
    std::ifstream stats(_statFileName);
    if (!stats)
        {
        std::cerr <<"Cannot open file : " << _statFileName << std::endl;
        exit(1);
        }
    while (!stats.eof() && !(found_in && found_out))
        {
        stats.getline(buf, MAX_PROCSTAT_LENGTH);
        if (!strncmp(buf, "pswpin", 6))
            {
            pageinfo_[pageindex_][0] = strtoul(buf+7, NULL, 10);
            found_in = true;
            }
        if (!strncmp(buf, "pswpout", 7))
            {
            pageinfo_[pageindex_][1] = strtoul(buf+8, NULL, 10);
            found_out = true;
            }
        }
    updateinfo();
    }

void PageMeter::getpageinfo( void ){
  total_ = 0;
  char buf[MAX_PROCSTAT_LENGTH];
  std::ifstream stats(_statFileName);

  if ( !stats ){
    std::cerr <<"Cannot open file : " << _statFileName << std::endl;
    exit( 1 );
  }

  do {
    stats >>buf;
  } while (!stats.eof() && strncasecmp(buf, "swap", 5));

  stats >>pageinfo_[pageindex_][0] >>pageinfo_[pageindex_][1];

  updateinfo();
}
