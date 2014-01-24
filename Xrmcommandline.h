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
{ "-hmargin", "*horizontalMargin", XrmoptionSepArg, (caddr_t) NULL },
{ "-vmargin", "*verticalMargin", XrmoptionSepArg, (caddr_t) NULL },
{ "-vspacing", "*verticalSpacing", XrmoptionSepArg, (caddr_t) NULL },


//  XOSView-specific resources:
{ "-labels", "*labels", XrmoptionNoArg, "False" },
{ "+labels", "*labels", XrmoptionNoArg, "True" },
{ "-captions", "*captions", XrmoptionNoArg, "False" },
{ "+captions", "*captions", XrmoptionNoArg, "True" },
{ "-usedlabels", "*usedlabels", XrmoptionNoArg, "False" },
{ "+usedlabels", "*usedlabels", XrmoptionNoArg, "True" },
{ "-samplesPerSec", "*samplesPerSec", XrmoptionSepArg, (caddr_t) NULL },
//  CPU resources
{ "-cpu", "*cpu", XrmoptionNoArg, "False" },
{ "+cpu", "*cpu", XrmoptionNoArg, "True" },
{ "-cpus", "*cpuFormat", XrmoptionNoArg, "single" },
{ "+cpus", "*cpuFormat", XrmoptionNoArg, "all" },
// Load resources
{ "-load", "*load", XrmoptionNoArg, "False" },
{ "+load", "*load", XrmoptionNoArg, "True" },
// Memmeter resources
{ "-mem", "*mem", XrmoptionNoArg, "False" },
{ "+mem", "*mem", XrmoptionNoArg, "True" },
// Swapmeter resources
{ "-swap", "*swap", XrmoptionNoArg, "False" },
{ "+swap", "*swap", XrmoptionNoArg, "True" },
// Batterymeter resources
{ "-battery", "*battery", XrmoptionNoArg, "False" },
{ "+battery", "*battery", XrmoptionNoArg, "True" },
// Wirelessmeter resources
{ "-wireless", "*wireless", XrmoptionNoArg, "False" },
{ "+wireless", "*wireless", XrmoptionNoArg, "True" },
//  GFX resources
{ "-gfx", "*gfx", XrmoptionNoArg, "False" },
{ "+gfx", "*gfx", XrmoptionNoArg, "True" },

// Networkmeter resources
{ "-net", "*net", XrmoptionNoArg, "False" },
{ "+net", "*net", XrmoptionNoArg, "True" },
//  Previously, network was overloaded to be the bandwidth and the
//  on/off flag.  Now, we have -net for on/off, and networkBandwidth
//  for bandwidth, with the alias networkBW, and network for backwards
//  compatibility.
{ "-network", "*netBandwidth", XrmoptionSepArg, (caddr_t) NULL },
{ "-networkBW", "*netBandwidth", XrmoptionSepArg, (caddr_t) NULL },
{ "-networkBandwidth", "*netBandwidth", XrmoptionSepArg, (caddr_t) NULL },

// Page Meter
{ "-page", "*page", XrmoptionNoArg, "False" },
{ "+page", "*page", XrmoptionNoArg, "True" },
{ "-pagespeed", "*pageBandWidth", XrmoptionSepArg, (caddr_t) NULL },

#if !defined(__hpux__) && !defined(__hpux)
//  Disk Meter Options
{ "-disk", "*disk", XrmoptionNoArg, "False" },
{ "+disk", "*disk", XrmoptionNoArg, "True" },
#endif

// Interrupt meter resources  --  all sorts of aliases.
{ "-int", "*interrupts", XrmoptionNoArg, "False" },
{ "+int", "*interrupts", XrmoptionNoArg, "True" },
{ "-ints", "*interrupts", XrmoptionNoArg, "False" },
{ "+ints", "*interrupts", XrmoptionNoArg, "True" },
{ "-interrupts", "*interrupts", XrmoptionNoArg, "False" },
{ "+interrupts", "*interrupts", XrmoptionNoArg, "True" },

// Intrate meter resources, for platforms that support it.
{ "-irqrate", "*irqrate", XrmoptionNoArg, "False" },
{ "+irqrate", "*irqrate", XrmoptionNoArg, "True" },
{ "-intrate", "*irqrate", XrmoptionNoArg, "False" },
{ "+intrate", "*irqrate", XrmoptionNoArg, "True" },

// lmstemp resources
{ "-lmstemp", "*lmstemp", XrmoptionNoArg, "False" },
{ "+lmstemp", "*lmstemp", XrmoptionNoArg, "True" },
// coretemp resources
{ "-coretemp", "*coretemp", XrmoptionNoArg, "False" },
{ "+coretemp", "*coretemp", XrmoptionNoArg, "True" },
// acpitemp resources
{ "-acpitemp", "*acpitemp", XrmoptionNoArg, "False" },
{ "+acpitemp", "*acpitemp", XrmoptionNoArg, "True" },

//  Special, catch-all option here --
//    xosview -xrm "*memFreeColor: purple" should work, for example.
{ "-xrm", "*xrm", XrmoptionResArg, (caddr_t) NULL },
};
//  This auto-detects changes in the number of options.
static const int NUM_OPTIONS = sizeof(options) / sizeof(options[0]);

#endif
