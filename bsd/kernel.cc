//
//  NetBSD port:
//  Copyright (c) 1995, 1996, 1997-2002 by Brian Grayson (bgrayson@netbsd.org)
//
//  This file was written by Brian Grayson for the NetBSD and xosview
//    projects.
//  This file contains code from the NetBSD project, which is covered
//    by the standard BSD license.
//  Dummy device ignore code by : David Cuka (dcuka@intgp1.ih.att.com)
//  The OpenBSD interrupt meter code was written by Oleg Safiullin
//    (form@vs.itam.nsc.ru).
//  This file may be distributed under terms of the GPL or of the BSD
//    license, whichever you choose.  The full license notices are
//    contained in the files COPYING.GPL and COPYING.BSD, which you
//    should have received.  If not, contact one of the xosview
//    authors for a copy.
//

#include "kernel.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <kvm.h>
#include <nlist.h>
#include <limits.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <sysexits.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <net/if.h>

#if defined(XOSVIEW_FREEBSD)
#include <net/if_var.h>
#endif

#if defined(XOSVIEW_NETBSD)
#include <sys/sched.h>
#include <sys/iostat.h>
#include <sys/envsys.h>
#include <prop/proplib.h>
#include <paths.h>
static int mib_cpt[2] = { CTL_KERN, KERN_CP_TIME };
static int mib_dsk[3] = { CTL_HW, HW_IOSTATS, sizeof(struct io_sysctl) };
#endif

#if defined(XOSVIEW_OPENBSD)
#include <sys/sched.h>
#include <sys/disk.h>
#include <sys/mount.h>
#include <net/route.h>
#include <net/if_dl.h>
static int mib_spd[2] = { CTL_HW, HW_CPUSPEED };
static int mib_cpt[2] = { CTL_KERN, KERN_CPTIME };
static int mib_ifl[6] = { CTL_NET, AF_ROUTE, 0, 0, NET_RT_IFLIST, 0 };
static int mib_bcs[3] = { CTL_VFS, VFS_GENERIC, VFS_BCACHESTAT };
#endif

#if defined(XOSVIEW_DFBSD)
#define _KERNEL_STRUCTURES
#include <net/if_var.h>
#include <kinfo.h>
#endif

#if defined(XOSVIEW_OPENBSD) || defined(XOSVIEW_DFBSD)
#include <sys/sensors.h>
static int mib_sen[5] = { CTL_HW, HW_SENSORS };
#endif

#if defined(HAVE_DEVSTAT)
#if defined(XOSVIEW_DFBSD)
#define CPUSTATES 5
#else
#include <sys/dkstat.h>
#endif
#include <devstat.h>
#endif

#if defined(HAVE_UVM)
#include <string.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <uvm/uvm_extern.h>
#ifdef VM_UVMEXP2
static int mib_uvm[2] = { CTL_VM, VM_UVMEXP2 };
#else
static int mib_uvm[2] = { CTL_VM, VM_UVMEXP };
#endif
#else
#include <sys/vmmeter.h>
#endif

#if defined(HAVE_SWAPCTL)
#include <sys/swap.h>
#endif


// ------------------------  local variables  ----------------------------------

//  This single kvm_t is shared by all of the kvm routines.
kvm_t* kd = NULL;

//  This struct has the list of all the symbols we want from the kernel.
static struct nlist nlst[] =
{
// We put a dummy symbol for a don't care, and ignore warnings about
// this later on.  This keeps the indices within the nlist constant.
#define DUMMY_SYM "dummy_sym"

#if defined(XOSVIEW_FREEBSD)
{ "_cnt" },
#define VMMETER_SYM_INDEX    0
#else
{ DUMMY_SYM },
#define DUMMY_0
#endif
#if !defined(XOSVIEW_OPENBSD)
{ "_ifnet" },
#define IFNET_SYM_INDEX      1
#else
{ DUMMY_SYM },
#define DUMMY_1
#endif

#if defined(XOSVIEW_OPENBSD)
{ "_disklist" },
#define DISKLIST_SYM_INDEX   2
#else
{ DUMMY_SYM },
#define DUMMY_2
#endif
#if defined(XOSVIEW_NETBSD)
{ "_allevents" },
#define ALLEVENTS_SYM_INDEX  3
{ "_bufmem" },
#define BUFMEM_SYM_INDEX     4
#else
{ DUMMY_SYM },
#define DUMMY_3
{ DUMMY_SYM },
#define DUMMY_4
#endif
#if defined(XOSVIEW_FREEBSD)
{ "_intrnames" },
#define INTRNAMES_SYM_INDEX  5
# if __FreeBSD_version >= 900040
{ "_sintrnames" },
# else
{ "_eintrnames" },
# endif
#define EINTRNAMES_SYM_INDEX 6
{ "_intrcnt" },
#define INTRCNT_SYM_INDEX    7
# if __FreeBSD_version >= 900040
{ "_sintrcnt" },
# else
{ "_eintrcnt" },
# endif
#define EINTRCNT_SYM_INDEX   8
#endif
{ NULL }
};

static char kernelFileName[_POSIX2_LINE_MAX];


// ------------------------  utility functions  --------------------------------
//  The following is an error-checking form of kvm_read.  In addition
//  it uses kd as the implicit kernel-file to read.  Saves typing.
//  Since this is C++, it's an inline function rather than a macro.

static inline void
safe_kvm_read(u_long kernel_addr, void* user_addr, size_t nbytes) {
	/*  Check for obvious bad symbols (i.e., from /netbsd when we
	 *  booted off of /netbsd.old), such as symbols that reference
	 *  0x00000000 (or anywhere in the first 256 bytes of memory).  */
	int retval = 0;
	if ( (kernel_addr & 0xffffff00) == 0 )
		errx(EX_SOFTWARE, "safe_kvm_read() was attempted on EA %#lx.", kernel_addr);
	if ( (retval = kvm_read(kd, kernel_addr, user_addr, nbytes)) == -1 )
		err(EX_SOFTWARE, "kvm_read() of kernel address %#lx", kernel_addr);
	if (retval != (int)nbytes)
		warn("safe_kvm_read(%#lx) returned %d bytes, not %d", kernel_addr, retval, (int)nbytes);
}

//  This version uses the symbol offset in the nlst variable, to make it
//  a little more convenient.  BCG
static inline void
safe_kvm_read_symbol(int nlstOffset, void* user_addr, size_t nbytes) {
	safe_kvm_read(nlst[nlstOffset].n_value, user_addr, nbytes);
}

int
ValidSymbol(int index) {
	return ( (nlst[index].n_value & 0xffffff00) != 0 );
}

int
SymbolValue(int index) {
	return nlst[index].n_value;
}

void
BSDInit() {
	kernelFileName[0] = '\0';
}

void
SetKernelName(const char* kernelName) {
	if (strlen(kernelName) >= _POSIX2_LINE_MAX)
		errx(EX_OSFILE, "Kernel file name of '%s' is too long.", kernelName);

	strncpy(kernelFileName, kernelName, _POSIX2_LINE_MAX);
}

