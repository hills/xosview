//
//  NetBSD port:  
//  Copyright (c) 1995 Brian Grayson(bgrayson@pine.ece.utexas.edu)
//
// dummy device ignore code by : David Cuka (dcuka@intgp1.ih.att.com)
//
//  This file may be distributed under terms of the GPL
//

#include <stdio.h>
#include <fcntl.h>
#include <kvm.h>
#include <nlist.h>

#include <err.h>                /*  For err(), warn(), etc.  BCG  */
#include <sys/dkstat.h>         /*  For CPUSTATES, which tells us how
                                      many cpu states there are.  */

#include <sys/socket.h>         /*  These two are needed for the  */
#include <net/if.h>             /*    NetMeter helper functions.  */


// ------------------------  local variables  ---------------------
static kvm_t* kd = NULL;  //  This single kvm_t is shared by all
                          //  of the kvm routines.

//  This struct has the list of all the symbols we want from the kernel.
static struct nlist nlst[] =
{
{ "_cp_time" },
#define CP_TIME 0
{ "_ifnet" },
#define IFNET 1
  {NULL}
  };

static int initialized = 0;

// ------------------------  utility functions  -------------------
//  The following is an error-checking form of kvm_read.  In addition
//  it uses kd as the implicit kernel-file to read.  Saves typing.
//  Since this is C++, it's an inline function rather than a macro.

static inline void
safe_kvm_read (u_long kernel_addr, void* user_addr, size_t nbytes)
{
  if (kvm_read (kd, kernel_addr, user_addr, nbytes)==-1)
    err(-1, "kvm_read() of kernel address %#lx", kernel_addr);
}

//  This version uses the symbol offset in the nlst variable, to make it
//  a little more convenient.  BCG
static inline void
safe_kvm_read_symbol (int nlstOffset, void* user_addr, size_t nbytes)
{
  safe_kvm_read (nlst[nlstOffset].n_value, user_addr, nbytes);
}


void
OpenKDIfNeeded()
{
  char unusederrorstring[256];

  if (kd) return; //  kd is non-NULL, so it has been initialized.  BCG

  if ((kd = kvm_openfiles (NULL, NULL, NULL, O_RDONLY, unusederrorstring))
      == NULL)
	  err (-1, "OpenKDIfNeeded():kvm-open()");
}


// ------------------------  CPUMeter functions  ------------------

void
NetBSDCPUInit()
{
  OpenKDIfNeeded();
}

void
NetBSDGetCPUTimes (long* timeArray)
{
  if (!timeArray) errx (-1, "NetBSDGetCPUTimes():  passed pointer was null!\n");
  if (CPUSTATES != 5)
    errx (-1, "Error:  xosview for NetBSD expects 5 cpu states!\n");
  safe_kvm_read_symbol (CP_TIME, timeArray, sizeof (long) * CPUSTATES);
}


// ------------------------  NetMeter functions  ------------------
void
NetBSDNetInit()
{
  OpenKDIfNeeded();
  kvm_nlist (kd, nlst);

  //  Look at all of the returned symbols, and check for bad lookups.
  //  (This may be unnecessary, but better to check than not to...  )
  struct nlist * nlp = nlst;
  while (nlp && nlp->n_name) {
    if ((nlp->n_type == 0) || (nlp->n_value == 0))
      errx (-1, "kvm_nlist() lookup failed for symbol '%s'.", nlp->n_name);
    nlp++;
  }
  initialized = 1;
}

void
NetBSDGetNetInOut (long long * inbytes, long long * outbytes)
{
  if (!initialized)
    NetBSDNetInit();


  struct ifnet * ifnetp;
  struct ifnet ifnet;
  //char ifname[256];

  //  The "ifnet" symbol in the kernel points to a 'struct ifnet' pointer.
  safe_kvm_read (nlst[IFNET].n_value, &ifnetp, sizeof(ifnetp));

  *inbytes = 0;
  *outbytes = 0;

  while (ifnetp) {
    //  Now, dereference the pointer to get the ifnet struct.
    safe_kvm_read ((unsigned long) ifnetp, &ifnet, sizeof(ifnet));
#ifdef NET_DEBUG
    safe_kvm_read ((unsigned long) ifnet.if_name, ifname, 256);
    sprintf (ifname, "%s%d", ifname, ifnet.if_unit);
    printf ("Interface name is %s\n", ifname);
    printf ("Ibytes: %8ld Obytes %8ld\n", ifnet.if_ibytes, ifnet.if_obytes);
    printf ("Ipackets:  %8ld\n", ifnet.if_ipackets);
#endif
    *inbytes  += ifnet.if_ibytes;
    *outbytes += ifnet.if_obytes;

    //  Linked-list step taken from if.c in netstat source, line 120.
    ifnetp = (struct ifnet*) ifnet.if_list.tqe_next;
  }
}


