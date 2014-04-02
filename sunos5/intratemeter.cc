//
//  Copyright (c) 2014 by Tomi Tapper <tomi.o.tapper@jyu.fi>
//
//  File based on bsd/intratemeter.* by
//  Copyright (c) 1999 by Brian Grayson (bgrayson@netbsd.org)
//
//  This file may be distributed under terms of the GPL
//
//

#include "intratemeter.h"
#include <stdlib.h>


IrqRateMeter::IrqRateMeter(XOSView *parent, kstat_ctl_t *kc)
	: FieldMeterGraph(parent, 2, "IRQs", "IRQs per sec/IDLE", 1, 1, 0)
{
	_lastirqcount = 0;
	_kc = kc;
	_cpus = KStatList::getList(_kc, KStatList::CPU_SYS);
}

IrqRateMeter::~IrqRateMeter(void)
{
}

void IrqRateMeter::checkResources(void)
{
	FieldMeterGraph::checkResources();
	setfieldcolor(0, parent_->getResource("irqrateUsedColor"));
	setfieldcolor(1, parent_->getResource("irqrateIdleColor"));
	priority_ = atoi(parent_->getResource("irqratePriority"));
	dodecay_ = parent_->isResourceTrue("irqrateDecay");
	useGraph_ = parent_->isResourceTrue("irqrateGraph");
	SetUsedFormat(parent_->getResource("irqrateUsedFormat"));
	total_ = 2000;
}

void IrqRateMeter::checkevent(void)
{
	getinfo();
	drawfields();
}

void IrqRateMeter::getinfo(void)
{
	kstat_named_t *k;
	uint64_t irqcount = 0;

	_cpus->update(_kc);
	IntervalTimerStop();
	for (size_t i = 0; i < _cpus->count(); i++) {
		if (kstat_read(_kc, (*_cpus)[i], NULL) == -1) {
			parent_->done(1);
			return;
		}
		k = (kstat_named_t *)kstat_data_lookup((*_cpus)[i], "intr");
		if (k == NULL) {
			parent_->done(1);
			return;
		}
		irqcount += kstat_to_ui64(k);
	}
	if (_lastirqcount == 0)
		_lastirqcount = irqcount;

	fields_[0] = (irqcount - _lastirqcount) / IntervalTimeInSecs();
	_lastirqcount = irqcount;
	IntervalTimerStart();

	if (fields_[0] > total_)
		total_ = fields_[0];

	setUsed(fields_[0], total_);
}

