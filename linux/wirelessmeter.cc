//  
//  Copyright (c) 2001 by Tim Ehlers ( tehlers@gwdg.de )
//
//  This file may be distributed under terms of the GPL
//
//
// $Id: wirelessmeter.cc,v 1.3 2001/06/13
//
#include "wirelessmeter.h"
#include "xosview.h"
#include <fstream>
#include <stdlib.h>
#include <string.h>
//#include <strstream>
#include <sstream>
#include <ctype.h>

using namespace std;

static const char WLFILENAME[] = "/proc/net/wireless";

WirelessMeter::WirelessMeter( XOSView *parent, int ID, const char *wlID)
  : FieldMeterGraph ( parent, 2, wlID, "/Link-Quality", 1, 1, 0 ), _number(ID) {
  lastqualitystate = -1;
  strcpy(devname, "0");
}

WirelessMeter::~WirelessMeter( void ){
}

void WirelessMeter::checkResources( void ){
  FieldMeterGraph::checkResources();

  poorqualcol_ = parent_->allocColor(parent_->getResource( "PoorQualityColor" ));
  fairqualcol_ = parent_->allocColor(parent_->getResource( "FairQualityColor" ));
  goodqualcol_ = parent_->allocColor(parent_->getResource( "GoodQualityColor" ));

  setfieldcolor( 1, parent_->getResource( "wirelessUsedColor" ) );

  priority_ = atoi (parent_->getResource( "wirelessPriority" ) );
  dodecay_ = parent_->isResourceTrue( "wirelessDecay" );
  SetUsedFormat(parent_->getResource( "wirelessUsedFormat" ) );
}

void WirelessMeter::checkevent( void ){
  getpwrinfo();

  drawfields();
}


void WirelessMeter::getpwrinfo( void ){
  ifstream loadinfo( WLFILENAME );

  if ( !loadinfo ){
    cerr <<"Can not open file : " <<WLFILENAME <<endl;
    parent_->done(1);
    return;
  }

  char buff[256];

  for (int i = 1 ; i < 2 ; i++)
    loadinfo.getline(buff, 256);

  int linkq = 0;

if (strncmp(devname, "0", 1)) {

  while (!loadinfo.eof()){
    loadinfo.getline(buff, 256);
    if (!loadinfo.eof()){
        loadinfo >> buff;
      if (!strncmp(buff, devname, strlen(devname))) {
	loadinfo >> buff >> linkq; }

} } }

if (!strncmp(devname, "0", 1)) {

  for (int i = 0 ; i < _number; ) {
    loadinfo.getline(buff, 256);
	loadinfo >> devname >> buff >> linkq;
	if (!loadinfo.eof()){
          if ( linkq != 0 ) i++; }
		if ( loadinfo.eof() ) break; }

}

  fields_[0] = linkq;

  if ( fields_[0] <  7 ) qualitystate = 0;
  if ( fields_[0] >= 7 ) qualitystate = 1;
  if ( fields_[0] >= 15 ) qualitystate = 2;
  
  if ( qualitystate != lastqualitystate ){
    if ( qualitystate == 0 ) setfieldcolor( 0, poorqualcol_ );
    if ( qualitystate == 1 ) setfieldcolor( 0, fairqualcol_ );
    if ( qualitystate == 2 ) setfieldcolor( 0, goodqualcol_ );
    lastqualitystate = qualitystate;
  }

    if ( fields_[0] >= 250 ) { fields_[0] = 0; qualitystate = 0; }

  total_ = 240;

 if ( fields_[0] < 210)
    total_ = 210;

 if ( fields_[0] < 180)
    total_ = 180;

 if ( fields_[0] < 150)
    total_ = 150;

 if ( fields_[0] < 120)
    total_ = 120;

 if ( fields_[0] < 90)
    total_ = 90;

 if ( fields_[0] < 60)
    total_ = 60;

 if ( fields_[0] < 30)
    total_ = 30;

  fields_[1] = fields_[0];

  setUsed (fields_[0], total_);
}

int WirelessMeter::countdevices(void){
  ifstream stats( WLFILENAME );

  if ( !stats ){
    cerr <<"Can not open file : " <<WLFILENAME <<endl;
    exit( 1 );
  }

char buf[1024];

for (int i = 1 ; i < 2 ; i++)
    stats.getline(buf, 1024);

  int wirelessCount = 0;
  int linkq = 0;
  while (!stats.eof()){
    stats.getline(buf, 1024);
	stats >> buf >> buf >> linkq;
if (!stats.eof()){
	  if ( linkq != 0 )
          wirelessCount++;
  }}

  return wirelessCount;
}

const char *WirelessMeter::wirelessStr(int num){
  static char buffer[32];
  std::ostringstream str;
  
  str << "WL";
  if (num != 1)
    str << (num);
  str << std::ends;

  strncpy(buffer, str.str().c_str(), 32);
  buffer[31] = '\0';

  return buffer;
}
