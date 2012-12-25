//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "intmeter.h"
#include "xosview.h"
#include <fstream>
#include <sstream>
#include <map>
#include <stdlib.h>


static const char *INTFILE     = "/proc/interrupts";

std::map<int,int> realintnum;

IntMeter::IntMeter( XOSView *parent, int cpu)
  : BitMeter( parent, "INTS", "", 1, 0, 0 ), _cpu(cpu) {
  _old = ( CPUMeter::getkernelversion() <= 2000000 ? true : false );
  irqs_ = lastirqs_ = NULL;
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
  separate_ = parent_->isResourceTrue("intSeparate");
}

void IntMeter::getirqs( void ){
  std::ifstream intfile( INTFILE );
  int intno, count, tmp, idx;

  if ( !intfile ){
    std::cerr <<"Can not open file : " <<INTFILE << std::endl;
    exit( 1 );
  }

  if (!_old)
      intfile.ignore(1024, '\n');

  while ( !intfile.eof() ){
    count = tmp = 0;
    intfile >> idx;
    intno = realintnum[idx];
    if(intno>=numBits())
    	updateirqcount(intno,false);
    if (!intfile) break;
    intfile.ignore(1);
    if ( !intfile.eof() ){
      for (int i = 0; i <= _cpu; i++) {
        intfile >> tmp;
        count += tmp;
      }
      intfile.ignore(1024, '\n');

      irqs_[intno] = ( separate_ ? tmp : count );
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

  os << "0";
  if (realintnum.upper_bound(15) == realintnum.end()) // only 16 ints
    os << "-15";
  else {
    int prev = 15, prev2 = 14;
    for (std::map<int,int>::const_iterator it = realintnum.upper_bound(15),
                                           end = realintnum.end();
                                           it != end; ++it) {
      if ( &*it == &*realintnum.rbegin() ) { // last element
        if ( it->first == prev + 1 )
          os << "-" ;
        else {
          if ( prev == prev2 + 1 )
            os << "-" << prev;
          os << "," ;
        }
        os << it->first;
      }
      else {
        if ( it->first != prev + 1 ) {
          if ( prev == prev2 + 1 )
            os << "-" << prev;
          os << "," << it->first ;
        }
      }
      prev2 = prev;
      prev = it->first;
    }
    os << std::ends;
  }
   legend(os.str().c_str());
   unsigned long *old_irqs_=irqs_, *old_lastirqs_=lastirqs_;
   irqs_=new unsigned long[n+1];
   lastirqs_=new unsigned long[n+1];
   /* If we are in init, set it to zero,
    * otherwise copy over the old set */
   if( init ) {
	   for( int i=0; i < numBits(); i++)
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
