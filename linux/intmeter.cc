//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "intmeter.h"
#include <stdlib.h>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <map>

static const char *INTFILE     = "/proc/interrupts";
static std::map<const int,int> realintnum;
static const int max = 1024;


IntMeter::IntMeter( XOSView *parent, int cpu )
  : BitMeter( parent, "INTS", "", 1, 0, 0 ), _cpu(cpu) {
  _irqs = _lastirqs = NULL;
  initirqcount();
}

IntMeter::~IntMeter( void ){
  if (_irqs)
    delete[] _irqs;
  if (_lastirqs)
    delete[] _lastirqs;
}

void IntMeter::checkevent( void ){
  getirqs();

  for ( int i = 0 ; i < numBits() ; i++ ){
    bits_[i] = ((_irqs[i] - _lastirqs[i]) != 0);
    _lastirqs[i] = _irqs[i];
  }

  BitMeter::checkevent();
}

void IntMeter::checkResources( void ){
  BitMeter::checkResources();
  onColor_  = parent_->allocColor( parent_->getResource( "intOnColor" ) );
  offColor_ = parent_->allocColor( parent_->getResource( "intOffColor" ) );
  priority_ = atoi( parent_->getResource( "intPriority" ) );
  _separate = parent_->isResourceTrue( "intSeparate" );
}

void IntMeter::getirqs( void ){
  std::ifstream intfile( INTFILE );
  std::string line;
  int intno, idx, i;
  unsigned long count, tmp;
  char *end = NULL;

  if ( !intfile ){
    std::cerr <<"Can not open file : " <<INTFILE << std::endl;
    exit( 1 );
  }

  intfile.ignore(max, '\n');

  while ( !intfile.eof() ){
    std::getline(intfile, line);
    if ( line.find_first_of("0123456789") > line.find_first_of(':') )
      break;  // reached non-numeric interrupts
    idx = strtoul(line.c_str(), &end, 10);
    if (idx >= max)
      break;
    intno = realintnum[idx];
    if ( intno >= numBits() )
      updateirqcount(intno, false);
    const char *cur = end + 1;
    count = tmp = i = 0;
    while (*cur && i++ <= _cpu) {
      tmp = strtoul(cur, &end, 10);
      count += tmp;
      cur = end;
    }
    _irqs[intno] = ( _separate ? tmp : count );
  }
}

/* The highest numbered interrupts, the number of interrupts
 * is going to be at least +1 (for int 0) and probably higher
 * if interrupts numbered more than this one just aren't active.
 * Must call with init = true the first time.
 */
void IntMeter::updateirqcount( int n, bool init ){
  int old_bits = numBits();
  setNumBits(n + 1);
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
  unsigned long *old_irqs = _irqs, *old_lastirqs = _lastirqs;
  _irqs = new unsigned long[n+1];
  _lastirqs = new unsigned long[n+1];
  /* If we are in init, set it to zero,
   * otherwise copy over the old set */
  if (init) {
    for (int i = 0; i < numBits(); i++)
      _irqs[i] = _lastirqs[i] = 0;
  }
  else {
    for (int i = 0; i < old_bits; i++) {
      _irqs[i] = old_irqs[i];
      _lastirqs[i] = old_lastirqs[i];
    }
    // zero to the end the irq's that haven't been seen before
    for (int i = old_bits; i < numBits(); i++)
      _irqs[i] = _lastirqs[i] = 0;
  }
  if (old_irqs)
    delete[] old_irqs;
  if (old_lastirqs)
    delete[] old_lastirqs;
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

  for (i = 0; i < 16; i++)
    realintnum[i] = i;

  intfile.ignore(max, '\n');

  /* just looking for the highest number interrupt that
   * is in use, ignore the rest of the data
   */
  idx = 16;
  while ( !intfile.eof() ){
    intfile >> i;
    /* break when reaching non-numeric special interrupts */
    if (!intfile)
      break;
    if (i < 16)
      intno = i;
    else {
      intno = idx;
      realintnum[i] = idx++;
    }
    intfile.ignore(max, '\n');
  }
  updateirqcount(intno, true);
}
