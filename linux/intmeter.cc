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
 if (getLinuxVersion() <= 2.0)
 	_old = true;
 else
 	_old = false;
 irqs_=lastirqs_=0;
 initirqcount();
}

IntMeter::~IntMeter( void ){
   if(irqs_)
   	delete [] irqs_;
   if(lastirqs_)
   	delete [] lastirqs_;
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
    if(intno>=numBits())
    	updateirqcount(intno,false);
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

/* The highest numbered interrupts, the number of interrupts
 * is going to be at least +1 (for int 0) and probably higher
 * if interrupts numbered more than this one just aren't active.
 * Must call with init = true the first time.
 */
void IntMeter::updateirqcount( int n, bool init ){
   int old_bits=numBits();
   setNumBits(n+1);
   ostrstream os;
   os << "INTs (0-" << (n) << ")" << ends;
   legend(os.str());
   delete[] os.str();
   unsigned long *old_irqs_=irqs_, *old_lastirqs_=lastirqs_;
   irqs_=new unsigned long[n+1];
   lastirqs_=new unsigned long[n+1];
   /* If we are in init, set it to zero,
    * otherwise copy over the old set */
   if( init ) {
	   for( int i=0; i < n; i++)
		irqs_[i]=lastirqs_[i]=0;
   }
   else {
   	for( int i=0; i < old_bits; i++) {
	   irqs_[i]=old_irqs_[i];
	   lastirqs_[i]=old_lastirqs_[i];
	}
	// zero to the end the irq's that haven't been seen before
	for( int i=old_bits; i< numBits(); i++) {
	   irqs_[i]=lastirqs_[i]=0;
        }
   }
   if(old_irqs_)
   	delete [] old_irqs_;
   if(old_lastirqs_)
   	delete [] old_lastirqs_;
}

/* Find the highest number of interrupts and call updateirqcount to
 * update the number of interrupts listed
 */
void IntMeter::initirqcount( void ){
  ifstream intfile( INTFILE );
  int intno;

  if ( !intfile ){
    cerr <<"Can not open file : " <<INTFILE <<endl;
    exit( 1 );
  }

  if (!_old)
      intfile.istream::ignore(1024, '\n');

  /* just looking for the highest number interrupt that
   * is in use, ignore the rest of the data
   */
  while ( !intfile.eof() ){
    intfile >>intno;
    if (!intfile) break;
    intfile.istream::ignore(1024, '\n');
  }
  updateirqcount(intno, true);
}
