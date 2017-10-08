-include .config

AWK ?= awk
INSTALL ?= install
PLATFORM ?= linux

# Installation paths

PREFIX ?= /usr/local

BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man
XDGAPPSDIR ?= $(PREFIX)/share/applications
ICONDIR ?= $(PREFIX)/share/icons/hicolor

# Optional build arguments; user may wish to override

OPTFLAGS ?= -Wall -O3

# Required build arguments

CPPFLAGS += $(OPTFLAGS) -I. -MMD
LDLIBS += -lX11 -lXpm

OBJS = Host.o \
	Xrm.o \
	bitfieldmeter.o \
	bitmeter.o \
	defaultstring.o \
	fieldmeter.o \
	fieldmeterdecay.o \
	fieldmetergraph.o \
	llist.o \
	main.o \
	meter.o \
	xosview.o \
	xwin.o

# Optional platform type

ifeq ($(PLATFORM), linux)
ARCH = $(shell uname -m)
OBJS += sensorfieldmeter.o \
	linux/MeterMaker.o \
	linux/btrymeter.o \
	linux/cpumeter.o \
	linux/diskmeter.o \
	linux/intmeter.o \
	linux/intratemeter.o \
	linux/lmstemp.o \
	linux/loadmeter.o \
	linux/memmeter.o \
	linux/netmeter.o \
	linux/nfsmeter.o \
	linux/pagemeter.o \
	linux/raidmeter.o \
	linux/serialmeter.o \
	linux/swapmeter.o \
	linux/wirelessmeter.o \
	linux/acpitemp.o
ifeq ($(findstring 86,$(ARCH)),86)
OBJS += linux/coretemp.o
endif
CPPFLAGS += -Ilinux/
LDLIBS += -lm
endif

ifeq ($(PLATFORM), bsd)
ARCH = $(shell uname -m)
OBJS += sensorfieldmeter.o \
        bsd/MeterMaker.o \
        bsd/btrymeter.o \
        bsd/cpumeter.o \
        bsd/diskmeter.o \
        bsd/intmeter.o \
        bsd/intratemeter.o \
        bsd/kernel.o \
        bsd/loadmeter.o \
        bsd/memmeter.o \
        bsd/netmeter.o \
        bsd/pagemeter.o \
        bsd/swapmeter.o \
        bsd/sensor.o
ifeq ($(ARCH),$(filter $(ARCH),i386 amd64 x86_64))
OBJS += bsd/coretemp.o
endif
CPPFLAGS += -Ibsd/
LDLIBS += -lm
endif

ifeq ($(PLATFORM), irix65)
OBJS += irix65/MeterMaker.o \
        irix65/cpumeter.o \
        irix65/diskmeter.o \
        irix65/gfxmeter.o \
        irix65/loadmeter.o \
        irix65/memmeter.o \
        irix65/sarmeter.o
CPPFLAGS += -Iirix65/
endif

ifeq ($(PLATFORM), sunos5)
OBJS += sunos5/MeterMaker.o \
        sunos5/cpumeter.o \
        sunos5/diskmeter.o \
        sunos5/loadmeter.o \
        sunos5/memmeter.o \
        sunos5/netmeter.o \
        sunos5/pagemeter.o \
        sunos5/swapmeter.o \
        sunos5/intratemeter.o
CPPFLAGS += -Isunos5/ -Wno-write-strings
LDLIBS += -lkstat -lnsl -lsocket
INSTALL = ginstall
endif

ifeq ($(PLATFORM), gnu)
OBJS += gnu/get_def_pager.o \
	gnu/loadmeter.o \
	gnu/memmeter.o \
	gnu/MeterMaker.o \
	gnu/pagemeter.o \
	gnu/swapmeter.o
CPPFLAGS += -Ignu/
endif

DEPS := $(OBJS:.o=.d)

xosview:	$(OBJS)
		$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

defaultstring.cc:	Xdefaults defresources.awk
		$(AWK) -f defresources.awk Xdefaults > defaultstring.cc

Xrm.o:		CXXFLAGS += -Wno-write-strings

.PHONY:		dist install clean

dist:
		./mkdist $(VERSION)

install:	xosview
		$(INSTALL) -d $(DESTDIR)$(BINDIR)
		$(INSTALL) -d $(DESTDIR)$(MANDIR)/man1
		$(INSTALL) -d $(DESTDIR)$(XDGAPPSDIR)
		$(INSTALL) -d $(DESTDIR)$(ICONDIR)/32x32/apps
		$(INSTALL) -m 755 xosview $(DESTDIR)$(BINDIR)/xosview
		$(INSTALL) -m 644 xosview.1 $(DESTDIR)$(MANDIR)/man1/xosview.1
		$(INSTALL) -m 644 xosview.desktop $(DESTDIR)$(XDGAPPSDIR)
		$(INSTALL) -m 644 xosview.png $(DESTDIR)$(ICONDIR)/32x32/apps

clean:
		rm -f xosview $(OBJS) $(DEPS) defaultstring.cc

-include $(DEPS)
