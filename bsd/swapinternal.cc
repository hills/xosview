// For OpenBSD with HAVE_SWAPCTL, we don't need any of this file.
// For others, include it to provide the old method of getting swap
// info.
#if !(defined(XOSVIEW_OPENBSD) && defined(HAVE_SWAPCTL))

//  Copyright (c) 1995 by Brian Grayson (bgrayson@netbsd.org)
//
//  This code is borrowed HEAVILY from the vmstat source code in the
//  NetBSD distribution.  As such, the NetBSD copyright claim/disclaimer
//  applies to most of this code.  The disclaimer, along with the CVS
//  header from the version from which this file was created, are included
//  below:
//
// $Id$
//
//  NOTE THAT THIS FILE IS UNDER THE BSD COPYRIGHT, AND NOT GPL!
//

/*      $NetBSD: swap.c,v 1.4 1995/08/31 22:20:19 jtc Exp $     */

/*-
 * Copyright (c) 1980, 1992, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

//---------------------  The remainder of this file is based/borrowed
//                       from /usr/src/bin/systat/swap.c in the NetBSD
//                       distribution.  Modifications are _no
//                       longer_ all marked appropriately via
//                       //------------:
//                       there are too many of them.  BCG

/*
 * swapinfo - based on a program of the same name by Kevin Lahey
 */

//---------------------  Note:  all of these includes were in the
//		       original source code.  I am leaving them
//		       undisturbed, although it is likely that
//		       some may be removed, since lots of the swap
//		       code has been removed.  BCG  FIXME SOMEDAY

#include <sys/param.h>		/*  For things in sys/conf.h.  */
#include <sys/conf.h>		/*  For struct swdevt.  */
#ifdef XOSVIEW_FREEBSD
# ifndef USE_KVM_GETSWAPINFO
#include <sys/rlist.h>
# endif
#else
#include <sys/map.h>		/*  For struct mapent.  */
#endif

#ifdef USE_KVM_GETSWAPINFO
#include <unistd.h>		/*  For getpagesize().  */
#endif

#include <kvm.h>		/*  For all sorts of stuff.  */
#ifndef HAVE_SWAPCTL
# include <stdio.h>		/*  For error messages.  */
#endif
#include <stdlib.h>		/*  For malloc().  */
#include <string.h>		/*  For bzero().  */

#ifdef XOSVIEW_BSDI      /* some more include files */
#include <err.h>
#include <vm/swap_pager.h>
#include <nlist.h>
#include <stdio.h>
#include <unistd.h>
#endif


extern char *getbsize __P((int *headerlenp, long *printoutblocksizep));

//-----------------------  We use a single kd for kvm access,
//			   initialized in kernel.cc, so the local one
//			   (swap_kd) has been commented out, and a
//			   #define has been added to make future
//			   references to swap_kd look like kd.  BCG
/*static kvm_t   *swap_kd;*/
#define swap_kd kd
extern kvm_t*	swap_kd;

#ifdef USE_KVM_GETSWAPINFO
static struct kvm_swap kvmsw[16];
static int kvnsw;
static int pagesize;
#else /* USE_KVM_GETSWAPINFO */
struct nlist syms[] = {
        { "_swdevt" },  /* list of swap devices and sizes */
#define VM_SWDEVT       0
#ifdef XOSVIEW_BSDI
        { "_niswap" },
#define VM_NSWAP       1
        { "_niswdev" },
#define VM_NSWDEV      2
#else
        { "_nswap" },   /* size of largest swap device */
#define VM_NSWAP        1
        { "_nswdev" },  /* number of swap devices */
#define VM_NSWDEV       2
#endif
        { "_dmmax" },   /* maximum size of a swap block */
#define VM_DMMAX        3
#ifdef XOSVIEW_FREEBSD
        { "_swaplist" },/* list of free swap areas */
#define VM_SWAPLIST     4
#else /* XOSVIEW_FREEBSD */
        { "_swapmap" }, /* list of free swap areas */
#define VM_SWAPMAP      4
#ifdef XOSVIEW_BSDI
        { "_swseq" },
#define VM_SWSEQ        5
        { "_swapstats" }, /* do we need to work on dead kernels? */
#define VM_SWAPSTATS    6
#else
        { "_nswapmap" },/* size of the swap map */
#define VM_NSWAPMAP     5
#endif /* BSDI */
#endif /* XOSVIEW_FREEBSD */
        {0}		/* End-of-list (need {} to avoid gcc warning) */
};

