-include .config

INSTALL ?= install
PLATFORM ?= linux

# Installation paths

PREFIX ?= /usr/local

BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man

# Optional build arguments; user may wish to override

OPTFLAGS ?= -Wall -O3

# Required build arguments

CPPFLAGS += $(OPTFLAGS) -I.
LDLIBS += -lX11

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
OBJS += linux/MeterMaker.o \
	linux/btrymeter.o \
	linux/cpumeter.o \
	linux/diskmeter.o \
	linux/intmeter.o \
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
	linux/acpitemp.o \
	linux/coretemp.o
CPPFLAGS += -Ilinux/ -DHAVE_XPM -DHAVE_USLEEP
LDLIBS += -lXpm
endif

ifeq ($(PLATFORM), bsd)
OBJS += bsd/MeterMaker.o \
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
        bsd/coretemp.o \
        bsd/sensor.o
CPPFLAGS += -Ibsd/ -DHAVE_XPM -DHAVE_USLEEP
LDLIBS += -lXpm
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
        sunos5/swapmeter.o
CPPFLAGS += -Isunos5/
endif

DEPS := $(OBJS:.o=.d)

xosview:	$(OBJS)
		$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

defaultstring.cc:	Xdefaults defresources.awk
		gawk -f defresources.awk Xdefaults > defaultstring.cc

Xrm.o:		CXXFLAGS += -Wno-write-strings

.PHONY:		dist install clean

dist:
		./mkdist $(VERSION)

install:	xosview
		$(INSTALL) -Dm 755 xosview $(DESTDIR)$(BINDIR)/xosview
		$(INSTALL) -Dm 644 xosview.1 $(DESTDIR)$(MANDIR)/man1/xosview.1

clean:
		rm -f xosview $(OBJS) $(DEPS)

-include $(DEPS)
