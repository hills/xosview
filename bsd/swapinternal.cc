//  Copyright (c) 1995 by Brian Grayson (bgrayson@pine.ece.utexas.edu)
//
//  This code is borrowed HEAVILY from the vmstat source code in the
//  NetBSD distribution.  As such, the NetBSD copyright claim/disclaimer
//  applies to most of this code.  The disclaimer, along with the CVS
//  header from the version from which this file was created, are included
//  below:

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

//---------------------  For xosview, we need to include <err.h>.  BCG
#include <err.h>

//---------------------  The remainder of this file is based/borrowed
//                       from /usr/src/bin/systat/swap.c in the NetBSD
//                       distribution.  BCG

/*
 * swapinfo - based on a program of the same name by Kevin Lahey
 */

//---------------------  Note:  all of these includes were in the
//		       original source code.  I am leaving them
//		       undisturbed, although it is likely that
//		       some may be removed, since lots of the swap
//		       code has been removed.  BCG  FIXME SOMEDAY

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/map.h>
#include <sys/stat.h>

#include <kvm.h>
#include <nlist.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



static long netbsdSwapBlockSize;

#include <sys/cdefs.h>
#include <fcntl.h>

extern char *getbsize __P((int *headerlenp, long *printoutblocksizep));

static kvm_t   *swap_kd;

struct nlist syms[] = {
        { "_swapmap" }, /* list of free swap areas */
#define VM_SWAPMAP      0
        { "_nswapmap" },/* size of the swap map */
#define VM_NSWAPMAP     1
        { "_swdevt" },  /* list of swap devices and sizes */
#define VM_SWDEVT       2
        { "_nswap" },   /* size of largest swap device */
#define VM_NSWAP        3
        { "_nswdev" },  /* number of swap devices */
#define VM_NSWDEV       4
        { "_dmmax" },   /* maximum size of a swap block */
#define VM_DMMAX        5
        {0}		/* End-of-list (need {} to avoid gcc warning) */
};

static int nswap, nswdev, dmmax, nswapmap;
static struct swdevt *sw;
static long *perdev;
static struct map *swapmap, *kswapmap;
static struct mapent *mp;
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

int
NetBSDInitSwapInfo()
{
        int i;
        char msgbuf[BUFSIZ];
        static int once = 0;
	int stringlen;

	getbsize (&stringlen, &netbsdSwapBlockSize);
        swap_kd = kvm_open (NULL, NULL, NULL, O_RDONLY, NULL);
	if (!swap_kd)
	  err (-1, "NetBSDInitSwapInfo(): kvm_open()");
        if (once)
                return (1);
        if (kvm_nlist(swap_kd, syms)) {
                strcpy(msgbuf, "systat: swap: cannot find");
                for (i = 0; syms[i].n_name != NULL; i++) {
                        if (syms[i].n_value == 0) {
                                strcat(msgbuf, " ");
                                strcat(msgbuf, syms[i].n_name);
                        }
                }
                printf(msgbuf);
                return (0);
        }
        KGET(VM_NSWAP, nswap);
        KGET(VM_NSWDEV, nswdev);
        KGET(VM_DMMAX, dmmax);
        KGET(VM_NSWAPMAP, nswapmap);
        KGET(VM_SWAPMAP, kswapmap);     /* kernel `swapmap' is a pointer */
        if ((sw = (struct swdevt*) malloc(nswdev * sizeof(*sw))) == NULL ||
            (perdev = (long*) malloc(nswdev * sizeof(*perdev))) == NULL ||
            (mp = (struct mapent*) malloc(nswapmap * sizeof(*mp))) == NULL) {
                printf("swap malloc");
                return (0);
        }
        KGET1(VM_SWDEVT, sw, (signed) (nswdev * sizeof(*sw)), "swdevt");
        once = 1;
        return (1);
}

void
fetchswap()
{
        int s, e, i;
        int elast;
	struct mapent* localmp;

	localmp = mp;
        s = nswapmap * sizeof(*localmp);
        if (kvm_read(swap_kd, (long)kswapmap, localmp, s) != s)
                printf("cannot read swapmap: %s", kvm_geterr(swap_kd));

        /* first entry in map is `struct map'; rest are mapent's */
        swapmap = (struct map *)localmp;
        if (nswapmap != swapmap->m_limit - (struct mapent *)kswapmap)
                printf("panic: swap: nswapmap goof");

        /*
         * Count up swap space.
         */
        nfree = 0;
        elast = 0;
        bzero(perdev, nswdev * sizeof(*perdev));
        for (localmp++; localmp->m_addr != 0; localmp++) {
                s = localmp->m_addr;                 /* start of swap region */
                e = localmp->m_addr + localmp->m_size;    /* end of region */
                elast = e;
                nfree += localmp->m_size;

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
        }
}

void
NetBSDGetSwapInfo(int* total, int* free)
{
        int div, i, avail, npfree, used=0, xsize, xfree;

	fetchswap();
        div = netbsdSwapBlockSize / 512;
        avail = npfree = 0;
        for (i = 0; i < nswdev; i++) {
                /*
                 * Don't report statistics for partitions which have not
                 * yet been activated via swapon(8).
                 */
                if (!sw[i].sw_freed) {
                        printf ("Device %d not available for swapping.\n",
                        i);
                        continue;
                }
                xsize = sw[i].sw_nblks;
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
        *total = avail;
        *free = avail-used;
}
