//
//  NetBSD port:  
//  Copyright (c) 1995, 1996, 1997 by Brian Grayson (bgrayson@ece.utexas.edu)
//
//  This file was written by Brian Grayson for the NetBSD and xosview
//    projects.
// dummy device ignore code by : David Cuka (dcuka@intgp1.ih.att.com)
//  This file may be distributed under terms of the GPL or of the BSD
//    license, whichever you choose.  The full license notices are
//    contained in the files COPYING.GPL and COPYING.BSD, which you
//    should have received.  If not, contact one of the xosview
//    authors for a copy.
//
// $Id$
//
#ifndef XOSVIEW_NETBSD
/*  NetBSD pulls in stdio.h via one of the other includes, but
 *  the other BSDs don't.  */
# include <stdio.h>
#endif

#include <fcntl.h>
#include <kvm.h>
#include <limits.h>		/*  For _POSIX2_LINE_MAX  */
#include <string.h>		/*  For strncpy().  */

#include <err.h>                /*  For err(), warn(), etc.  BCG  */
#include <errno.h>
#include <sys/dkstat.h>         /*  For CPUSTATES, which tells us how
                                      many cpu states there are.  */
#ifndef XOSVIEW_FREEBSD
#include <sys/disk.h>		/*  For disk statistics.  */
#endif

#include <sys/socket.h>         /*  These two are needed for the  */
#include <net/if.h>             /*    NetMeter helper functions.  */
#if defined(XOSVIEW_FREEBSD)
# include <osreldate.h>
# if (__FreeBSD_version >= 300000)
#  include <net/if_var.h>
# endif
#endif

#ifdef HAVE_DEVSTAT
#include <devstat.h>
#include <stdlib.h>	/*  For malloc().  */
void DevStat_Init();
int DevStat_Get();
#endif

#include <sys/param.h>	/*  Needed by both UVM and swapctl stuff.  */
#if defined(UVM)
#include <string.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>
#include <sys/device.h>
#include <vm/vm.h>	/* XXX  Is this needed?  */
#else
#include <sys/vmmeter.h>	/*  For struct vmmeter.  */
#endif

#ifdef HAVE_SWAPCTL
#include <unistd.h>		/*  For swapctl proto.  */
#include <vm/vm_swap.h>		/*  For swapent, SWAP_*.  */
#include <stdlib.h>		/*  For malloc(), free().  */
#endif

#include "general.h"
#include "kernel.h"		/*  To grab CVSID stuff.  */

CVSID("$Id$");
CVSID_DOT_H(KERNEL_H_CVSID);


// ------------------------  local variables  ---------------------
kvm_t* kd = NULL;	//  This single kvm_t is shared by all
                        //  of the kvm routines.

//  This struct has the list of all the symbols we want from the kernel.
static struct nlist nlst[] =
{
{ "_cp_time" },
#define CP_TIME_SYM_INDEX 0
{ "_ifnet" },
#define IFNET_SYM_INDEX 1
#if defined(UVM)
{ "_disklist" },	/*  Keep UVM happy...  */
#else
{ "_cnt" },
#endif
#define VMMETER_SYM_INDEX	2

#ifndef XOSVIEW_FREEBSD	/*  NetBSD has a disklist, which FreeBSD doesn't...  */
{ "_disklist" },
#define DISKLIST_SYM_INDEX	3
{ "_disklist" },
#define DUMMY_4			4
{ "_disklist" },
#define DUMMY_5			5
{ "_disklist" },
#define DUMMY_6			6
{ "_intrcnt" },
#define INTRCNT_SYM_INDEX 	7
{ "_eintrcnt" },
#define EINTRCNT_SYM_INDEX 	8

#else                   // but FreeBSD has unified buffer cache...

{ "_bufspace" },
#define BUFSPACE_SYM_INDEX      3
{ "_intr_countp" },
#define INTRCOUNTP_SYM_INDEX 	4
{ "_intrcnt" },
#define INTRCNT_SYM_INDEX 	5
{ "_eintrcnt" },
#define EINTRCNT_SYM_INDEX 	6

#ifndef HAVE_DEVSTAT

{ "_dk_ndrive" },
#define DK_NDRIVE_SYM_INDEX     7
{ "_dk_wds" },
#define DK_WDS_SYM_INDEX        8

#endif /*HAVE_DEVSTAT */

#endif /* XOSVIEW_FREEBSD */

