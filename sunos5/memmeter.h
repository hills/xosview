//  
//  Initial port performed by Greg Onufer (exodus@cheers.bungi.com)
//
#ifndef _MEMMETER_H_
#define _MEMMETER_H_

#include "fieldmetergraph.h"
#include "xosview.h"
#include <kstat.h>


class MemMeter : public FieldMeterGraph {
 public:
	MemMeter(XOSView *parent, kstat_ctl_t *kcp);
	~MemMeter(void);

	const char *name(void) const { return "MemMeter"; }
	void checkevent( void );
	void checkResources(void);

 protected:
	void getmeminfo( void );

 private:
	int pageSize;
	kstat_ctl_t *kc;
	kstat_t *ksp_sp, *ksp_zfs;
};


#endif