void
OpenKDIfNeeded() {
	char errstring[_POSIX2_LINE_MAX];

	if (kd)
		return; //  kd is non-NULL, so it has been initialized.  BCG

	/*  Open it read-only, for a little added safety.  */
	/*  If the first character of kernelFileName is not '\0', then use
	 *  that kernel file.  Otherwise, use the default kernel, by
	 *  specifying NULL.  */
	if ((kd = kvm_openfiles((kernelFileName[0] ? kernelFileName : NULL),
		                    NULL, NULL, O_RDONLY, errstring)) == NULL)
		err(EX_OSFILE, "OpenKDIfNeeded(): %s", errstring);

	// Parenthetical note:  FreeBSD kvm_openfiles() uses getbootfile() to get
	// the correct kernel file if the 1st arg is NULL.  As far as I can see,
	// one should always use NULL in FreeBSD, but I suppose control is never a
	// bad thing... (pavel 21-Jan-1998)

	/*  Now grab the symbol offsets for the symbols that we want.  */
	if (kvm_nlist(kd, nlst) < 0)
		err(EX_OSERR, "Could not get kvm symbols");

	//  Look at all of the returned symbols, and check for bad lookups.
	//  (This may be unnecessary, but better to check than not to...  )
	struct nlist *nlp = nlst;
	while (nlp && nlp->n_name && strncmp(nlp->n_name, DUMMY_SYM, strlen(DUMMY_SYM))) {
		if ( nlp->n_type == 0 || nlp->n_value == 0 )
#if defined(XOSVIEW_FREEBSD) && defined(__alpha__)
			/* XXX: this should be properly fixed. */
			;
#else
			warnx("kvm_nlist() lookup failed for symbol '%s'.", nlp->n_name);
#endif
		nlp++;
	}
}

int
BSDGetCPUSpeed() {
	size_t size;
	int cpu_speed = 0;

#if defined(XOSVIEW_FREEBSD)
	char name[25];
	int speed = 0, cpus = BSDCountCpus(), avail_cpus = 0;
	size = sizeof(speed);
	for (int i = 0; i < cpus; i++) {
		snprintf(name, 25, "dev.cpu.%d.freq", i);
		if ( sysctlbyname(name, &speed, &size, NULL, 0) == 0 ) {
			// count only cpus with individual freq available
			cpu_speed += speed;
			avail_cpus++;
		}
	}
	if (avail_cpus > 1)
		cpu_speed /= avail_cpus;
#elif defined(XOSVIEW_OPENBSD)
	size = sizeof(cpu_speed);
	if ( sysctl(mib_spd, 2, &cpu_speed, &size, NULL, 0) < 0 )
		err(EX_OSERR, "syscl hw.cpuspeed failed");
#else  /* XOSVIEW_NETBSD || XOSVIEW_DFBSD */
	long speed = 0;
	size = sizeof(speed);
#if defined(XOSVIEW_NETBSD)
	if ( sysctlbyname("machdep.tsc_freq", &speed, &size, NULL, 0) < 0 )
		err(EX_OSERR, "sysctl machdep.tsc_freq failed");
#else  /* XOSVIEW_DFBSD */
	if ( sysctlbyname("hw.tsc_frequency", &speed, &size, NULL, 0) < 0 )
		err(EX_OSERR, "sysctl hw.tsc_frequency failed");
#endif
	cpu_speed = speed / 1000000;
#endif
	return cpu_speed;
}


// --------------------  PageMeter & MemMeter functions  -----------------------
void
BSDPageInit() {
	OpenKDIfNeeded();
}

/* meminfo[5]  = { active, inactive, wired, cached, free } */
/* pageinfo[2] = { pages_in, pages_out }                   */
void
BSDGetPageStats(u_int64_t *meminfo, u_int64_t *pageinfo) {
#if defined(HAVE_UVM)
#ifdef VM_UVMEXP2
	struct uvmexp_sysctl uvm;
#else
	struct uvmexp uvm;
#endif
	size_t size = sizeof(uvm);
	if ( sysctl(mib_uvm, 2, &uvm, &size, NULL, 0) < 0 )
		err(EX_OSERR, "sysctl vm.uvmexp failed");

	if (meminfo) {
		// UVM excludes kernel memory -> assume it is active mem
		meminfo[0] = (u_int64_t)(uvm.npages - uvm.inactive - uvm.wired - uvm.free) * uvm.pagesize;
		meminfo[1] = (u_int64_t)uvm.inactive * uvm.pagesize;
		meminfo[2] = (u_int64_t)uvm.wired * uvm.pagesize;

		// cache is already included in active and inactive memory and
		// there's no way to know how much is in which -> disable cache
		meminfo[3] = 0;
		meminfo[4] = (u_int64_t)uvm.free * uvm.pagesize;
	}
	if (pageinfo) {
		pageinfo[0] = (u_int64_t)uvm.pgswapin;
		pageinfo[1] = (u_int64_t)uvm.pgswapout;
	}
#else  /* HAVE_UVM */
	struct vmmeter vm;
#if defined(XOSVIEW_FREEBSD)
	safe_kvm_read_symbol(VMMETER_SYM_INDEX, &vm, sizeof(vm));
#else  /* XOSVIEW_DFBSD */
	struct vmstats vms;
	size_t size = sizeof(vms);
	if ( sysctlbyname("vm.vmstats", &vms, &size, NULL, 0) < 0 )
		err(EX_OSERR, "sysctl vm.vmstats failed");
	size = sizeof(vm);
	if ( sysctlbyname("vm.vmmeter", &vm, &size, NULL, 0) < 0 )
		err(EX_OSERR, "sysctl vm.vmmeter failed");
#endif
	if (meminfo) {
#if defined(XOSVIEW_FREEBSD)
		meminfo[0] = (u_int64_t)vm.v_active_count * vm.v_page_size;
		meminfo[1] = (u_int64_t)vm.v_inactive_count * vm.v_page_size;
		meminfo[2] = (u_int64_t)vm.v_wire_count * vm.v_page_size;
		meminfo[3] = (u_int64_t)vm.v_cache_count * vm.v_page_size;
		meminfo[4] = (u_int64_t)vm.v_free_count * vm.v_page_size;
#else  /* XOSVIEW_DFBSD */
		meminfo[0] = (u_int64_t)vms.v_active_count * vms.v_page_size;
		meminfo[1] = (u_int64_t)vms.v_inactive_count * vms.v_page_size;
		meminfo[2] = (u_int64_t)vms.v_wire_count * vms.v_page_size;
		meminfo[3] = (u_int64_t)vms.v_cache_count * vms.v_page_size;
		meminfo[4] = (u_int64_t)vms.v_free_count * vms.v_page_size;
#endif
	}
	if (pageinfo) {
		pageinfo[0] = (u_int64_t)vm.v_vnodepgsin + (u_int64_t)vm.v_swappgsin;
		pageinfo[1] = (u_int64_t)vm.v_vnodepgsout + (u_int64_t)vm.v_swappgsout;
	}
#endif
}


