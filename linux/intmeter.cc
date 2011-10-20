//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//
// $Id$
//
#include "intmeter.h"
#include "xosview.h"
#include "cpumeter.h"
#include <fstream>
#include <sstream>
#include <map>
#include <stdlib.h>


static const char *INTFILE     = "/proc/interrupts";
static const char *VERSIONFILE = "/proc/version";

std::map<int,int> realintnum;

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
    std::ifstream vfile(VERSIONFILE);
    if (!vfile) {
      std::cerr << "Can not open file : " << VERSIONFILE << std::endl;
      exit(1);
    }

    std::string dump;
    float rval;
    vfile >> dump >> dump; // Drop the first two words
    vfile >> rval; // Drops everything but #.# (float regex)

    return rval;
}

int IntMeter::countCPUs(void) {
 return CPUMeter::countCPUs();
}

void IntMeter::getirqs( void ){
  std::ifstream intfile( INTFILE );
  int intno, count;
  int	idx;

  if ( !intfile ){
    std::cerr <<"Can not open file : " <<INTFILE << std::endl;
    exit( 1 );
  }

  if (!_old)
      intfile.ignore(1024, '\n');

  while ( !intfile.eof() ){
    intfile >> idx;
    intno = realintnum[idx];
    if(intno>=numBits())
    	updateirqcount(intno,false);
    if (!intfile) break;
    intfile.ignore(1);
    if ( !intfile.eof() ){
      for (int i = 0 ; i <= _cpu ; i++)
          intfile >>count;
      intfile.ignore(1024, '\n');

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
   std::ostringstream os;

   os << "INTs (0-15" ;
   for (std::map<int,int>::const_iterator it = realintnum.upper_bound(15),
                                          end = realintnum.end();
                                          it != end; ++it) {
         os << ", " << it->first ;
   }
   os << ")" << std::ends;

   legend(os.str().c_str());
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
  std::ifstream intfile( INTFILE );
  int intno = 0;
  int i, idx;

  if ( !intfile ){
    std::cerr <<"Can not open file : " <<INTFILE << std::endl;
    exit( 1 );
  }

  if (!_old) {
    for (i=0; i<16; i++)
      realintnum[i] = i;
    intfile.ignore(1024, '\n');
  }

  /* just looking for the highest number interrupt that
   * is in use, ignore the rest of the data
   */
  idx = 16;
  while ( !intfile.eof() ){
    intfile >> i;
    /* break when reaching non-numeric special interrupts */
    if (!intfile) break;
    if (i < 16)
	intno = i;
    else {
	intno = idx;
	realintnum[i] = idx++;
    }
    intfile.ignore(1024, '\n');
  }
  updateirqcount(intno, true);
}
