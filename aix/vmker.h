//
//  Copyright (c) 2026 by Antoni Sawicki ( as@tenoware.com )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _VMKER_H_
#define _VMKER_H_

#include <sys/types.h>

//  The AIX kernel exports its virtual memory counters through the "vmker"
//  symbol, but ships no header for it.  This layout was reverse engineered
//  by Jussi Maki for "monitor" and is the same one used by the AIX 4.1
//  module of top(1); the numperm field is named after vmtune.c.  It has been
//  verified on AIX 4.1.5: totalmem matches "lsattr -El sys0 -a realmem",
//  freemem tracks the vmstat "fre" column, and totalvmem / freevmem match
//  the paging space totals reported by lsps(1) and swapqry(2).

struct vmker {
    uint n0, n1, n2, n3, n4, n5, n6, n7, n8;
    uint totalmem;              /* real memory frames                     */
    uint badmem;                /* unusable frames (RS/6000 model 220)    */
    uint freemem;               /* free real memory frames                */
    uint n12;
    uint numperm;               /* persistent (file cache) pages          */
    uint totalvmem, freevmem;   /* paging space frames, total and free    */
    uint n15, n16, n17, n18, n19;
};

#endif
