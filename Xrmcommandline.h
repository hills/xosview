#ifndef _Xrmcommandline_h
#define _Xrmcommandline_h

#include <X11/Xresource.h>

static XrmOptionDescRec options[] = {
//  For these options, try to use '+' to turn them on, and '-' to turn them
//    off, even though this is different from the usual tradition of -foo
//    turning on 'foo', which is off by default.  At least this way,
//    it is self-consistent, and self-explanatory.

//  General, X11 resources:
{ "-display", "*display", XrmoptionSepArg, (caddr_t) NULL },
{ "-font", "*font", XrmoptionSepArg, (caddr_t) NULL },
{ "-title", "*title", XrmoptionSepArg, (caddr_t) NULL },
{ "-geometry", "*geometry", XrmoptionSepArg, (caddr_t) NULL },

//  XOSView-specific resources:
{ "-labels", "*labels", XrmoptionNoArg, "False" },
{ "+labels", "*labels", XrmoptionNoArg, "True" },
{ "-load", "*load", XrmoptionNoArg, "False" },
{ "+load", "*load", XrmoptionNoArg, "True" },
//  FIXME BCG  The network option needs to be changed -- it is overloaded
//  more than it should be.  It is both whether or not a netmeter should be
//  shown, and the maximum bandwidth of the network.  A 'xosview.net'
//  resource exists for the sole purpose of turning on or off the
//  netmeter already.
{ "-network", "*network", XrmoptionSepArg, (caddr_t) NULL },
//  -net is an abbreviation for -network
{ "-net", "*network", XrmoptionSepArg, (caddr_t) NULL },

//  Serial Meter Options
{ "+serial1", "*serial1", XrmoptionNoArg, "True" },
{ "-serial1", "*serial1", XrmoptionNoArg, "False" },
{ "+serial2", "*serial2", XrmoptionNoArg, "True" },
{ "-serial2", "*serial2", XrmoptionNoArg, "False" },
{ "+serial3", "*serial3", XrmoptionNoArg, "True" },
{ "-serial3", "*serial3", XrmoptionNoArg, "False" },
{ "+serial4", "*serial4", XrmoptionNoArg, "True" },
{ "-serial4", "*serial4", XrmoptionNoArg, "False" },

//  Special, catch-all option here --
//    xosview -xrm "*memFreeColor: purple" should work.
{ "-xrm", "*xrm", XrmoptionResArg, (caddr_t) NULL },
};
//  This auto-detects changes in the number of options.
static const int NUM_OPTIONS = sizeof(options) / sizeof(options[0]);




#endif
