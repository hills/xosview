//
//  Copyright (c) 2001 by Tim Ehlers ( tehlers@gwdg.de )
//
//  This file may be distributed under terms of the GPL
//
//

#include "wirelessmeter.h"
#include <stdlib.h>
#include <stdio.h>
#include <glob.h>
#include <fstream>
#include <iostream>


WirelessMeter::WirelessMeter( XOSView *parent, int ID, const char *wlID)
  : FieldMeterGraph ( parent, 2, wlID, "LINK/LEVEL", 1, 1, 0 ), _number(ID) {
  _lastquality = -1;
  _lastlink = true;
  total_ = 0;
}

WirelessMeter::~WirelessMeter( void ){
}

void WirelessMeter::checkResources( void ){
  FieldMeterGraph::checkResources();

  _poorqualcol = parent_->allocColor(parent_->getResource( "PoorQualityColor" ));
  _fairqualcol = parent_->allocColor(parent_->getResource( "FairQualityColor" ));
  _goodqualcol = parent_->allocColor(parent_->getResource( "GoodQualityColor" ));
  setfieldcolor( 1, parent_->getResource( "wirelessUsedColor" ) );

  priority_ = atoi(parent_->getResource( "wirelessPriority" ) );
  dodecay_ = parent_->isResourceTrue( "wirelessDecay" );
  SetUsedFormat(parent_->getResource( "wirelessUsedFormat" ) );
}

void WirelessMeter::checkevent( void ){
  getpwrinfo();

  drawfields();
}

void WirelessMeter::getpwrinfo( void ){
  std::ifstream loadinfo( WLFILENAME );
  if ( !loadinfo ){
    std::cerr << "Can not open file : " << WLFILENAME << std::endl;
    parent_->done(1);
    return;
  }

  char buff[16];
  int linkq = 0, quality = 0;
  bool link = false;

  // skip the two header rows
  loadinfo.ignore(1024, '\n');
  loadinfo.ignore(1024, '\n');

  if ( _devname.empty() ) {  // find devname on first run
    for (int i = 0; i < _number; i++)
      loadinfo.ignore(1024, '\n');
    if ( loadinfo.good() )
      loadinfo >> _devname >> buff >> linkq;
  }
  else {
    while ( !loadinfo.eof() ) {
      loadinfo >> buff;
      if ( _devname == buff ) {
        loadinfo >> buff >> linkq;
        link = true;
        break;
      }
      loadinfo.ignore(1024, '\n');
    }
  }

  if ( linkq >= 250 )
    linkq = 0;

  fields_[0] = linkq;
  if ( fields_[0] >= 15 )
    quality = 2;
  else if ( fields_[0] >= 7 )
    quality = 1;
  else
    quality = 0;

  if ( link && !_lastlink ) {
    legend("LINK/LEVEL");
    drawlegend();
    _lastlink = link;
  }
  else if ( !link && _lastlink ) {
    legend("NONE/LEVEL");
    drawlegend();
    _lastlink = link;
  }

  if ( quality != _lastquality ){
    if ( quality == 0 )
      setfieldcolor( 0, _poorqualcol );
    else if ( quality == 1 )
      setfieldcolor( 0, _fairqualcol );
    else
      setfieldcolor( 0, _goodqualcol );

    drawlegend();
    _lastquality = quality;
  }

  if (fields_[0] >= total_)
    total_ = 30 * (int)(fields_[0] / 30 + 1);
  fields_[1] = total_ - fields_[0];
  setUsed(fields_[0], total_);
}

int WirelessMeter::countdevices(void){
  glob_t gbuf;
  glob("/sys/class/net/*/wireless", 0, NULL, &gbuf);
  int count = gbuf.gl_pathc;
  globfree(&gbuf);
  return count;
}

const char *WirelessMeter::wirelessStr(int num) {
  static char buffer[8] = "WL";
  snprintf(buffer + 2, 5, "%d", num);
  buffer[7] = '\0';
  return buffer;
}