#ifdef XOSVIEW_BSDI
static struct swapstats swapstats;
#endif
static int nswap, nswdev, dmmax;
static struct swdevt *sw;
static long *perdev;
#ifdef XOSVIEW_FREEBSD
static struct rlisthdr swaplist;
#else
#ifndef XOSVIEW_BSDI
static int nswapmap;
#endif
static struct map *swapmap, *kswapmap;
static struct mapent *mp;
#endif
static int nfree;

#define SVAR(var) __STRING(var) /* to force expansion */
#define KGET(idx, var) \
        KGET1(idx, &var, sizeof(var), SVAR(var))
#define KGET1(idx, p, s, msg) \
        KGET2(syms[idx].n_value, p, s, msg)
#define KGET2(addr, p, s, msg) \
        if (kvm_read(swap_kd, addr, p, s) != s) { \
                printf("cannot read %s: %s", msg, kvm_geterr(swap_kd)); \
                return (0); \
        }
#endif /* USE_KVM_GETSWAPINFO */

int
BSDInitSwapInfo()
{
#ifdef XOSVIEW_BSDI
        struct swdevt *swseq;
        int i;
#endif
        static int once = 0;
#ifdef USE_KVM_GETSWAPINFO
	char msgbuf[BUFSIZ];
	struct kvm_swap dummy;
 
	if (kvm_getswapinfo(kd, &dummy, 1, 0) < 0) {
		snprintf(msgbuf, sizeof(msgbuf),
		    "xosview: swap: kvm_getswapinfo failed");
	}
	pagesize = getpagesize();
#else /* USE_KVM_GETSWAPINFO */
        u_long ptr;

	(void) ptr;	//  Avoid gcc warnings.
        if (once)
                return (1);
        if (kvm_nlist(swap_kd, syms)) {
#ifndef HAVE_SWAPCTL
		int i;
		char msgbuf[BUFSIZ];

                strncpy(msgbuf, "xosview: swap: cannot find", BUFSIZ);
                for (i = 0; syms[i].n_name != NULL; i++) {
                        if (syms[i].n_value == 0) {
			  	strncat(msgbuf, " ", BUFSIZ-strlen(msgbuf));
			  	strncat(msgbuf, syms[i].n_name,
				    BUFSIZ-strlen(msgbuf));
                        }
                }
                printf("%s\n", msgbuf);
#endif
                return (0);
        }
#ifdef XOSVIEW_BSDI
        KGET(VM_SWAPSTATS, swapstats);
#endif
        KGET(VM_NSWAP, nswap);
        KGET(VM_NSWDEV, nswdev);
        KGET(VM_DMMAX, dmmax);
#ifdef XOSVIEW_FREEBSD
        if ((sw = (struct swdevt*) malloc(nswdev * sizeof(*sw))) == NULL ||
            (perdev = (long*) malloc(nswdev * sizeof(*perdev))) == NULL) {
                printf("xosview: swap: malloc returned NULL.\n"  
		  "Number of Swap devices (nswdef) looked like %d\n", nswdev);
                return (0);
        }
	KGET1(VM_SWDEVT, &ptr, sizeof ptr, "swdevt");
	KGET2(ptr, sw, (signed) (nswdev * sizeof(*sw)), "*swdevt");
#else /* XOSVIEW_FREEBSD */
#ifndef XOSVIEW_BSDI
        KGET(VM_NSWAPMAP, nswapmap);
#endif
        KGET(VM_SWAPMAP, kswapmap);     /* kernel `swapmap' is a pointer */
#ifdef XOSVIEW_BSDI
        if ((sw = (struct swdevt*) malloc(swapstats.swap_nswdev * sizeof(*sw)))
	    == NULL ||
            (perdev = (long*) malloc(swapstats.swap_nswdev * sizeof(*perdev)))
	    == NULL ||
            (mp =(struct mapent*) malloc(swapstats.swap_mapsize * sizeof(*mp)))
	    == NULL) {
#else
        if ((sw = (struct swdevt*) malloc(nswdev * sizeof(*sw))) == NULL ||
            (perdev = (long*) malloc(nswdev * sizeof(*perdev))) == NULL ||
            (mp = (struct mapent*) malloc(nswapmap * sizeof(*mp))) == NULL) {
#endif
                printf("xosview: swap: malloc returned NULL.\n"  
		  "Number of Swap devices (nswdef) looked like %d\n", nswdev);
                return (0);
        }
        KGET1(VM_SWDEVT, sw, (signed) (nswdev * sizeof(*sw)), "swdevt");
#ifdef XOSVIEW_BSDI
        if (swapstats.swap_nswdev > nswdev) {
                /* The rest were allocated individually. */
                KGET(VM_SWSEQ, swseq);
                for (i = nswdev; i < swapstats.swap_nswdev; i++) {
                        KGET2((u_long)swseq, &sw[i], sizeof(sw[i]), "swseq");
                        swseq = sw[i].sw_next;
                }
        }
#endif
#endif /* XOSVIEW_FREEBSD */
#endif /* USE_KVM_GETSWAPINFO */
        once = 1;
        return (1);
}

#ifdef XOSVIEW_FREEBSD
/* Taken verbatim from /usr/src/usr.bin/systat/swap.c (pavel 24-Jan-1998) */
int
fetchswap()
{
#ifndef USE_KVM_GETSWAPINFO
	struct rlist head;
	struct rlist *swapptr;

	/* Count up swap space. */
	nfree = 0;
	memset(perdev, 0, nswdev * sizeof(*perdev));
	KGET1(VM_SWAPLIST, &swaplist, sizeof swaplist, "swaplist");
	swapptr = swaplist.rlh_list;
	while (swapptr) {
		int	top, bottom, next_block;

		KGET2((unsigned long)swapptr, &head,
		      sizeof(struct rlist), "swapptr");

		top = head.rl_end;
		bottom = head.rl_start;

		nfree += top - bottom + 1;

		/*
		 * Swap space is split up among the configured disks.
		 *
		 * For interleaved swap devices, the first dmmax blocks
		 * of swap space some from the first disk, the next dmmax
		 * blocks from the next, and so on up to nswap blocks.
		 *
		 * The list of free space joins adjacent free blocks,
		 * ignoring device boundries.  If we want to keep track
		 * of this information per device, we'll just have to
		 * extract it ourselves.
		 */
		while (top / dmmax != bottom / dmmax) {
			next_block = ((bottom + dmmax) / dmmax);
			perdev[(bottom / dmmax) % nswdev] +=
				next_block * dmmax - bottom;
			bottom = next_block * dmmax;
		}
		perdev[(bottom / dmmax) % nswdev] +=
			top - bottom + 1;

		swapptr = head.rl_next;
	}
#else /* USE_KVM_GETSWAPINFO */
	kvnsw = kvm_getswapinfo(kd, kvmsw, 1, 0);
#endif /* USE_KVM_GETSWAPINFO */
	return 0;

}
#else /* XOSVIEW_FREEBSD */
void
fetchswap()
{
        int s, e, i=0;
        int elast;
	struct mapent* localmp;
#ifdef XOSVIEW_BSDI
	long  siz;
        struct swdevt *sp;
#endif

	localmp = mp;
#ifdef XOSVIEW_BSDI
        s= swapstats.swap_mapsize * sizeof(*mp);
#else
        s = nswapmap * sizeof(*localmp);
#endif
        if (kvm_read(swap_kd, (long)kswapmap, localmp, s) != s)
                printf("cannot read swapmap: %s", kvm_geterr(swap_kd));

        /* first entry in map is `struct map'; rest are mapent's */
        swapmap = (struct map *)localmp;
	if (!swapmap)
	{
	  fprintf(stderr, "Error:  swapmap appears to be %p.  Did you\n"
#ifdef XOSVIEW_BSDI
                  "specify the correct kernel via -N, if not running /bsd?\n",
#else
	    "specify the correct kernel via -N, if not running /netbsd?\n",
#endif
	    swapmap);
	  exit(1);
	}
#ifdef XOSVIEW_BSDI
        if (swapstats.swap_mapsize !=
            (unsigned)(swapmap->m_limit + 1 - (struct mapent *)kswapmap))
                err(-1,"panic: swap: swapstats.swap_mapsize goof");
#else
        if (nswapmap != swapmap->m_limit - (struct mapent *)kswapmap)
                printf("panic: swap: nswapmap goof");
#endif

        /*
         * Count up swap space.
         */
        nfree = 0;
        elast = 0;
#ifdef XOSVIEW_BSDI
        bzero(perdev, swapstats.swap_nswdev * sizeof(*perdev));
#else
        bzero(perdev, nswdev * sizeof(*perdev));
#endif
#ifdef XOSVIEW_BSDI
        for (localmp++; (unsigned)(siz = localmp->m_size) != 0xffffffff; localmp++) {
#else
        for (localmp++; localmp->m_addr != 0; localmp++) {
#endif
                s = localmp->m_addr;                 /* start of swap region */
                e = localmp->m_addr + localmp->m_size;    /* end of region */
                elast = e;
                nfree += localmp->m_size;

#ifdef XOSVIEW_BSDI
/* This code mimics swstrategy(). */
                if (swapstats.swap_nswdev > 1) {
                        if (s < nswap) {
                                if (nswdev > 1)
                                        i = (s / dmmax / 2) % nswdev;
                                else
                                        i = 0;
                                if (sw[i].sw_flags & SW_SEQUENTIAL)
                                        err(-1,"panic: swap: interlv/seq 1");
                        } else {
                                s -= nswap;
                                for (sp = &sw[i = nswdev];
                                     i < swapstats.swap_nswdev; sp++, i++) {
                                        if (s <= sp->sw_nblks)
                                                break;
                                        s -= sp->sw_nblks;
                                }
                                if ((sw[i].sw_flags & SW_SEQUENTIAL) == 0)
                                        err(-1,"panic: swap: interlv/seq 2");
                        }
                } else
                        i = 0;

                perdev[i] += siz;
#else
                /*
                 * Swap space is split up among the configured disks.
                 * The first dmmax blocks of swap space some from the
                 * first disk, the next dmmax blocks from the next,
                 * and so on.  The list of free space joins adjacent
                 * free blocks, ignoring device boundries.  If we want
                 * to keep track of this information per device, we'll
                 * just have to extract it ourselves.
                 */

                /* calculate first device on which this falls */
                i = (s / dmmax) % nswdev;
                while (s < e) {         /* XXX this is inefficient */
                        int bound = roundup(s + 1, dmmax);

                        if (bound > e)
                                bound = e;
                        perdev[i] += bound - s;
                        if (++i >= nswdev)
                                i = 0;
                        s = bound;
                }
#endif
        }
}
#endif /* XOSVIEW_FREEBSD */

void
BSDGetSwapInfo(int* total, int* free)
{
        int i, avail, npfree, used=0, xsize, xfree;

	fetchswap();
#ifdef USE_KVM_GETSWAPINFO
	avail = used = 0;
	if (kvnsw == 0) {
		avail += pagesize * kvmsw[0].ksw_total;
		used += pagesize * kvmsw[0].ksw_used;
	}
	*total = avail;
	*free = avail - used;
#else /* USE_KVM_GETSWAPINFO */
        avail = npfree = 0;
#ifdef XOSVIEW_BSDI
        for (i = 0; i < swapstats.swap_nswdev; i++) {
#else
        for (i = 0; i < nswdev; i++) {
#endif
                /*
                 * Don't report statistics for partitions which have not
                 * yet been activated via swapon(8).
                 */
                if (!sw[i].sw_freed) {
			/* -----  Originally, this printed a
			 * warning.  However, for xosview, we
			 * don't want the warning printed.
			 * bgrayson  */
                        continue;
                }
#ifdef XOSVIEW_FREEBSD
                /*
                 * The first dmmax is never allocated to avoid trashing of
                 * disklabels
                 */
                /*xsize = sw[i].sw_nblks - dmmax;*/
		/*  Actually, count those dmmax blocks -- pstat,
		 *  top, etc. do.  It is swap space that is not
		 *  free for use.  bgrayson, on suggestion from
		 *  Andrew Sharp.  */
                xsize = sw[i].sw_nblks;
#else
                xsize = sw[i].sw_nblks;
#endif /* XOSVIEW_FREEBSD */
		xfree = perdev[i];
                used = xsize - xfree;
                npfree++;
                avail += xsize;
        }
        /*
         * If only one partition has been set up via swapon(8), we don't
         * need to bother with totals.
         */
        if (npfree > 1) {
                used = avail - nfree;
        }
	  /*  Convert from 512-byte blocks to bytes.  */
        *total = 512*avail;
        *free = 512*(avail-used);
#endif /* USE_KVM_GETSWAPINFO */
}

#endif