  {NULL}
};

static char kernelFileName[_POSIX2_LINE_MAX];

// ------------------------  utility functions  -------------------
//  The following is an error-checking form of kvm_read.  In addition
//  it uses kd as the implicit kernel-file to read.  Saves typing.
//  Since this is C++, it's an inline function rather than a macro.

static inline void
safe_kvm_read (u_long kernel_addr, void* user_addr, size_t nbytes)
{
    /*  Check for obvious bad symbols (i.e., from /netbsd when we
     *  booted off of /netbsd.old), such as symbols that reference
     *  0x00000000 (or anywhere in the first 256 bytes of memory).  */
  int retval = 0;
  if ((kernel_addr&0xffffff00) == 0)
    errx(-1, "safe_kvm_read() was attempted on EA %#lx\n", kernel_addr);
#if 0
  if ((kernel_addr&0xf0000000) != 0xf0000000)
    warnx("safe_kvm_read() was attempted on EA %#lx\n", kernel_addr);
#endif
  if ((retval = kvm_read (kd, kernel_addr, user_addr, nbytes))==-1)
    err(-1, "kvm_read() of kernel address %#lx", kernel_addr);
  if (retval != (int) nbytes) {
    warnx("safe_kvm_read(%#lx) returned %d bytes, not %d!",
	kernel_addr, retval, nbytes);
  }
}

//  This version uses the symbol offset in the nlst variable, to make it
//  a little more convenient.  BCG
static inline void
safe_kvm_read_symbol (int nlstOffset, void* user_addr, size_t nbytes)
{
  safe_kvm_read (nlst[nlstOffset].n_value, user_addr, nbytes);
}


int
ValidSymbol (int index)
{
  return ((nlst[index].n_value & 0xffffff00) != 0);
}


void
BSDInit()
{
  kernelFileName[0] = '\0';
}

void
SetKernelName(const char* const kernelName)
{
  if (strlen(kernelName)>=_POSIX2_LINE_MAX)
  {
    fprintf (stderr, "Error:  kernel file name of '%s' is too long!\n",
      kernelName);
    exit(1);
  }
  strncpy(kernelFileName, kernelName, _POSIX2_LINE_MAX);
}

void
OpenKDIfNeeded()
{
  char unusederrorstring[_POSIX2_LINE_MAX];

  if (kd) return; //  kd is non-NULL, so it has been initialized.  BCG

    /*  Open it read-only, for a little added safety.  */
    /*  If the first character of kernelFileName is not '\0', then use
	that kernel file.  Otherwise, use the default kernel, by
	specifying NULL.  */
  if ((kd = kvm_openfiles ((kernelFileName[0]) ? kernelFileName : NULL,
			    NULL, NULL, O_RDONLY, unusederrorstring))
      == NULL)
	  err (-1, "OpenKDIfNeeded():kvm-open()");
  // Parenthetical note:  FreeBSD kvm_openfiles() uses getbootfile() to get
  // the correct kernel file if the 1st arg is NULL.  As far as I can see,
  // one should always use NULL in FreeBSD, but I suppose control is never a
  // bad thing... (pavel 21-Jan-1998)

  /*  Now grab the symbol offsets for the symbols that we want.  */
  kvm_nlist (kd, nlst);

  //  Look at all of the returned symbols, and check for bad lookups.
  //  (This may be unnecessary, but better to check than not to...  )
  struct nlist * nlp = nlst;
  while (nlp && nlp->n_name) {
    if ((nlp->n_type == 0) || (nlp->n_value == 0))
      /*errx (-1, "kvm_nlist() lookup failed for symbol '%s'.", nlp->n_name);*/
      warnx ("kvm_nlist() lookup failed for symbol '%s'.", nlp->n_name);
    nlp++;
  }
#ifdef HAVE_DEVSTAT
  DevStat_Init();
#endif
}


