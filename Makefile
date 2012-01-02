-include .config

INSTALL = install

CPPFLAGS += -I. -DHAVE_SNPRINTF=1 -DHAVE_BOOL=1
LDLIBS += -lX11

# Installation paths

BINDIR = $(PREFIX)/bin
EXECDIR = $(PREFIX)/libexec
MANDIR = $(PREFIX)/share/man
DOCDIR = $(PREFIX)/share/doc

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
	snprintf.o \
	xosview.o \
	xwin.o

# Optional platform type

ifdef LINUX
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
	linux/wirelessmeter.o
CPPFLAGS += -Ilinux/
endif

ifdef BSD
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
        bsd/swapinternal.o \
        bsd/swapmeter.o
CPPFLAGS += -Ibsd/
endif

ifdef IRIX65
OBJS += irix65/MeterMaker.o \
        irix65/cpumeter.o \
        irix65/diskmeter.o \
        irix65/gfxmeter.o \
        irix65/loadmeter.o \
        irix65/memmeter.o \
        irix65/sarmeter.o
CPPFLAGS += -Iirix65/
endif

ifdef SUNOS5
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

.PHONY:		dist install clean

dist:
		./mkdist $(VERSION)

install:	xosview
		$(INSTALL) -Dm 755 xosview $(BINDIR)
		$(INSTALL) -Dm 644 xosview.1 $(MANDIR)/man1

clean:
		rm -f xosview $(OBJS) $(DEPS)

-include $(DEPS)
