#ifndef snprintf_h
#define snprintf_h

#ifndef HAVE_SNPRINTF
extern "C" int snprintf ( char *str, int n, const char *format, ...);
#endif

#endif
