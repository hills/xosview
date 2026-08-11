//
//  Copyright (c) 2026 by Antoni Sawicki ( as@tenoware.com )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _AIXCOMPAT_H_
#define _AIXCOMPAT_H_

//  AIX 4.1 ships libc routines that its headers predate: snprintf and
//  vsnprintf are in libc.a but appear in no header, and the strcasecmp
//  family lives in <strings.h> rather than <string.h>.  The Makefile force
//  includes this into every translation unit so that the shared xosview
//  sources need no AIX specific conditionals.

#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>
#include <strings.h>

extern "C" {
  int snprintf( char *, size_t, const char *, ... );
  int vsnprintf( char *, size_t, const char *, va_list );
}

#endif
