#ifndef HAVE_SNPRINTF

#include <stdarg.h>
#include <stdio.h>

extern "C" int snprintf ( char *str, int n, const char *format, ...)
    {
    va_list ap;
    va_start(ap, format);
    int rval = vsprintf(str, format, ap);
    va_end(ap);
    return rval;
    }

#endif
