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
#include <fstream.h>
#include <stdlib.h>


static const char STATFILENAME[] = "/proc/stat";


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
  dodecay_ = !strncasecmp (parent_->getResource( "pageDecay" ), "True", 5 );
  useGraph_ = !strncasecmp (parent_->getResource( "pageGraph" ), "True", 5 );
  SetUsedFormat (parent_->getResource("pageUsedFormat"));
}

void PageMeter::checkevent( void ){
  getpageinfo();
  drawfields();
}

void PageMeter::getpageinfo( void ){
  total_ = 0;
  char buf[256];
  ifstream stats( STATFILENAME );

  if ( !stats ){
    cerr <<"Cannot open file : " <<STATFILENAME <<endl;
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


// void PageMeter::getpageinfo( void ){
//   total_ = 0;
//   char buf[256];
//   ifstream stats( STATFILENAME );

//   if ( !stats ){
//     cerr <<"Cannot open file : " <<STATFILENAME <<endl;
//     exit( 1 );
//   }

//   stats.getline( buf, 256 );
//   stats.getline( buf, 256 );
//   stats.getline( buf, 256 );

//   stats >>buf >>pageinfo_[pageindex_][0]
// 	      >>pageinfo_[pageindex_][1];

//   int oldindex = (pageindex_+1)%2;
  
//   for ( int i = 0; i < 2; i++ ) {
//     if ( pageinfo_[oldindex][i] == 0 )
//       pageinfo_[oldindex][i] = pageinfo_[pageindex_][i];

//     fields_[i] = pageinfo_[pageindex_][i] - pageinfo_[oldindex][i];
//     total_ += fields_[i];
//   }

//   if ( total_ > maxspeed_ )
//     fields_[2] = 0.0;
//   else {
//     fields_[2] = maxspeed_ - total_;
//     total_ = maxspeed_;
//   }

//   "Ack.  Use setUsed instead.  bgrayson"  used( (int)((100 * (total_ - fields_[2])) / maxspeed_) );
//   pageindex_ = (pageindex_ + 1) % 2;
// }
