//  
//  Initial port performed by Greg Onufer (exodus@cheers.bungi.com)
//
#include "loadmeter.h"
#include "cpumeter.h"
#include "xosview.h"
#include <stdlib.h>
#include <string.h>
#include <kstat.h>
/*#include <sys/loadavg.h>*/


LoadMeter::LoadMeter(XOSView *parent, kstat_ctl_t *_kc)
	: FieldMeterGraph(parent, 2, "LOAD", "PROCS/MIN", 1, 1, 0)
{
	kc = _kc;
	ksp = kstat_lookup(kc, "unix", 0, "system_misc");
	if (ksp == NULL) {
		parent_->done(1);
		return;
	}
	total_ = -1;
	lastalarmstate = -1;
}

LoadMeter::~LoadMeter(void)
{
}

void LoadMeter::checkResources(void)
{
	FieldMeterGraph::checkResources();

	warnloadcol = parent_->allocColor(parent_->getResource("loadWarnColor"));
	procloadcol = parent_->allocColor(parent_->getResource("loadProcColor"));
	critloadcol = parent_->allocColor(parent_->getResource("loadCritColor"));

	setfieldcolor(0, procloadcol);
	setfieldcolor(1, parent_->getResource("loadIdleColor"));
	priority_ = atoi (parent_->getResource("loadPriority"));
	dodecay_ = parent_->isResourceTrue("loadDecay");
	useGraph_ = parent_->isResourceTrue("loadGraph");
	SetUsedFormat(parent_->getResource("loadUsedFormat"));

	const char *warn = parent_->getResource("loadWarnThreshold");
	if (strncmp(warn, "auto", 2) == 0)
		warnThreshold = CPUMeter::countCPUs(kc);
	else
		warnThreshold = atoi(warn);

	const char *crit = parent_->getResource("loadCritThreshold");
	if (strncmp(crit, "auto", 2) == 0)
		critThreshold = warnThreshold * 4;
	else
		critThreshold = atoi(crit);

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
	int alarmstate;
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
	
	if (fields_[0] <  warnThreshold)
		alarmstate = 0;
	else if (fields_[0] >= critThreshold)
		alarmstate = 2;
	else /* if fields_[0] >= warnThreshold */
		alarmstate = 1;

	if (alarmstate != lastalarmstate) {
		if (alarmstate == 0)
			setfieldcolor(0, procloadcol);
		else if (alarmstate == 1)
			setfieldcolor(0, warnloadcol);
		else /* if alarmstate == 2 */
			setfieldcolor(0, critloadcol);
		drawlegend();
		lastalarmstate = alarmstate;
	}

	// Adjust total to next power-of-two of the current load.
	if ( (fields_[0]*5.0 < total_ && total_ > 1.0) || fields_[0] > total_ ) {
		unsigned int i = fields_[0];
		i |= i >> 1; i |= i >> 2; i |= i >> 4; i |= i >> 8; i |= i >> 16;  // i = 2^n - 1
		total_ = i + 1;
	}

	fields_[1] = total_ - fields_[0];
	setUsed(fields_[0], total_);
}