// ------------------------  PageMeter functions  -----------------
void
BSDPageInit() {
  OpenKDIfNeeded();
  /*  XXX  Ought to add a check/warning for UVM/non-UVM kernel here, to
   *  avoid surprises.  */
}


#if defined(UVM)
void
BSDGetUVMPageStats(struct uvmexp* uvm) {
  size_t size;
  int mib[2];
  if (!uvm) errx(-1, "BSDGetUVMPageStats():  passed pointer was null!\n");
  size = sizeof(uvmexp);
  mib[0] = CTL_VM;
  mib[1] = VM_UVMEXP;
  if (sysctl(mib, 2, uvm, &size, NULL, 0) < 0) {
    printf("can't get uvmexp: %s\n", strerror(errno));
    memset(&uvm, 0, sizeof(uvmexp));
  }
}
#else
void
BSDGetPageStats(struct vmmeter* vmp) {
  if (!vmp) errx(-1, "BSDGetPageStats():  passed pointer was null!\n");
  safe_kvm_read_symbol(VMMETER_SYM_INDEX, vmp, sizeof(struct vmmeter));
}
#endif
#ifdef XOSVIEW_FREEBSD
// This function returns the num bytes devoted to buffer cache
void
FreeBSDGetBufspace(int* bfsp)
{
    if (! bfsp) errx (-1, "FreeBSDGetBufspace(): passed null ptr argument\n");
    safe_kvm_read_symbol (BUFSPACE_SYM_INDEX, bfsp, sizeof(int));
}
#endif

// ------------------------  CPUMeter functions  ------------------

void
BSDCPUInit()
{
  OpenKDIfNeeded();
}

void
BSDGetCPUTimes (long* timeArray)
{
  if (!timeArray) errx (-1, "BSDGetCPUTimes():  passed pointer was null!\n");
  if (CPUSTATES != 5)
    errx (-1, "Error:  xosview for *BSD expects 5 cpu states!\n");
  safe_kvm_read_symbol (CP_TIME_SYM_INDEX, timeArray, sizeof (long) * CPUSTATES);
}


// ------------------------  NetMeter functions  ------------------
void
BSDNetInit()
{
  OpenKDIfNeeded();
}

void
BSDGetNetInOut (long long * inbytes, long long * outbytes)
{

#if (__FreeBSD_version < 300000) //werner May/29/98 quick hack for current

  struct ifnet * ifnetp;
  struct ifnet ifnet;
  //char ifname[256];

  //  The "ifnet" symbol in the kernel points to a 'struct ifnet' pointer.
  safe_kvm_read (nlst[IFNET_SYM_INDEX].n_value, &ifnetp, sizeof(ifnetp));

  *inbytes = 0;
  *outbytes = 0;

  while (ifnetp) {
    //  Now, dereference the pointer to get the ifnet struct.
    safe_kvm_read ((u_long) ifnetp, &ifnet, sizeof(ifnet));
#ifdef NET_DEBUG
    safe_kvm_read ((u_long) ifnet.if_name, ifname, 256);
    snprintf (ifname, 256, "%s%d", ifname, ifnet.if_unit);
    printf ("Interface name is %s\n", ifname);
    printf ("Ibytes: %8ld Obytes %8ld\n", ifnet.if_ibytes, ifnet.if_obytes);
    printf ("Ipackets:  %8ld\n", ifnet.if_ipackets);
#endif
    *inbytes  += ifnet.if_ibytes;
    *outbytes += ifnet.if_obytes;

    //  Linked-list step taken from if.c in netstat source, line 120.
#ifdef XOSVIEW_FREEBSD
#if (__FreeBSD_version >= 300000) 
    ifnetp = (struct ifnet*) ifnet.if_link.tqe_next; 
#else 
    ifnetp = (struct ifnet*) ifnet.if_next;
#endif
#else /* XOSVIEW_NETBSD or XOSVIEW_OPENBSD */
    ifnetp = (struct ifnet*) ifnet.if_list.tqe_next;
#endif
  }

#else
  (void) inbytes;
  (void) outbytes;
#endif	//werner

}


