//  
//  Copyright (c) 1999 by Brian Grayson (bgrayson@netbsd.org)
//
//  This file may be distributed under terms of the GPL or of the BSD
//    license, whichever you choose.  The full license notices are
//    contained in the files COPYING.GPL and COPYING.BSD, which you
//    should have received.  If not, contact one of the xosview
//    authors for a copy.
//
// $Id$
//
#include <err.h>		//  For warnx.
#include <stdlib.h>	//  For atoi.
#include "general.h"
#include "intratemeter.h"
#include "xosview.h"

CVSID("$Id$");
CVSID_DOT_H(IRQRATEMETER_H_CVSID);

IrqRateMeter::IrqRateMeter( XOSView *parent )
  : FieldMeterGraph( parent, 2, "IRQs", "IRQs per sec/IDLE", 1, 1, 0 ){
  kernelHasStats_ = BSDIntrInit();

  if (!kernelHasStats_) {
  warnx(
  "!!! The kernel does not seem to have the symbols needed for the IrqRateMeter.");
  warnx(
  "!!! The IrqRateMeter has been disabled.");

    disableMeter();
  }
}

IrqRateMeter::~IrqRateMeter( void ){
}

void IrqRateMeter::checkResources( void ){
  FieldMeterGraph::checkResources();
  if (kernelHasStats_) {
    oncol_ = parent_->allocColor(parent_->getResource("irqrateUsedColor"));
    idlecol_ = parent_->allocColor(parent_->getResource("irqrateIdleColor"));
    setfieldcolor( 0, oncol_ );
    setfieldcolor( 1, idlecol_);
  }

  priority_ = atoi (parent_->getResource("irqratePriority"));
  dodecay_ = parent_->isResourceTrue("irqrateDecay");
  useGraph_ = parent_->isResourceTrue("irqrateGraph");
  SetUsedFormat (parent_->getResource("irqrateUsedFormat"));
  total_ = 2000;

  //  Now, grab a sample.  I don't know if this is needed here.  BCG
  BSDGetIntrStats (lastirqs_);
  BSDGetIntrStats (irqs_);
  getinfo();
}

void IrqRateMeter::checkevent( void ){
  getinfo();
  drawfields();
}

void IrqRateMeter::getinfo( void ){
  int i;

  IntervalTimerStop();
  BSDGetIntrStats (irqs_);
  delta = 0;
  for (i=0;i<NUM_INTR;i++) {
    delta += irqs_[i]-lastirqs_[i];
    lastirqs_[i] = irqs_[i];
  }
  /*  Scale delta by the priority.  */
  fields_[0] = delta / IntervalTimeInSecs();
  
  //  Bump total_, if needed.
  if (fields_[0] > total_) total_ = fields_[0];

  /*  I don't see why anyone would want to use any format besides
   *  float, but just in case.... */
  setUsed (fields_[0], total_);
  IntervalTimerStart();
}