// ------------------------  CPUMeter functions  -------------------------------

void
BSDCPUInit() {
	OpenKDIfNeeded();
}

void
#if defined(XOSVIEW_NETBSD) || defined(XOSVIEW_DFBSD)
BSDGetCPUTimes(u_int64_t *timeArray) {
#else
BSDGetCPUTimes(long *timeArray) {
#endif
#if defined(XOSVIEW_DFBSD)
	struct kinfo_cputime cpu;
#elif defined(XOSVIEW_NETBSD)
	uint64_t cpu[CPUSTATES];
#else
	long cpu[CPUSTATES];
#endif
	if (!timeArray)
		errx(EX_SOFTWARE, "BSDGetCPUTimes(): passed pointer was null.");
	if (CPUSTATES != 5)
		errx(EX_SOFTWARE, "xosview for *BSD expects 5 cpu states.");

#if defined(XOSVIEW_DFBSD)
	if (kinfo_get_sched_cputime(&cpu))
		err(EX_OSERR, "kinfo_get_sched_cputime() failed");
	timeArray[0] = cpu.cp_user;
	timeArray[1] = cpu.cp_nice;
	timeArray[2] = cpu.cp_sys;
	timeArray[3] = cpu.cp_intr;
	timeArray[4] = cpu.cp_idle;
#else
	size_t size = sizeof(cpu);
#if defined(XOSVIEW_NETBSD) || defined(XOSVIEW_OPENBSD)
	if ( sysctl(mib_cpt, 2, cpu, &size, NULL, 0) < 0 )
#else
	if ( sysctlbyname("kern.cp_time", cpu, &size, NULL, 0) < 0 )
#endif
		err(EX_OSERR, "sysctl kern.cp_time failed");
	for (int i = 0; i < CPUSTATES; i++)
		timeArray[i] = cpu[i];
#endif
}


// ------------------------  NetMeter functions  -------------------------------
int
BSDNetInit() {
	OpenKDIfNeeded();
#if defined(XOSVIEW_NETBSD)
	return ValidSymbol(IFNET_SYM_INDEX);
#else
	return 1;
#endif
}

void
BSDGetNetInOut(unsigned long long *inbytes, unsigned long long *outbytes, const char *netIface, bool ignored) {
	char ifname[IFNAMSIZ];
	*inbytes = 0;
	*outbytes = 0;
#if defined(XOSVIEW_OPENBSD)
	size_t size;
	char *buf, *next;
	struct if_msghdr *ifm;
	struct if_data ifd;
	struct sockaddr_dl *sdl;

	if ( sysctl(mib_ifl, 6, NULL, &size, NULL, 0) < 0 )
		err(EX_OSERR, "BSDGetNetInOut(): sysctl 1 failed");
	if ( (buf = (char *)malloc(size)) == NULL)
		err(EX_OSERR, "BSDGetNetInOut(): malloc failed");
	if ( sysctl(mib_ifl, 6, buf, &size, NULL, 0) < 0 )
		err(EX_OSERR, "BSDGetNetInOut(): sysctl 2 failed");

	for (next = buf; next < buf + size; next += ifm->ifm_msglen) {
		bool skipif = false;
		ifm = (struct if_msghdr *)next;
		if (ifm->ifm_type != RTM_IFINFO || ifm->ifm_addrs & RTAX_IFP == 0)
			continue;
		ifd = ifm->ifm_data;
		sdl = (struct sockaddr_dl *)(ifm + 1);
		if (sdl->sdl_family != AF_LINK)
			continue;
		if ( strncmp(netIface, "False", 5) != 0 ) {
			memcpy(ifname, sdl->sdl_data, (sdl->sdl_nlen >= IFNAMSIZ ? IFNAMSIZ - 1 : sdl->sdl_nlen));
			if ( (!ignored && strncmp(sdl->sdl_data, netIface, sdl->sdl_nlen) != 0) ||
				 ( ignored && strncmp(sdl->sdl_data, netIface, sdl->sdl_nlen) == 0) )
				skipif = true;
		}
		if (!skipif) {
			*inbytes += ifd.ifi_ibytes;
			*outbytes += ifd.ifi_obytes;
		}
	}
	free(buf);
#else  /* XOSVIEW_OPENBSD */
	struct ifnet *ifnetp;
	struct ifnet ifnet;
#if defined (XOSVIEW_NETBSD)
	struct ifnet_head ifnethd;
#else
	struct ifnethead ifnethd;
#endif
	safe_kvm_read(nlst[IFNET_SYM_INDEX].n_value, &ifnethd, sizeof(ifnethd));
	ifnetp = TAILQ_FIRST(&ifnethd);

	while (ifnetp) {
		bool skipif = false;
		//  Now, dereference the pointer to get the ifnet struct.
		safe_kvm_read((u_long)ifnetp, &ifnet, sizeof(ifnet));
		strlcpy(ifname, ifnet.if_xname, sizeof(ifname));
#if defined(XOSVIEW_NETBSD)
		ifnetp = TAILQ_NEXT(&ifnet, if_list);
#else
		ifnetp = TAILQ_NEXT(&ifnet, if_link);
#endif
		if (!(ifnet.if_flags & IFF_UP))
			continue;
		if ( strncmp(netIface, "False", 5) != 0 ) {
			if ( (!ignored && strncmp(ifname, netIface, 256) != 0) ||
			     ( ignored && strncmp(ifname, netIface, 256) == 0) )
				skipif = true;
		}
		if (!skipif) {
			*inbytes  += ifnet.if_ibytes;
			*outbytes += ifnet.if_obytes;
		}
	}
#endif  /* XOSVIEW_OPENBSD */
}


//  ---------------------- Swap Meter stuff  -----------------------------------

int
BSDSwapInit() {
	OpenKDIfNeeded();
	return 1;
}

void
BSDGetSwapInfo(u_int64_t *total, u_int64_t *used) {
#if defined(HAVE_SWAPCTL)
	//  This code is based on a patch sent in by Scott Stevens
	//  (s.k.stevens@ic.ac.uk, at the time).
	struct swapent *sep, *swapiter;
	int bsize, rnswap, nswap = swapctl(SWAP_NSWAP, 0, 0);
	*total = *used = 0;

	if (nswap < 1)  // no swap devices on
		return;

	if ( (sep = (struct swapent *)malloc(nswap* sizeof(struct swapent))) == NULL )
		err(EX_OSERR, "BSDGetSwapInfo(): malloc failed");
	rnswap = swapctl(SWAP_STATS, (void *)sep, nswap);
	if (rnswap < 0)
		err(EX_OSERR, "BSDGetSwapInfo(): getting SWAP_STATS failed");
	if (nswap != rnswap)
		warnx("SWAP_STATS gave different value than SWAP_NSWAP "
		      "(nswap=%d versus rnswap=%d).", nswap, rnswap);

	swapiter = sep;
	bsize = 512;  // block size is that of underlying device, *usually* 512 bytes
	for ( ; rnswap-- > 0; swapiter++) {
		*total += (u_int64_t)swapiter->se_nblks * bsize;
		*used += (u_int64_t)swapiter->se_inuse * bsize;
	}
	free(sep);
#else
	struct kvm_swap kswap;
	OpenKDIfNeeded();
	int pgsize = getpagesize();
	if ( kvm_getswapinfo(kd, &kswap, 1, 0) )
		err(EX_OSERR, "BSDGetSwapInfo(): kvm_getswapinfo failed");

	*total = (u_int64_t)kswap.ksw_total * pgsize;
	*used = (u_int64_t)kswap.ksw_used * pgsize;
#endif
}


// ----------------------- Disk Meter stuff  -----------------------------------

#ifdef HAVE_DEVSTAT
/*
 * Make use of the new FreeBSD kernel device statistics library using
 * code shamelessly borrowed from xsysinfo, which borrowed shamelessly
 * from FreeBSD's iostat(8).
 */
long generation;
devstat_select_mode select_mode;
struct devstat_match *matches;
int num_matches = 0;
int num_selected, num_selections;
long select_generation;
static struct statinfo cur, last;
int num_devices;
struct device_selection *dev_select;
int nodisk = 0;

void
DevStat_Init(void) {
	/*
	 * Make sure that the userland devstat version matches the kernel
	 * devstat version.
	 */
#if defined(XOSVIEW_FREEBSD)
	if (devstat_checkversion(NULL) < 0) {
#else
	if (checkversion() < 0) {
#endif
		nodisk++;
		warn("%s\n", devstat_errbuf);
		return;
	}

	/* find out how many devices we have */
#if defined(XOSVIEW_FREEBSD)
	if ( (num_devices = devstat_getnumdevs(NULL)) < 0 ) {
#else
	if ( (num_devices = getnumdevs()) < 0 ) {
#endif
		nodisk++;
		warn("%s\n", devstat_errbuf);
		return;
	}

	cur.dinfo = (struct devinfo *)calloc(1, sizeof(struct devinfo));
	last.dinfo = (struct devinfo *)calloc(1, sizeof(struct devinfo));

	/*
	 * Grab all the devices.  We don't look to see if the list has
	 * changed here, since it almost certainly has.  We only look for
	 * errors.
	 */
#if defined(XOSVIEW_FREEBSD)
	if (devstat_getdevs(NULL, &cur) == -1) {
#else
	if (getdevs(&cur) == -1) {
#endif
		nodisk++;
		warn("%s\n", devstat_errbuf);
		return;
	}

	num_devices = cur.dinfo->numdevs;
	generation = cur.dinfo->generation;
	dev_select = NULL;

	/* only interested in disks */
	matches = NULL;
	char da[3] = "da";
#if defined(XOSVIEW_FREEBSD)
	if (devstat_buildmatch(da, &matches, &num_matches) != 0) {
#else
	if (buildmatch(da, &matches, &num_matches) != 0) {
#endif
		nodisk++;
		warn("%s\n", devstat_errbuf);
		return;
	}

	if (num_matches == 0)
		select_mode = DS_SELECT_ADD;
	else
		select_mode = DS_SELECT_ONLY;

	/*
	 * At this point, selectdevs will almost surely indicate that the
	 * device list has changed, so we don't look for return values of 0
	 * or 1.  If we get back -1, though, there is an error.
	 */
#if defined(XOSVIEW_FREEBSD)
	if (devstat_selectdevs(&dev_select, &num_selected,
#else
	if (selectdevs(&dev_select, &num_selected,
#endif
	               &num_selections, &select_generation,
	               generation, cur.dinfo->devices, num_devices,
	               matches, num_matches, NULL, 0, select_mode, 10, 0) == -1) {
		nodisk++;
		warn("%s\n", devstat_errbuf);
	}
}

u_int64_t
DevStat_Get(u_int64_t *read_bytes, u_int64_t *write_bytes) {
	register int dn;
	long double busy_seconds;
	u_int64_t reads, writes, total_bytes = 0;
	struct devinfo *tmp_dinfo;

	if (nodisk > 0)
		/* Diskless system or some error happened. */
		return 0;

	/*
	 * Here what we want to do is refresh our device stats.
	 * getdevs() returns 1 when the device list has changed.
	 * If the device list has changed, we want to go through
	 * the selection process again, in case a device that we
	 * were previously displaying has gone away.
	 */
#if defined(XOSVIEW_FREEBSD)
	switch (devstat_getdevs(NULL, &cur)) {
#else
	switch (getdevs(&cur)) {
#endif
	case -1:
		return (0);
	case 1:
		int retval;
		num_devices = cur.dinfo->numdevs;
		generation = cur.dinfo->generation;
#if defined(XOSVIEW_FREEBSD)
		retval = devstat_selectdevs(&dev_select, &num_selected,
#else
		retval = selectdevs(&dev_select, &num_selected,
#endif
		                    &num_selections, &select_generation,
		                    generation, cur.dinfo->devices,
		                    num_devices, matches, num_matches,
		                    NULL, 0, select_mode, 10, 0);
		switch(retval) {
		case -1:
			return (0);
		case 1:
			break;
		default:
			break;
		break;
		}
	default:
		break;
	}

	/*
	 * Calculate elapsed time up front, since it's the same for all
	 * devices.
	 */
#if defined(XOSVIEW_FREEBSD)
	busy_seconds = cur.snap_time - last.snap_time;
#else
	busy_seconds = compute_etime(cur.busy_time, last.busy_time);
#endif
	/* this is the first time thru so just copy cur to last */
	if (last.dinfo->numdevs == 0) {
		tmp_dinfo = last.dinfo;
		last.dinfo = cur.dinfo;
		cur.dinfo = tmp_dinfo;
#if defined(XOSVIEW_FREEBSD)
		last.snap_time = cur.snap_time;
#else
		last.busy_time = cur.busy_time;
#endif
		return (0);
	}

	for (dn = 0; dn < num_devices; dn++) {
		int di;
		if ( (dev_select[dn].selected == 0) || (dev_select[dn].selected > 10) )
			continue;

		di = dev_select[dn].position;
#if defined(XOSVIEW_FREEBSD)
		if (devstat_compute_statistics(&cur.dinfo->devices[di],
		                               &last.dinfo->devices[di], busy_seconds,
		                               DSM_TOTAL_BYTES_READ, &reads,
		                               DSM_TOTAL_BYTES_WRITE, &writes,
		                               DSM_NONE) != 0) {
#else
		if (compute_stats_read(&cur.dinfo->devices[di],
		                       &last.dinfo->devices[di], busy_seconds,
		                       &reads, NULL,
		                       NULL, NULL, NULL, NULL, NULL, NULL) != 0) {
			warn("%s\n", devstat_errbuf);
			break;
		}
		if (compute_stats_write(&cur.dinfo->devices[di],
		                        &last.dinfo->devices[di], busy_seconds,
		                        &writes, NULL,
		                        NULL, NULL, NULL, NULL, NULL, NULL) != 0) {
#endif
			warn("%s\n", devstat_errbuf);
			break;
		}
		*read_bytes += reads;
		*write_bytes += writes;
		total_bytes += reads + writes;
	}

	tmp_dinfo = last.dinfo;
	last.dinfo = cur.dinfo;
	cur.dinfo = tmp_dinfo;
#if defined(XOSVIEW_FREEBSD)
	last.snap_time = cur.snap_time;
#else
	last.busy_time = cur.busy_time;
#endif

	return total_bytes;
}
#endif

int
BSDDiskInit() {
	OpenKDIfNeeded();
#if defined(HAVE_DEVSTAT)
	DevStat_Init();
#endif
	return 1;
}

u_int64_t
BSDGetDiskXFerBytes(u_int64_t *read_bytes, u_int64_t *write_bytes) {
#if defined(HAVE_DEVSTAT)
	return DevStat_Get(read_bytes, write_bytes);
#else
	*read_bytes = *write_bytes = 0;
# if defined(XOSVIEW_NETBSD)
	size_t size;
	// Do a sysctl with a NULL data pointer to get the size that would
	// have been returned, and use that to figure out # drives.
	if ( sysctl(mib_dsk, 3, NULL, &size, NULL, 0) < 0 )
		err(EX_OSERR, "BSDGetDiskXFerBytes(): sysctl hw.iostats #1 failed");
	unsigned int ndrives = size / mib_dsk[2];
	struct io_sysctl drive_stats[ndrives];

	// Get the stats.
	if ( sysctl(mib_dsk, 3, drive_stats, &size, NULL, 0) < 0 )
		err(EX_OSERR, "BSDGetDiskXFerBytes(): sysctl hw.iostats #2 failed");

	// Now accumulate the total.
	for (uint i = 0; i < ndrives; i++) {
		*read_bytes += drive_stats[i].rbytes;
		*write_bytes += drive_stats[i].wbytes;
	}
# else  /* XOSVIEW_OPENBSD */
  /*  This function is a little tricky -- we have to iterate over a
   *  list in kernel land.  To make things simpler, data structures
   *  and pointers for objects in kernel-land have kvm tacked on front
   *  of their names.  Thus, kvmdiskptr points to a disk struct in
   *  kernel memory.  kvmcurrdisk is a copy of the kernel's struct,
   *  and it has pointers in it to other structs, so it also is
   *  prefixed with kvm.  */
	struct disklist_head kvmdisklist;
	struct disk *kvmdiskptr;
	struct disk kvmcurrdisk;
	safe_kvm_read_symbol(DISKLIST_SYM_INDEX, &kvmdisklist, sizeof(kvmdisklist));
	kvmdiskptr = TAILQ_FIRST(&kvmdisklist);
	while (kvmdiskptr != NULL) {
		safe_kvm_read((u_long)kvmdiskptr, &kvmcurrdisk, sizeof(kvmcurrdisk));
		*read_bytes += kvmcurrdisk.dk_rbytes;
		*write_bytes += kvmcurrdisk.dk_wbytes;
		kvmdiskptr = TAILQ_NEXT(&kvmcurrdisk, dk_link);
	}
# endif
#endif
	return (*read_bytes + *write_bytes);
}


//  ---------------------- Interrupt Meter stuff  ------------------------------

int
BSDIntrInit() {
	OpenKDIfNeeded();
	// Make sure the intr counter array is nonzero in size.
#if defined(XOSVIEW_FREEBSD)
# if __FreeBSD_version >= 900040
	int nintr;
	safe_kvm_read(nlst[EINTRCNT_SYM_INDEX].n_value, &nintr, sizeof(nintr));
	return ValidSymbol(INTRCNT_SYM_INDEX) && ValidSymbol(EINTRCNT_SYM_INDEX) && (nintr > 0);
# else
	return ValidSymbol(INTRCNT_SYM_INDEX) && ValidSymbol(EINTRCNT_SYM_INDEX) && ((SymbolValue(EINTRCNT_SYM_INDEX) - SymbolValue(INTRCNT_SYM_INDEX)) > 0);
# endif
#elif defined(XOSVIEW_NETBSD)
	return ValidSymbol(ALLEVENTS_SYM_INDEX);
#endif
	return 1;
}

int
BSDNumInts() {
	/* This code is stolen from vmstat. */
	int count = 0, nbr = 0;
#if defined(XOSVIEW_FREEBSD)
	int nintr = 0;
	size_t inamlen;
	char *intrnames, *intrs;

# if __FreeBSD_version >= 900040
	safe_kvm_read(nlst[EINTRCNT_SYM_INDEX].n_value, &nintr, sizeof(nintr));
	safe_kvm_read(nlst[EINTRNAMES_SYM_INDEX].n_value, &inamlen, sizeof(inamlen));
# else
	nintr = nlst[EINTRCNT_SYM_INDEX].n_value - nlst[INTRCNT_SYM_INDEX].n_value;
	inamlen = nlst[EINTRNAMES_SYM_INDEX].n_value - nlst[INTRNAMES_SYM_INDEX].n_value;
#  endif
	if (nintr == 0 || inamlen == 0) {
		warnx("Could not get interrupt numbers.");
		return 0;
	}

	intrnames = intrs = (char *)malloc((size_t)inamlen);
	if (!intrs)
		err(EX_OSERR, "BSDNumInts(): malloc failed");
	safe_kvm_read(nlst[INTRNAMES_SYM_INDEX].n_value, intrs, (size_t)inamlen);
	nintr /= sizeof(long);
	while (--nintr >= 0) {
		if ( sscanf(intrnames, "irq%d", &nbr) && nbr > count )
			count = nbr;
		intrnames += strlen(intrnames) + 1;
	}
	free(intrs);
#elif defined(XOSVIEW_NETBSD)
	struct evcntlist events;
	struct evcnt evcnt, *evptr;
	char dummy[30];
	char *name;

	safe_kvm_read(nlst[ALLEVENTS_SYM_INDEX].n_value, &events, sizeof(events));
	evptr = TAILQ_FIRST(&events);
	while (evptr) {
		safe_kvm_read((u_long)evptr, &evcnt, sizeof(evcnt));
		if (evcnt.ev_type == EVCNT_TYPE_INTR) {
			if ( !(name = (char *)malloc(evcnt.ev_namelen + 1)) )
				err(EX_OSERR, "BSDNumInts(): malloc failed");
			safe_kvm_read((u_long)evcnt.ev_name, name, evcnt.ev_namelen + 1);
			if ( sscanf(name, "%s%d", dummy, &nbr) == 2 && nbr > count )
				count = nbr;
			free(name);
		}
		evptr = TAILQ_NEXT(&evcnt, ev_list);
	}
#elif defined(XOSVIEW_OPENBSD)
	int nintr = 0;
	int mib_int[4] = { CTL_KERN, KERN_INTRCNT, KERN_INTRCNT_NUM };
	size_t size = sizeof(nintr);
	if ( sysctl(mib_int, 3, &nintr, &size, NULL, 0) < 0 ) {
		warn("Could not get interrupt count");
		return 0;
	}
	for (int i = 0; i < nintr; i++) {
		mib_int[2] = KERN_INTRCNT_VECTOR;
		mib_int[3] = i;
		size = sizeof(nbr);
		if ( sysctl(mib_int, 4, &nbr, &size, NULL, 0) < 0 )
			warn("Could not get name of interrupt %d", i);
		else
			if ( nbr > count )
				count = nbr;
	}
#else  // XOSVIEW_DFBSD
	int nintr = 0;
	size_t inamlen;
	char *intrnames, *intrs;

	if ( sysctlbyname("hw.intrnames", NULL, &inamlen, NULL, 0) != 0 ) {
		warn("sysctl hw.intrnames failed");
		return 0;
	}
	intrnames = intrs = (char *)malloc(inamlen);
	if (!intrs)
		err(EX_OSERR, "BSDNumInts(): malloc failed");

	if ( sysctlbyname("hw.intrnames", intrs, &inamlen, NULL, 0) < 0 ) {
		warn("sysctl hw.intrnames failed");
		free(intrs);
		return 0;
	}
	for (uint i = 0; i < inamlen; i++) {
		if (intrs[i] == '\0')  // count end-of-strings
			nintr++;
	}
	for (int i = 0; i < nintr; i++) {
		if ( sscanf(intrnames, "irq%d", &nbr) == 0 ) {
			if ( ++nbr > count )  // unused ints are named irqn where
				count = nbr;      // 0<=n<=255, used ones have device name
		}
		intrnames += strlen(intrnames) + 1;
	}
	free(intrs);
#endif
	return count;  // this is the highest numbered interrupt
}

void
BSDGetIntrStats(unsigned long *intrCount, unsigned int *intrNbrs) {
	/* This code is stolen from vmstat */
	int nbr = 0;
#if defined(XOSVIEW_FREEBSD)
	unsigned long *kvm_intrcnt, *intrcnt;
	char *kvm_intrnames, *intrnames;
	size_t inamlen;
	int nintr = 0;

# if __FreeBSD_version >= 900040
	safe_kvm_read(nlst[EINTRCNT_SYM_INDEX].n_value, &nintr, sizeof(nintr));
	safe_kvm_read(nlst[EINTRNAMES_SYM_INDEX].n_value, &inamlen, sizeof(inamlen));
# else
	nintr = nlst[EINTRCNT_SYM_INDEX].n_value - nlst[INTRCNT_SYM_INDEX].n_value;
	inamlen = nlst[EINTRNAMES_SYM_INDEX].n_value - nlst[INTRNAMES_SYM_INDEX].n_value;
# endif
	if (nintr == 0 || inamlen == 0) {
		warnx("Could not get interrupt numbers.");
		return;
	}
	if ( ((kvm_intrcnt = (unsigned long *)malloc(nintr)) == NULL) ||
	     ((kvm_intrnames = (char *)malloc(inamlen)) == NULL) )
		err(EX_OSERR, "BSDGetIntrStats(): malloc failed");

	// keep track of the mem we're given:
	intrcnt = kvm_intrcnt;
	intrnames = kvm_intrnames;

	safe_kvm_read(nlst[INTRCNT_SYM_INDEX].n_value, kvm_intrcnt, nintr);
	safe_kvm_read(nlst[INTRNAMES_SYM_INDEX].n_value, kvm_intrnames, inamlen);

	nintr /= sizeof(long);
	/* kvm_intrname has the ASCII names of the IRQs, every null-terminated
	 * string corresponds to a value in the kvm_intrcnt array
	 * e.g. irq1: atkbd0   */
	while (--nintr >= 0) {
		/* Figure out which irq we have here */
		if ( sscanf(kvm_intrnames, "irq%d", &nbr) == 1 ) {
			intrCount[nbr] = *kvm_intrcnt;
			if (intrNbrs)
				intrNbrs[nbr] = 1;
		}
		kvm_intrcnt++;
		kvm_intrnames += strlen(kvm_intrnames) + 1;
	}
	free(intrcnt);
	free(intrnames);
#elif defined(XOSVIEW_NETBSD)
	struct evcntlist events;
	struct evcnt evcnt, *evptr;
	char dummy[30];
	char *name;

	safe_kvm_read(nlst[ALLEVENTS_SYM_INDEX].n_value, &events, sizeof(events));
	evptr = TAILQ_FIRST(&events);
	while (evptr) {
		safe_kvm_read((u_long)evptr, &evcnt, sizeof(evcnt));
		if (evcnt.ev_type == EVCNT_TYPE_INTR) {
			if ( !(name = (char *)malloc(evcnt.ev_namelen + 1)) )
				err(EX_OSERR, "BSDGetIntrStats(): malloc failed");
			safe_kvm_read((u_long)evcnt.ev_name, name, evcnt.ev_namelen + 1);
			if ( sscanf(name, "%s%d", dummy, &nbr) == 2 ) {
				intrCount[nbr] = evcnt.ev_count;
				if (intrNbrs)
					intrNbrs[nbr] = 1;
			}
			free(name);
		}
		evptr = TAILQ_NEXT(&evcnt, ev_list);
	}
#elif defined(XOSVIEW_OPENBSD)
	int nintr = 0;
	u_quad_t count = 0;
	size_t size = sizeof(nintr);
	int mib_int[4] = { CTL_KERN, KERN_INTRCNT, KERN_INTRCNT_NUM };
	if ( sysctl(mib_int, 3, &nintr, &size, NULL, 0) < 0 ) {
		warn("Could not get interrupt count");
		return;
	}
	for (int i = 0; i < nintr; i++) {
		mib_int[2] = KERN_INTRCNT_VECTOR;
		mib_int[3] = i;
		size = sizeof(nbr);
		if ( sysctl(mib_int, 4, &nbr, &size, NULL, 0) < 0 )
			continue;  // not active
		mib_int[2] = KERN_INTRCNT_CNT;
		size = sizeof(count);
		if ( sysctl(mib_int, 4, &count, &size, NULL, 0) < 0 ) {
			warn("sysctl kern.intrcnt.cnt.%d failed", i);
			count = 0;
		}
		intrCount[nbr] += count;  // += because ints can share number
		if (intrNbrs)
			intrNbrs[nbr] = 1;
	}
#else  // XOSVIEW_DFBSD
	int nintr = 0;
	size_t inamlen;
	unsigned long *intrcnt;
	char *dummy, *intrs, **intrnames;

	if ( sysctlbyname("hw.intrnames", NULL, &inamlen, NULL, 0) != 0 ) {
		warn("sysctl hw.intrnames failed");
		return;
	}

	dummy = intrs = (char *)malloc(inamlen);
	if (!intrs)
		err(EX_OSERR, "BSDGetIntrStats(): malloc failed");
	if ( sysctlbyname("hw.intrnames", intrs, &inamlen, NULL, 0) < 0 ) {
		warn("sysctl hw.intrnames failed");
		free(intrs);
		return;
	}
	for (uint i = 0; i < inamlen; i++) {
		if (intrs[i] == '\0')  // count end-of-strings
			nintr++;
	}
	if ( !(intrnames = (char **)malloc(nintr * sizeof(char *))) )
		err(EX_OSERR, "BSDGetIntrStats(): malloc failed");

	for (int i = 0; i < nintr; i++) {
		intrnames[i] = intrs;
		intrs += strlen(intrs) + 1;
	}
	if ( !(intrcnt = (unsigned long *)calloc(nintr, sizeof(long))) )
		err(EX_OSERR, "BSDGetIntrStats(): malloc failed");

	inamlen = nintr * sizeof(long);
	if ( sysctlbyname("hw.intrcnt", intrcnt, &inamlen, NULL, 0) < 0 )
		err(EX_OSERR, "sysctl hw.intrcnt failed");

	for (int i = 0; i < nintr; i++) {
		if ( sscanf(intrnames[i], "irq%d", &nbr) == 0 ) {
			nbr++;
			intrCount[nbr] += intrcnt[i];
			if (intrNbrs)
				intrNbrs[nbr] = 1;
		}
	}
	free(dummy);
	free(intrnames);
	free(intrcnt);
#endif
}


//  ---------------------- Sensor Meter stuff  ---------------------------------

static int mib_cpu[2] = { CTL_HW, HW_NCPU };

int
BSDCountCpus(void) {
	int cpus = 0;
	size_t size = sizeof(cpus);
	if ( sysctl(mib_cpu, 2, &cpus, &size, NULL, 0) < 0 )
		warn("sysctl hw.ncpu failed.");
	return cpus;
}

void
BSDGetCPUTemperature(float *temps, float *tjmax) {
	if (!temps)
		errx(EX_SOFTWARE, "NULL pointer passed to BSDGetCPUTemperature().");
#if defined(XOSVIEW_NETBSD)
	// All kinds of sensors are read with libprop. We have to go through them
	// to find either Intel Core 2 or AMD ones. Actual temperature is in
	// cur-value and TjMax, if present, in critical-max.
	// Values are in microdegrees Kelvin.
	int fd;
	const char *name = NULL;
	char dummy[20];
	prop_dictionary_t pdict;
	prop_object_t pobj, pobj1, pobj2;
	prop_object_iterator_t piter, piter2;
	prop_array_t parray;

	if ( (fd = open(_PATH_SYSMON, O_RDONLY)) == -1 ) {
		warn("Could not open %s", _PATH_SYSMON);
		return;  // this seems to happen occasionally, so only warn
	}
	if (prop_dictionary_recv_ioctl(fd, ENVSYS_GETDICTIONARY, &pdict))
		err(EX_OSERR, "Could not get sensor dictionary");
	if (close(fd) == -1)
		err(EX_OSERR, "Could not close %s", _PATH_SYSMON);

	if (prop_dictionary_count(pdict) == 0) {
		warn("No sensors found");
		return;
	}
	if ( !(piter = prop_dictionary_iterator(pdict)) )
		err(EX_OSERR, "Could not get sensor iterator");

	while ( (pobj = prop_object_iterator_next(piter)) ) {
		parray = (prop_array_t)prop_dictionary_get_keysym(pdict, (prop_dictionary_keysym_t)pobj);
		if (prop_object_type(parray) != PROP_TYPE_ARRAY)
			continue;
		name = prop_dictionary_keysym_cstring_nocopy((prop_dictionary_keysym_t)pobj);
		if ( strncmp(name, "coretemp", 8) && strncmp(name, "amdtemp", 7) )
			continue;
		if ( !(piter2 = prop_array_iterator(parray)) )
			err(EX_OSERR, "Could not get sensor iterator");

		int i = 0;
		sscanf(name, "%[^0-9]%d", dummy, &i);
		while ( (pobj = prop_object_iterator_next(piter2)) ) {
			if ( !(pobj1 = prop_dictionary_get((prop_dictionary_t)pobj, "type")) )
				continue;
			if ( (pobj1 = prop_dictionary_get((prop_dictionary_t)pobj, "cur-value")) )
				temps[i] = (prop_number_integer_value((prop_number_t)pobj1) / 1000000.0) - 273.15;
			if ( (pobj2 = prop_dictionary_get((prop_dictionary_t)pobj, "critical-max")) && tjmax )
				tjmax[i] = (prop_number_integer_value((prop_number_t)pobj2) / 1000000.0) - 273.15;
		}
		prop_object_iterator_release(piter2);
	}
	prop_object_iterator_release(piter);
	prop_object_release(pdict);
#else  /* XOSVIEW_NETBSD */
	int val = 0;
	size_t size = sizeof(val);

#if defined(XOSVIEW_OPENBSD) || defined(XOSVIEW_DFBSD)
	// All kinds of sensors are read with sysctl. We have to go through them
	// to find either Intel Core 2 or AMD ones.
	// Values are in microdegrees Kelvin.
	struct sensordev sd;
	struct sensor s;
	int nbr = 0;
	char dummy[10];

	for (int dev = 0; dev < 1024; dev++) {  // go through all sensor devices
		mib_sen[2] = dev;
		size = sizeof(sd);
		if ( sysctl(mib_sen, 3, &sd, &size, NULL, 0) < 0 ) {
			if (errno == ENOENT)
				break;  // no more sensors
			if (errno == ENXIO)
				continue;  // no sensor with this mib
			err(EX_OSERR, "sysctl hw.sensors.%d failed", dev);
		}
		if ( strncmp(sd.xname, "cpu", 3) )
			continue;  // not CPU sensor
		sscanf(sd.xname, "%[^0-9]%d", dummy, &nbr);

		mib_sen[3] = SENSOR_TEMP;  // for each device, get temperature sensors
		for (int i = 0; i < sd.maxnumt[SENSOR_TEMP]; i++) {
			mib_sen[4] = i;
			size = sizeof(s);
			if ( sysctl(mib_sen, 5, &s, &size, NULL, 0) < 0 )
				continue;  // no sensor on this core?
			if (s.flags & SENSOR_FINVALID)
				continue;
			temps[nbr] = (float)(s.value - 273150000) / 1000000.0;
		}
	}
#else  /* XOSVIEW_FREEBSD */
	// Temperatures can be read with sysctl dev.cpu.%d.temperature on both
	// Intel Core 2 and AMD K8+ processors.
	// Values are in degrees Celsius (FreeBSD < 7.2) or in
	// 10*degrees Kelvin (FreeBSD >= 7.3).
	char name[25];
	int cpus = BSDCountCpus();
	for (int i = 0; i < cpus; i++) {
		snprintf(name, 25, "dev.cpu.%d.temperature", i);
		if ( sysctlbyname(name, &val, &size, NULL, 0) == 0)
#if __FreeBSD_version >= 702106
			temps[i] = ((float)val - 2732.0) / 10.0;
#else
			temps[i] = (float)val;
#endif
		else {
			warn("sysctl %s failed", name);
			temps[i] = -300.0;
		}
		if (tjmax) {
			snprintf(name, 25, "dev.cpu.%d.coretemp.tjmax", i);
			if ( sysctlbyname(name, &val, &size, NULL, 0) == 0 )
#if __FreeBSD_version >= 702106
				tjmax[i] = ((float)val - 2732.0) / 10.0;
#else
				tjmax[i] = (float)val;
#endif
			else {
				warn("sysctl %s failed", name);
				tjmax[i] = -300.0;
			}
		}
	}
#endif
#endif
}

void
BSDGetSensor(const char *name, const char *valname, float *value) {
	if (!name || !valname || !value)
		errx(EX_SOFTWARE, "NULL pointer passed to BSDGetSensor().");
#if defined(XOSVIEW_NETBSD)
	/* Adapted from envstat. */
	// All kinds of sensors are read with libprop. Specific device and value
	// can be asked for. Values are transformed to suitable units.
	int fd, val = 0;
	char type[20];
	prop_dictionary_t pdict;
	prop_object_t pobj, pobj1;
	prop_object_iterator_t piter;

	if ( (fd = open(_PATH_SYSMON, O_RDONLY)) == -1 ) {
		warn("Could not open %s", _PATH_SYSMON);
		return;  // this seems to happen occasionally, so only warn
	}
	if (prop_dictionary_recv_ioctl(fd, ENVSYS_GETDICTIONARY, &pdict))
		err(EX_OSERR, "Could not get sensor dictionary");
	if (close(fd) == -1)
		err(EX_OSERR, "Could not close %s", _PATH_SYSMON);

	if (prop_dictionary_count(pdict) == 0) {
		warn("No sensors found");
		return;
	}
	pobj = prop_dictionary_get(pdict, name);
	if (prop_object_type(pobj) != PROP_TYPE_ARRAY)
		err(EX_USAGE, "Device %s does not exist", name);

	if ( !(piter = prop_array_iterator((prop_array_t)pobj)) )
		err(EX_OSERR, "Could not get sensor iterator");

	while ( (pobj = prop_object_iterator_next(piter)) ) {
		if ( !(pobj1 = prop_dictionary_get((prop_dictionary_t)pobj, "type")) )
			continue;
		strlcpy(type, prop_string_cstring_nocopy((prop_string_t)pobj1), 20);
		if ( strncmp(type, "Indicator", 3) == 0 ||
		     strncmp(type, "Battery", 3) == 0   ||
		     strncmp(type, "Drive", 3) == 0 )
			continue;  // these are string values
		if ( (pobj1 = prop_dictionary_get((prop_dictionary_t)pobj, valname)) )
			val = prop_number_integer_value((prop_number_t)pobj1);
		else
			err(EX_USAGE, "Value %s does not exist", valname);
		if ( strncmp(type, "Temperature", 4) == 0 )
			*value = (val / 1000000.0) - 273.15;  // temperatures are in microkelvins
		else if ( strncmp(type, "Fan", 3) == 0 ||
		          strncmp(type, "Integer", 3) == 0 )
			*value = (float)val;                  // plain integer values
		else if ( strncmp(type, "Voltage", 4) == 0 ||
		          strncmp(type, "Ampere", 4) == 0  ||
		          strncmp(type, "Watt", 4) == 0    ||
		          strncmp(type, "Ohms", 4) == 0 )
			*value = (float)val / 1000000.0;      // electrical units are in micro{V,A,W,Ohm}
	}
	prop_object_iterator_release(piter);
	prop_object_release(pdict);
#else  /* XOSVIEW_NETBSD */
	size_t size;
	char dummy[50];
#if defined(XOSVIEW_FREEBSD) || defined(XOSVIEW_DFBSD)
	// FreeBSD has no sensor framework, but ACPI thermal zones might work.
	// They are readable through sysctl (also works in Dragonfly).
	// Values are in 10 * degrees Kelvin.
	if ( strncmp(name, "tz", 2) == 0 ) {
		int val = 0;
		size = sizeof(val);
		snprintf(dummy, 50, "hw.acpi.thermal.%s.%s", name, valname);
		if ( sysctlbyname(dummy, &val, &size, NULL, 0) < 0 )
			err(EX_OSERR, "sysctl %s failed", dummy);
		*value = ((float)val - 2732.0) / 10.0;
		return;
	}
	// If Dragonfly and tzN specified, return. Otherwise, fall through.
#endif
#if defined(XOSVIEW_OPENBSD) || defined(XOSVIEW_DFBSD)
	/* Adapted from systat. */
	// All kinds of sensors are read with sysctl. We have to go through them
	// to find the required device and value. Parameter 'name' is the device
	// name and 'valname' consists of type and sensor index (e.g. it0.temp1).
	//  Values are transformed to suitable units.
	int index = -1;
	struct sensordev sd;
	struct sensor s;

	for (int dev = 0; dev < 1024; dev++) {  // go through all sensor devices
		mib_sen[2] = dev;
		size = sizeof(sd);
		if ( sysctl(mib_sen, 3, &sd, &size, NULL, 0) < 0 ) {
			if (errno == ENOENT)
				break;  // no more devices
			if (errno == ENXIO)
				continue;  // no device with this mib
			err(EX_OSERR, "sysctl hw.sensors.%d failed", dev);
		}
		if ( strncmp(sd.xname, name, sizeof(name)) )
			continue;  // sensor name does not match

		for (int t = 0; t < SENSOR_MAX_TYPES; t++) {
			if ( strncmp(sensor_type_s[t], valname, strlen(sensor_type_s[t])) )
				continue;  // wrong type
			mib_sen[3] = t;
			sscanf(valname, "%[^0-9]%d", dummy, &index);
			if (index < sd.maxnumt[t]) {
				mib_sen[4] = index;
				size = sizeof(s);
				if ( sysctl(mib_sen, 5, &s, &size, NULL, 0) < 0 ) {
					if (errno != ENOENT)
						err(EX_OSERR, "sysctl hw.sensors.%d.%d.%d failed", dev, t, index);
					continue;  // no more sensors
				}
				if (s.flags & SENSOR_FINVALID)
					continue;
				switch (t) {
				case SENSOR_TEMP:
					*value = (float)(s.value - 273150000) / 1000000.0;
					break;
				case SENSOR_FANRPM:
				case SENSOR_OHMS:
				case SENSOR_INDICATOR:
				case SENSOR_INTEGER:
					*value = (float)s.value;
					break;
				case SENSOR_VOLTS_DC:
				case SENSOR_VOLTS_AC:
				case SENSOR_WATTS:
				case SENSOR_AMPS:
				case SENSOR_WATTHOUR:
				case SENSOR_AMPHOUR:
				case SENSOR_LUX:
#if defined(XOSVIEW_OPENBSD)
				case SENSOR_FREQ:
				case SENSOR_ANGLE:
#endif
					*value = (float)s.value / 1000000.0;
					break;
				case SENSOR_PERCENT:
#if defined(XOSVIEW_OPENBSD)
				case SENSOR_HUMIDITY:
#endif
					*value = (float)s.value / 1000.0;
					break;
				case SENSOR_TIMEDELTA:
					*value = (float)s.value / 1000000000.0;
					break;
				default:
					*value = (float)s.value;
					break;
				}
			}
		}
	}
#endif
#endif
}