/*  ---------------------- Swap Meter stuff  -----------------  */
#if defined(HAVE_SWAPCTL)
struct swapent *sep;
int nswapAllocd = 0;
#endif

int
BSDSwapInit() {
  OpenKDIfNeeded();
  /*  Need to merge some of swapinteral.cc here, to be smart about
   *  missing kvm symbols (due to OS version mismatch, for example).
   *  */
  /*return ValidSymbol(*/
#if defined(HAVE_SWAPCTL)
  nswapAllocd = 32;	/*  Add buffering, beyond nswap...  */
  int nswap = swapctl(SWAP_NSWAP, 0, 0);
  if (nswap >= 0) nswapAllocd += nswap;
  sep = (struct swapent *) malloc(nswapAllocd * sizeof(*sep));
#endif
  return 1;
}

/*  If we have swapctl, let's enable that stuff.  However, the default
    is still the old method, so if we compile on a swapctl-capable machine,
    the binary will still work on an older machine.  */
#ifdef HAVE_SWAPCTL
//  This code is based on a patch sent in by Scott Stevens
//  (s.k.stevens@ic.ac.uk, at the time).
//

void
BSDGetSwapCtlInfo(int *totalp, int *freep)
{
  int	totalinuse, totalsize;
  int rnswap, nswap = swapctl(SWAP_NSWAP, 0, 0);
  struct swapent *swapiter;

  if (nswap < 1) {
    *totalp = *freep = 0;
    return;
  }

  /*  We did a malloc in the Init routine.  Only realloc if nswap has grown.  */
  if (nswap > nswapAllocd) {
    free(sep);
    nswapAllocd = nswap+32;	/*  Extra space, so we can avoid mallocs.  */
    sep = (struct swapent *)malloc(nswapAllocd * sizeof(*sep));
  }
  if (sep == NULL)
    err(1, "malloc");
  rnswap = swapctl(SWAP_STATS, (void *)sep, nswap);
  if (nswap < 0)
    errx(1, "SWAP_STATS");
  if (rnswap < 0)
    warnx("SWAP_STATS error");
  else if (nswap != rnswap)
    warnx("SWAP_STATS gave different value than SWAP_NSWAP "
    "(nswap=%d versus rnswap=%d)", nswap, rnswap);

  swapiter = sep;
  totalsize = totalinuse = 0;
  for (; rnswap-- > 0; swapiter++) {
    totalsize += swapiter->se_nblks;
    totalinuse += swapiter->se_inuse;
  }
#define BYTES_PER_SWAPBLOCK	512
  *totalp = totalsize * BYTES_PER_SWAPBLOCK;
  *freep = (totalsize - totalinuse) * BYTES_PER_SWAPBLOCK;
}
#endif	/*  Swapctl info retrieval  */

/*  ---------------------- Disk Meter stuff  -----------------  */

#ifdef HAVE_DEVSTAT
  /*
   * Make use of the new FreeBSD kernel device statistics library using
   * code shamelessly borrowed from xsysinfo, which borrowed shamelessly
   * from FreeBSD's iostat(8).
   */
  long generation;
  devstat_select_mode select_mode;
  struct devstat_match *matches;
  int num_matches;
  int num_selected, num_selections;
  long select_generation;
  static struct statinfo cur, last;
  int num_devices;
  struct device_selection *dev_select;
  char nodisk;

