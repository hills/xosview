//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//
// $Id$
//
#include "intmeter.h"
#include "xosview.h"
#include "cpumeter.h"
#include <fstream.h>
#include <strstream.h>
#include <stdlib.h>
#ifdef __alpha__
#include <asm/irq.h>
#endif 


static const char *INTFILE     = "/proc/interrupts";
static const char *VERSIONFILE = "/proc/version";

IntMeter::IntMeter( XOSView *parent, int cpu)
  : BitMeter( parent, "INTS", "", 1, 
              0, 0 ), _cpu(cpu), _old(true) {
#ifdef __alpha__
char tmp[32];
#endif 
 if (getLinuxVersion() <= 2.0) {
   setNumBits(16);
   legend("INTs (0-15)");
 }
 else {
   _old = false;
#ifdef __alpha__
   setNumBits(NR_IRQS);
   sprintf(tmp,"INTs (0-%d)",(NR_IRQS-1));
   legend(tmp);
#else    
   setNumBits(24);
   legend("INTs (0-23)");
#endif
 }

  for ( int i = 0 ; i < numBits() ; i++ )
    irqs_[i] = lastirqs_[i] = 0;
}

IntMeter::~IntMeter( void ){
}

void IntMeter::checkevent( void ){
  getirqs();

  for ( int i = 0 ; i < numBits() ; i++ ){
    bits_[i] = ((irqs_[i] - lastirqs_[i]) != 0);
    lastirqs_[i] = irqs_[i];
  }

  BitMeter::checkevent();
}

void IntMeter::checkResources( void ){
  BitMeter::checkResources();
  onColor_  = parent_->allocColor( parent_->getResource( "intOnColor" ) );
  offColor_ = parent_->allocColor( parent_->getResource( "intOffColor" ) );
  priority_ = atoi(parent_->getResource("intPriority"));
}

float IntMeter::getLinuxVersion(void) {
    ifstream vfile(VERSIONFILE);
    if (!vfile) {
      cerr << "Can not open file : " << VERSIONFILE << endl;
      exit(1);
    }

    char buffer[128];
    vfile >> buffer >> buffer >> buffer;
    *strrchr(buffer, '.') = '\0';
    istrstream is(buffer, 128);
    float rval = 0.0;
    is >> rval;

    return rval;
}

int IntMeter::countCPUs(void) {
 return CPUMeter::countCPUs();
}

void IntMeter::getirqs( void ){
  ifstream intfile( INTFILE );
  int intno, count;

  if ( !intfile ){
    cerr <<"Can not open file : " <<INTFILE <<endl;
    exit( 1 );
  }

  if (!_old)
      intfile.istream::ignore(1024, '\n');

  while ( !intfile.eof() ){
    intfile >>intno;
    if (!intfile) break;
    intfile.ignore(1);
    if ( !intfile.eof() ){
      for (int i = 0 ; i <= _cpu ; i++)
          intfile >>count;
      intfile.istream::ignore(1024, '\n');
      
      irqs_[intno] = count;
    }
  }
}
