//  
//  Initial port performed by Greg Onufer (exodus@cheers.bungi.com)
//

#include "memmeter.h"
#include "kstats.h"
#include <unistd.h>
#include <stdlib.h>
#include <iostream>


MemMeter::MemMeter(XOSView *parent, kstat_ctl_t *_kc)
	: FieldMeterGraph(parent, 4, "MEM", "SYS/ZFS/OTHER/FREE")
{
	kc = _kc;
	pageSize = sysconf(_SC_PAGESIZE);
	total_ = sysconf(_SC_PHYS_PAGES);

	ksp_sp = kstat_lookup(kc, "unix", 0, "system_pages");
	ksp_zfs = kstat_lookup(kc, "vmem", -1, "zfs_file_data_buf");
	if (ksp_sp == NULL) { // ZFS cache may be missing
		std::cerr << "Can not find unix:0:system_pages kstat." << std::endl;
		parent_->done(1);
		return;
	}
}

void MemMeter::checkResources(void)
{
	FieldMeterGraph::checkResources();

	setfieldcolor(0, parent_->getResource("memKernelColor"));
	setfieldcolor(1, parent_->getResource("memCacheColor"));
	setfieldcolor(2, parent_->getResource("memUsedColor"));
	setfieldcolor(3, parent_->getResource("memFreeColor"));
	priority_ = atoi(parent_->getResource("memPriority"));
	dodecay_ = parent_->isResourceTrue("memDecay");
	useGraph_ = parent_->isResourceTrue("memGraph");
	SetUsedFormat(parent_->getResource("memUsedFormat"));
}

MemMeter::~MemMeter(void)
{
}

void MemMeter::checkevent(void)
{
	getmeminfo();
	drawfields();
}

void MemMeter::getmeminfo(void)
{
	kstat_named_t *k;

	fields_[1] = 0;
	if (ksp_zfs) {
		if (kstat_read(kc, ksp_zfs, NULL) == -1) {
			std::cerr << "Can not read vmem::zfs_file_data_buf kstat." << std::endl;
			parent_->done(1);
			return;
		}
		k = (kstat_named_t *)kstat_data_lookup(ksp_zfs, "mem_inuse");
		if (k == NULL) {
			std::cerr << "Can not read vmem::zfs_file_data_buf:mem_inuse kstat." << std::endl;
			parent_->done(1);
			return;
		}
		fields_[1] = kstat_to_double(k) / pageSize;
	}

	if (kstat_read(kc, ksp_sp, NULL) == -1) {
		std::cerr << "Can not read unix:0:system_pages kstat." << std::endl;
		parent_->done(1);
		return;
	}
	k = (kstat_named_t *)kstat_data_lookup(ksp_sp, "pp_kernel");
	if (k == NULL) {
		std::cerr << "Can not read unix:0:system_pages:pp_kernel kstat." << std::endl;
		parent_->done(1);
		return;
	}
	fields_[0] = kstat_to_double(k) - fields_[1];
	k = (kstat_named_t *)kstat_data_lookup(ksp_sp, "freemem");
	if (k == NULL) {
		std::cerr << "Can not read unix:0:system_pages:freemem kstat." << std::endl;
		parent_->done(1);
		return;
	}
	fields_[3] = kstat_to_double(k);
	fields_[2] = total_ - (fields_[0] + fields_[1] + fields_[3]);

	XOSDEBUG("kernel: %llu kB zfs: %llu kB other: %llu kB free: %llu kB\n",
	         (unsigned long long)(fields_[0] * pageSize / 1024),
	         (unsigned long long)(fields_[1] * pageSize / 1024),
	         (unsigned long long)(fields_[2] * pageSize / 1024),
	         (unsigned long long)(fields_[3] * pageSize / 1024));

	setUsed((fields_[0] + fields_[1] + fields_[2]) * pageSize, total_ * pageSize);
}

