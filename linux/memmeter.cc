//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//  Copyright (c) 2015 Framestore
//
//  This file may be distributed under terms of the GPL
//

#include "memmeter.h"
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MEMFILENAME "/proc/meminfo"

#define KB(x) ((double)((x) * 1024))


MemMeter::MemMeter( XOSView *parent )
: FieldMeterGraph( parent, 6, "MEM", "USED/BUFF/SLAB/MAP/CACHE/FREE" ){
}

void MemMeter::checkResources( void ){
  FieldMeterGraph::checkResources();

  setfieldcolor( 0, parent_->getResource( "memUsedColor" ) );
  setfieldcolor( 1, parent_->getResource( "memBufferColor" ) );
  setfieldcolor( 2, parent_->getResource( "memSlabColor" ) );
  setfieldcolor( 3, parent_->getResource( "memMapColor" ) );
  setfieldcolor( 4, parent_->getResource( "memCacheColor" ) );
  setfieldcolor( 5, parent_->getResource( "memFreeColor" ) );
  priority_ = atoi (parent_->getResource( "memPriority" ) );
  dodecay_ = parent_->isResourceTrue( "memDecay" );
  useGraph_ = parent_->isResourceTrue( "memGraph" );
  SetUsedFormat (parent_->getResource("memUsedFormat"));
}

void MemMeter::checkevent( void ){
  getstats();
  drawfields();
}

void MemMeter::getstats() {
  FILE *f;

  f = fopen(MEMFILENAME, "r");
  if (!f) {
    perror(MEMFILENAME);
    exit(1);
  }

  long mem_total = 0,
    mem_free = 0,
    buffers = 0,
    slab = 0,
    mapped = 0,
    cached = 0;

  for (;;) {
    char line[128];
    char *c, *endptr;
    long unsigned kb = 0;

    /*
     * Parse lines in the format: "FieldName:      12345678 kB"
     *
     * We prefer to not use scanf because it's harder with variable
     * number of fields; the 'kB' is not present if value is 0
     */

    if (!fgets(line, sizeof line, f))
      break;

    c = strchr(line, ':');
    if (!c) {
      fprintf(stderr, MEMFILENAME ": parse error, ':' expected at '%s'\n", line);
      exit(1);
    }

    *c = '\0';
    c++;

    kb = strtoul(c, &endptr, 10);
    if (kb == ULONG_MAX) {
      fprintf(stderr, MEMFILENAME ": parse error, '%s' is out of range\n", c);
      exit(1);
    }

    if (strcmp(line, "MemTotal") == 0)
      mem_total = kb;
    else if (strcmp(line, "MemFree") == 0)
      mem_free = kb;
    else if (strcmp(line, "Buffers") == 0)
      buffers = kb;
    else if (strcmp(line, "Cached") == 0)
      cached = kb;
    else if (strcmp(line, "Slab") == 0)
      slab = kb;
    else if (strcmp(line, "Mapped") == 0)
      mapped = kb;
  }

  if (fclose(f) != 0)
    abort();

  /* Don't do arithmetic on the fields_ themselves; these are floating
   * point and when memory is large are affected by inaccuracy */

  fields_[1] = KB(buffers);
  fields_[2] = KB(slab);
  fields_[3] = KB(mapped);
  fields_[4] = KB(cached);
  fields_[5] = KB(mem_free);

  fields_[0] =
	KB(mem_total - mem_free - buffers - mapped - cached - slab);
  total_ =     KB(mem_total);

  setUsed(KB(mem_total - mem_free), KB(mem_total));
}
