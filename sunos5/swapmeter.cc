//  
//  Initial port performed by Greg Onufer (exodus@cheers.bungi.com)
//
#include "swapmeter.h"
#include "xosview.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/swap.h>

static size_t Pagesize;

SwapMeter::SwapMeter(XOSView *parent, kstat_ctl_t *_kc)
	: FieldMeterGraph(parent, 3, "SWAP", "USED/RSVD/FREE")
{
	if (!Pagesize)
		Pagesize = sysconf(_SC_PAGESIZE);
}

SwapMeter::~SwapMeter(void)
{
}

void SwapMeter::checkResources(void)
{
	FieldMeterGraph::checkResources();

	setfieldcolor(0, parent_->getResource("swapUsedColor"));
	setfieldcolor(1, parent_->getResource("swapReservedColor"));
	setfieldcolor(2, parent_->getResource("swapFreeColor"));
	priority_ = atoi(parent_->getResource("swapPriority"));
	dodecay_ = parent_->isResourceTrue("swapDecay");
	useGraph_ = parent_->isResourceTrue("swapGraph");
	SetUsedFormat(parent_->getResource("swapUsedFormat"));
}

void SwapMeter::checkevent(void)
{
	getswapinfo();
	drawfields();
}

void SwapMeter::getswapinfo(void)
{
	struct anoninfo ai;

	if (swapctl(SC_AINFO, &ai) == -1)
		return;

	total_ = ai.ani_max;
	fields_[0] = (ai.ani_max - ai.ani_free); // allocated
	fields_[1] = (ai.ani_resv - (ai.ani_max - ai.ani_free)); // reserved
	fields_[2] = (ai.ani_max - ai.ani_resv); // available

	setUsed((fields_[0] + fields_[1]) * Pagesize, total_ * Pagesize);
}
