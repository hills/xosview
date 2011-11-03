//  
//  Initial port performed by Greg Onufer (exodus@cheers.bungi.com)
//
#include "loadmeter.h"
#include "xosview.h"
#include <stdlib.h>
#include <kstat.h>
/*#include <sys/loadavg.h>*/

using std::cerr;

LoadMeter::LoadMeter(XOSView *parent, kstat_ctl_t *_kc)
	: FieldMeterGraph(parent, 2, "LOAD", "PROCS/MIN", 1, 1, 0)
{
	kc = _kc;
	ksp = kstat_lookup(kc, "unix", 0, "system_misc");
	if (ksp == NULL) {
		parent_->done(1);
		return;
	}
}

LoadMeter::~LoadMeter(void)
{
}

void LoadMeter::checkResources(void)
{
	FieldMeterGraph::checkResources();

	warnloadcol_ = parent_->allocColor(
	    parent_->getResource("loadWarnColor" ));
	procloadcol_ = parent_->allocColor(
	    parent_->getResource("loadProcColor"));

	setfieldcolor(0, procloadcol_);
	setfieldcolor(1, parent_->getResource("loadIdleColor"));
	priority_ = atoi (parent_->getResource("loadPriority"));
	dodecay_ = parent_->isResourceTrue("loadDecay");
	useGraph_ = parent_->isResourceTrue("loadGraph");
	SetUsedFormat(parent_->getResource("loadUsedFormat"));

	alarmThreshold = atoi (parent_->getResource("loadAlarmThreshold"));

	if (dodecay_){
		/*
		 * Warning: Since the loadmeter changes scale
		 * occasionally, old decay values need to be rescaled.
		 * However, if they are rescaled, they could go off the
		 * edge of the screen.  Thus, for now, to prevent this
		 * whole problem, the load meter can not be a decay
		 * meter.  The load is a decaying average kind of thing
		 * anyway, so having a decaying load average is
		 * redundant.
		 */
		std::cerr << "Warning:  The loadmeter can not be configured as a decay\n"
		     << "  meter.  See the source code (" << __FILE__ << ") for further\n"
		     << "  details.\n";
		dodecay_ = 0;
	}
}

void LoadMeter::checkevent(void)
{
	getloadinfo();
	drawfields();
}

void LoadMeter::getloadinfo(void)
{
#if 1
	kstat_named_t *k;

	if (kstat_read(kc, ksp, NULL) == -1) {
		parent_->done(1);
		return;
	}
	k = (kstat_named_t *)kstat_data_lookup(ksp, "avenrun_1min");
	if (k == NULL) {
		parent_->done(1);
		return;
	}
	fields_[0] = (double)k->value.l / (1l << 8);
#else
	double oneMinLoad;

	getloadavg(&oneMinLoad, 1);
	fields_[0] = oneMinLoad;
#endif
	
	if (fields_[0] > alarmThreshold) {
		if (total_ == alarmThreshold) {
			setfieldcolor(0, warnloadcol_);
			drawlegend();
		}
		total_ = fields_[1] = 20;
	} else {
		if (total_ == 20) {
			setfieldcolor(0, procloadcol_);
			drawlegend();
		}
		total_ = fields_[1] = alarmThreshold;
	}
	setUsed(fields_[0], total_);
}
