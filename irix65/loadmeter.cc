
#include "loadmeter.h"
#include "xosview.h"
#include <stdlib.h>
#include <unistd.h>

#ifndef FSCALE
#define FSCALE	(1 << 8)
#endif

LoadMeter::LoadMeter(XOSView *parent)
	: FieldMeterGraph(parent, 2, "LOAD", "PROCS/MIN", 1, 1, 0)
{
    if (gethostname (hostname, 255) != 0) {
        perror ("gethostname");
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
		cerr << "Warning:  The loadmeter can not be configured as a decay\n"
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
    if (rstat (hostname, &res) != 0) {
        cerr << hostname <<endl;
        perror ("rstat");
        return;
    }

	fields_[0] = (float) res.avenrun[0]/FSCALE;
	
	if (fields_[0] > alarmThreshold) {
		if (total_ == alarmThreshold) {
			setfieldcolor(0, warnloadcol_);
			if (dolegends_) drawlegend();
		}
		total_ = fields_[1] = 20;
	} else {
		if (total_ == 20) {
			setfieldcolor(0, procloadcol_);
			if (dolegends_) drawlegend();
		}
		total_ = fields_[1] = alarmThreshold;
	}
	setUsed(fields_[0], total_);
}
