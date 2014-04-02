#ifndef _KStatList_H_
#define _KStatList_H_

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <kstat.h>
#include <vector>


// Helper to keep track of kstats.
class KStatList {
 public:
	enum module { CPU_STAT, CPU_INFO, DISKS, NETS };
	static KStatList *getList(kstat_ctl_t *kcp, module m) {
		switch (m) {
			case CPU_STAT:
				static KStatList cpu_stats(kcp, m);
				return &cpu_stats;
			case CPU_INFO:
				static KStatList cpu_infos(kcp, m);
				return &cpu_infos;
			case DISKS:
				static KStatList disks(kcp, m);
				return &disks;
			case NETS:
				static KStatList nets(kcp, m);
				return &nets;
			default:
				return NULL;
		}
	}
	kstat_t *operator[](size_t i) {
		return ( i < _stats.size() ? _stats[i] : NULL );
	}
	size_t count(void) { return _stats.size(); }
	void update(kstat_ctl_t *kcp) {
		if (kstat_chain_update(kcp) > 0 || _chain != kcp->kc_chain_id) {
			XOSDEBUG("kstat chain id changed to %d\n", kcp->kc_chain_id);
			_chain = kcp->kc_chain_id;
			_stats.clear();
			getstats(kcp);
		}
	}
 private:
	KStatList(const KStatList &);
	~KStatList(void) {}
	KStatList &operator=(const KStatList &);
	KStatList(kstat_ctl_t *kcp, module m) {
		_chain = kcp->kc_chain_id;
		_m = m;
		getstats(kcp);
	}
	void getstats(kstat_ctl_t *kcp) {
		for (kstat_t *ksp = kcp->kc_chain; ksp != NULL; ksp = ksp->ks_next) {
			if (_m == CPU_STAT && strncmp(ksp->ks_name, "cpu_stat", 8) == 0)
				_stats.push_back(ksp);
			if (_m == CPU_INFO && strncmp(ksp->ks_name, "cpu_info", 8) == 0)
				_stats.push_back(ksp);
			if (_m == DISKS && ksp->ks_type == KSTAT_TYPE_IO &&
			    strncmp(ksp->ks_class, "disk", 4) == 0)
				_stats.push_back(ksp);
			if (_m == NETS && ksp->ks_type == KSTAT_TYPE_NAMED &&
			    strncmp(ksp->ks_class, "net", 3) == 0 &&
			    ( strncmp(ksp->ks_module, "link", 4) == 0 ||
			      strncmp(ksp->ks_module, "lo", 2) == 0 ))
				_stats.push_back(ksp);
		}
	}
	kid_t _chain;
	module _m;
	std::vector<kstat_t *> _stats;
};


#endif

