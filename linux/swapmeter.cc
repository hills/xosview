//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "swapmeter.h"
#include "xosview.h"
#include <fstream>
#include <sstream>
#include <stdlib.h>

static const char MEMFILENAME[] = "/proc/meminfo";


SwapMeter::SwapMeter( XOSView *parent )
: FieldMeterGraph( parent, 2, "SWAP", "USED/FREE" ){

}

SwapMeter::~SwapMeter( void ){
}

void SwapMeter::checkResources( void ){
  FieldMeterGraph::checkResources();

  setfieldcolor( 0, parent_->getResource( "swapUsedColor" ) );
  setfieldcolor( 1, parent_->getResource( "swapFreeColor" ) );
  priority_ = atoi (parent_->getResource( "swapPriority" ) );
  dodecay_ = parent_->isResourceTrue( "swapDecay" );
  useGraph_ = parent_->isResourceTrue( "swapGraph" );
  SetUsedFormat (parent_->getResource("swapUsedFormat"));
}

void SwapMeter::checkevent( void ){
  getswapinfo();
  drawfields();
}


void SwapMeter::getswapinfo( void ){
  std::ifstream meminfo( MEMFILENAME );
  if ( !meminfo ){
    std::cerr <<"Cannot open file : " <<MEMFILENAME << std::endl;
    exit( 1 );
  }

  total_ = fields_[0] = fields_[1] = 0;

  char buf[256];
  bool found_total = false, found_free = false;
  unsigned long long val;

  // Get the info from the "standard" meminfo file.
  while ( !meminfo.eof() && !(found_total && found_free) ){
    meminfo.getline(buf, 256);

    if ( !strncmp(buf, "SwapTotal", 9) ){
      val = strtoull(buf+10, NULL, 10);
      total_ = val<<10; // unit is always kB
      found_total = true;
    }
    if ( !strncmp(buf, "SwapFree", 8) ){
      val = strtoull(buf+9, NULL, 10);
      fields_[1] = val<<10; // unit is always kB
      found_free = true;
    }
  }

  fields_[0] = total_ - fields_[1];

  if ( total_ == 0 ){
    total_ = 1;
    fields_[0] = 0;
    fields_[1] = 1;
  }

  if (total_)
    setUsed (fields_[0], total_);
}
