//
//  Copyright (c) 1999, 2006 by Thomas Waldmann ( ThomasWaldmann@gmx.de )
//  based on work of Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "raidmeter.h"
#include "xosview.h"
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

RAIDMeter::RAIDMeter( XOSView *parent, int raiddev)
  : BitFieldMeter( parent, 1, 2, "RAID") {
  _raiddev = raiddev;
  getRAIDstate();
  if(disknum<1)
    disableMeter();
  std::ostringstream os;
  os << "MD" << raiddev << std::ends;
  legend(os.str().c_str());
  if(disknum>=1){
    setfieldlegend("Done/ToDo");
    setNumBits(disknum);
  }
  total_ = 100.0;
}

RAIDMeter::~RAIDMeter( void ){
}

void RAIDMeter::checkevent( void ){

  getRAIDstate();

  for ( int i = 0 ; i < disknum ; i++ ){
    bits_[i] = (working_map[i]=='+');
  }
  fields_[0]=100.0;
  sscanf(resync_state, "resync=%f", &fields_[0] );
  fields_[1] = total_ - fields_[1];
  if(fields_[0]<100.0){
    setfieldcolor(0,doneColor_);
    setfieldcolor(1,todoColor_);
  }else{
    setfieldcolor(0,completeColor_);
  }
  setUsed(fields_[0], total_);
  BitFieldMeter::checkevent();
}

void RAIDMeter::checkResources( void ){
  BitFieldMeter::checkResources();
  onColor_   = parent_->allocColor( parent_->getResource( "RAIDdiskOnlineColor" ) );
  offColor_  = parent_->allocColor( parent_->getResource( "RAIDdiskFailureColor" ) );
  doneColor_ = parent_->allocColor( parent_->getResource( "RAIDresyncdoneColor" ) );
  todoColor_ = parent_->allocColor( parent_->getResource( "RAIDresynctodoColor" ) );
  completeColor_= parent_->allocColor( parent_->getResource( "RAIDresynccompleteColor" ) );
  priority_  = atoi(parent_->getResource("RAIDPriority"));
  setfieldcolor( 0, doneColor_ );
  setfieldcolor( 1, todoColor_ );
  SetUsedFormat(parent_->getResource( "RAIDUsedFormat" ) );
}

// parser for /proc/mdstat

int RAIDMeter::find1(const char *key, const char *findwhat, int num1){
  char buf[80];
  int rc;
  std::ostringstream os;
  os << findwhat << "." << num1 << std::ends;
  strncpy(buf, os.str().c_str(), 80);
  buf[79] = '\0';
  rc=!strncmp(buf,key, 80);
  return rc;
}

int RAIDMeter::find2(const char *key, const char *findwhat,
  int num1, int num2){
  char buf[80];
  int rc;
  std::ostringstream os;
  os << findwhat << "." << num1 << "." << num2 << std::ends;
  strncpy(buf, os.str().c_str(), 80);
  buf[79] = '\0';
  rc=!strncmp(buf,key, 80);
  return rc;
}

static const char *RAIDFILE    = "/proc/mdstat";

int RAIDMeter::raidparse(char *cp){
  char *key, *val;
  key=strtok(cp," \n");
  val=strtok(NULL," \n");
  if(key==NULL) return 1;

  if(find1(key,"md_state",_raiddev)){
    if(val) strcpy(state,val);
  }else
  if(find1(key,"md_type",_raiddev)){
    if(val) strcpy(type,val);
  }else
  if(find1(key,"md_disk_count",_raiddev)){
    if(val) disknum=atoi(val);
  }else
  if(find1(key,"md_working_disk_map",_raiddev)){
    if(val) strcpy(working_map,val);
  }else
  if(find1(key,"md_resync_status",_raiddev)){
    if(val) strcpy(resync_state,val);
  }
  return 0;
}

void RAIDMeter::getRAIDstate( void ){
  std::ifstream raidfile( RAIDFILE );
  char l[256];

  if ( !raidfile ){
    std::cerr <<"Can not open file : " <<RAIDFILE << std::endl;
    exit( 1 );
  }

  do{
    raidfile.getline(l,256);
  }while((raidparse(l)==0) && (!raidfile.eof()));

//  printf("md0 %s %s %s resync: %s\n",type,state,working_map,resync_state);
}
