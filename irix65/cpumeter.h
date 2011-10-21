//  
//  Initial port performed by Stefan Eilemann (eilemann@gmail.com)
//
#ifndef _CPUMETER_H_
#define _CPUMETER_H_

#include "fieldmetergraph.h"

#include <sys/types.h>
#include <sys/sysmp.h>
#include <sys/sysinfo.h>
    
#define USED_CPU_STATES (CPU_STATES-1) // SXBRK + IDLE merged

class CPUMeter : public FieldMeterGraph {
 public:
	CPUMeter(XOSView *parent, const int cpuid = 0);
	~CPUMeter(void);

	const char *name(void) const { return "CPUMeter"; }
	void checkevent(void);

	void checkResources(void);

	static int nCPUs();
	static const char *cpuStr(int num);

 protected:
	time_t cputime_[2][USED_CPU_STATES];
	int cpuindex_;

	void getcputime(void);
	const char *toUpper(const char *str);

 private:
    struct sysinfo	 tsp;
    int              sinfosz;
    int              cpuid_;
};

#endif
