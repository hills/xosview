//
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include <stdlib.h>		/*  For atoi().  */
#include <sstream>
#include "intmeter.h"
#include "kernel.h"


IntMeter::IntMeter( XOSView *parent,
                    const char *, const char *, int dolegends,
                    int dousedlegends )
  : BitMeter( parent, "INTS", "IRQs", 1, dolegends, dousedlegends ) {
  irqcount_ = BSDNumInts();
  irqs_ = new unsigned long[irqcount_];
  lastirqs_ = new unsigned long[irqcount_];
  inbrs_ = new unsigned int[irqcount_];
  for ( unsigned int i = 0 ; i < irqcount_; i++ )
    irqs_[i] = lastirqs_[i] = inbrs_[i] = 0;
  if (!BSDIntrInit()) {
    disableMeter();
  }
  updateirqcount(true);
}

IntMeter::~IntMeter( void ){
  delete[] irqs_;
  delete[] lastirqs_;
  delete[] inbrs_;
}

void IntMeter::checkevent( void ){
  getirqs();

  for ( uint i = 0 ; i < irqcount_ ; i++ ){
    if (inbrs_[i] != 0) {
      if (realintnum.find(i) == realintnum.end()) {   // new interrupt number
        updateirqcount();
        return;
      }
      bits_[realintnum[i]] = ((irqs_[i] - lastirqs_[i]) != 0);
      lastirqs_[i] = irqs_[i];
    }
  }
  // This must be done in its own loop.
  for ( uint i = 0 ; i < irqcount_ ; i++ )
    inbrs_[i] = 0;

  BitMeter::checkevent();
}

void IntMeter::checkResources( void ){
  BitMeter::checkResources();
  if (!disabled_) {
    onColor_  = parent_->allocColor( parent_->getResource( "intOnColor" ) );
    offColor_ = parent_->allocColor( parent_->getResource( "intOffColor" ) );
    priority_ = atoi( parent_->getResource( "intPriority" ) );
  }
}

void IntMeter::getirqs( void ) {
  BSDGetIntrStats(irqs_, inbrs_);
}

void IntMeter::updateirqcount( bool init ) {
  int count = 16;

  if (init) {
    getirqs();
    for (int i = 0; i < 16; i++)
      realintnum[i] = i;
  }
  for (uint i = 16; i < irqcount_; i++) {
    if (inbrs_[i] != 0) {
      realintnum[i] = count++;
      inbrs_[i] = 0;
    }
  }
  setNumBits(count);
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
        else
          os << "," ;
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
}