void DevStat_Init(void)
{
	/*
	 * Make sure that the userland devstat version matches the kernel
	 * devstat version.
	 */
	if (checkversion() < 0) {
		nodisk++;
		return;
	}

	/* find out how many devices we have */
	if ((num_devices = getnumdevs()) < 0) {
		nodisk++;
		return;
	}

	cur.dinfo = (struct devinfo *)malloc(sizeof(struct devinfo));
	last.dinfo = (struct devinfo *)malloc(sizeof(struct devinfo));
	bzero(cur.dinfo, sizeof(struct devinfo));
	bzero(last.dinfo, sizeof(struct devinfo));

	/*
	 * Grab all the devices.  We don't look to see if the list has
	 * changed here, since it almost certainly has.  We only look for
	 * errors.
	 */
	if (getdevs(&cur) == -1) {
		nodisk++;
		return;
	}

	num_devices = cur.dinfo->numdevs;
	generation = cur.dinfo->generation;

	dev_select = NULL;

	/* only interested in disks */
	matches = NULL;
	if (buildmatch("da", &matches, &num_matches) != 0) {
		nodisk++;
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
	if (selectdevs(&dev_select, &num_selected,
		       &num_selections, &select_generation,
		       generation, cur.dinfo->devices, num_devices,
		       matches, num_matches,
		       NULL, 0,
		       select_mode, 10, 0) == -1)
		nodisk++;
}

int
DevStat_Get(void)
{
	register int dn;
	long double busy_seconds;
	u_int64_t total_transfers;
	u_int64_t total_bytes;
	struct devinfo *tmp_dinfo;
	int total_xfers = 0;
	int total_xbytes = 0;

	if (nodisk == 0) {
		/*
		 * Here what we want to do is refresh our device stats.
		 * getdevs() returns 1 when the device list has changed.
		 * If the device list has changed, we want to go through
		 * the selection process again, in case a device that we
		 * were previously displaying has gone away.
		 */
		switch (getdevs(&cur)) {
		case -1:
			return (0);
		case 1: {
			int retval;

			num_devices = cur.dinfo->numdevs;
			generation = cur.dinfo->generation;
			retval = selectdevs(&dev_select, &num_selected,
					    &num_selections, &select_generation,
					    generation, cur.dinfo->devices,
					    num_devices, matches, num_matches,
					    NULL, 0,
					    select_mode, 10, 0);
			switch(retval) {
			case -1:
				return (0);
			case 1:
				break;
			default:
				break;
			}
			break;
		}
		default:
			break;
		}

		/*
		 * Calculate elapsed time up front, since it's the same for all
		 * devices.
		 */
		busy_seconds = compute_etime(cur.busy_time, last.busy_time);

		/* this is the first time thru so just copy cur to last */
		if (last.dinfo->numdevs == 0) {
			tmp_dinfo = last.dinfo;
			last.dinfo = cur.dinfo;
			cur.dinfo = tmp_dinfo;
			last.busy_time = cur.busy_time;
			return (0);
		}


		for (dn = 0; dn < num_devices; dn++) {
			int di;

			if ((dev_select[dn].selected == 0)
			 || (dev_select[dn].selected > 10))
				continue;

			di = dev_select[dn].position;

			if (compute_stats(&cur.dinfo->devices[di],
				  &last.dinfo->devices[di], busy_seconds,
				  &total_bytes, &total_transfers,
				  NULL, NULL,
				  NULL, NULL, 
				  NULL, NULL)!= 0)
				  break;
			total_xfers += (int)total_transfers;
			total_xbytes += (int)total_bytes;
		}

		tmp_dinfo = last.dinfo;
		last.dinfo = cur.dinfo;
		cur.dinfo = tmp_dinfo;

		last.busy_time = cur.busy_time;

	} else {
		/* no disks found ? */
		total_xfers = 0;
		total_xbytes = 0;
	}

	return (total_xbytes);
}
#endif /* HAVE_DEVSTAT */

int
BSDDiskInit() {
  OpenKDIfNeeded(); 
#ifdef XOSVIEW_FREEBSD
#ifdef HAVE_DEVSTAT
  return 1;
#else
  return ValidSymbol(DK_NDRIVE_SYM_INDEX);
#endif
#else
  return ValidSymbol(DISKLIST_SYM_INDEX);
#endif
}

void
BSDGetDiskXFerBytes (unsigned long long *bytesXferred)
{
#ifdef XOSVIEW_FREEBSD
#ifdef HAVE_DEVSTAT
  *bytesXferred = DevStat_Get();
#else
  /* FreeBSD still has the old-style disk statistics in global arrays
     indexed by the disk number (defs are in <sys/dkstat.h> */

  long kvm_dk_wds[DK_NDRIVE];  /* # blocks of 32*16-bit words transferred */
  int kvm_dk_ndrive;           /* number of installed drives */

  safe_kvm_read_symbol (DK_NDRIVE_SYM_INDEX, &kvm_dk_ndrive, sizeof(int));
  safe_kvm_read_symbol (DK_WDS_SYM_INDEX, &kvm_dk_wds,
			sizeof(long)*DK_NDRIVE);

  for (int i=0; i < kvm_dk_ndrive; i++)
      *bytesXferred += kvm_dk_wds[i] * 64;
#endif
#else
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
  safe_kvm_read_symbol (DISKLIST_SYM_INDEX, &kvmdisklist, sizeof(kvmdisklist));
  kvmdiskptr = kvmdisklist.tqh_first;
  *bytesXferred = 0;
  while (kvmdiskptr != NULL)
  {
    safe_kvm_read ((u_long)kvmdiskptr, &kvmcurrdisk, sizeof(kvmcurrdisk));
      /*  Add up the contribution from this disk.  */
    *bytesXferred += kvmcurrdisk.dk_bytes;
#ifdef DEBUG
    printf ("Got %#x (lower 32bits)\n", (int) (*bytesXferred & 0xffffffff));
#endif
    kvmdiskptr = kvmcurrdisk.dk_link.tqe_next;
  }
#endif
}

/*  ---------------------- Interrupt Meter stuff  -----------------  */
static unsigned long kvm_intrcnt[128];// guess at space needed
#ifdef XOSVIEW_FREEBSD
static unsigned long kvm_intrptrs[NUM_INTR];
#endif

int
BSDIntrInit() {
    OpenKDIfNeeded();
    return ValidSymbol(INTRCNT_SYM_INDEX) && ValidSymbol(EINTRCNT_SYM_INDEX);
}

int BSDNumInts() {
  int nintr;
  OpenKDIfNeeded(); 
  nintr = (nlst[EINTRCNT_SYM_INDEX].n_value -
	   nlst[INTRCNT_SYM_INDEX].n_value)   / sizeof(int);
#ifdef XOSVIEW_FREEBSD
  /*  I'm not sure exactly how FreeBSD does things, but just do
   *  16 for now.  bgrayson  */
  return 16;
#else
# if defined(i386)
  /*  On the 386 platform, we count stray interrupts between
   *  intrct and eintrcnt, also, but we don't want to show these.  */
  return nintr/2;
# else
  return nintr;
# endif
#endif
}

void
BSDGetIntrStats (unsigned long intrCount[NUM_INTR])
{
#ifdef XOSVIEW_FREEBSD
    /* FreeBSD has an array of interrupt counts, indexed by device number.
       These are also indirected by IRQ num with intr_countp: */
    safe_kvm_read (nlst[INTRCOUNTP_SYM_INDEX].n_value,
		   kvm_intrptrs, sizeof(kvm_intrptrs));
    size_t len =  
	nlst[EINTRCNT_SYM_INDEX].n_value - nlst[INTRCNT_SYM_INDEX].n_value;
    safe_kvm_read (nlst[INTRCNT_SYM_INDEX].n_value, kvm_intrcnt, len);

    for (int i=0; i < NUM_INTR; i++) {
	int idx = (kvm_intrptrs[i] - nlst[INTRCNT_SYM_INDEX].n_value) /
	    sizeof(unsigned long);
	intrCount[i] = kvm_intrcnt[idx];
    }
#else /* XOSVIEW_FREEBSD */
  //  NetBSD/OpenBSD version, based on vmstat.c.  Note that the pc532
  //  platform does support intrcnt and eintrcnt, but vmstat uses
  //  the more advanced event counters to provide software
  //  counts.  We'll just use the intrcnt array here.  If anyone
  //  has problems, please mail me.  bgrayson
  {
    int nintr = BSDNumInts();
    safe_kvm_read(nlst[INTRCNT_SYM_INDEX].n_value, kvm_intrcnt,
      sizeof(long)*nintr);
    for (int i=0;i<nintr;i++) {
      intrCount[i] = kvm_intrcnt[i];
    }
  }
  return;
#endif
}
