# Generated automatically from Makefile.sunos5.in by configure.
#
# HPUX Makefile template
#
#
# $Id$
#
include ../Makefile.config

##  Autodepend rule defaults to disabled.
#-include ../Makefile.GNU.autodep

CXXFLAGS += -I..

OBJS = \
MeterMaker.o \
loadmeter.o \
cpumeter.o \
memmeter.o \
pagemeter.o \
swapmeter.o


CFILES := $(OBJS:.o=.cc)
DEPFILES := $(OBJS:%=.%.d)

vpath %.h $(TOP)
vpath %.cc $(TOP)

#-----------------------------------------------------------------------

all : libmeter.a

clean :
	rm -f $(OBJS) $(DEPFILES) libmeter.a *~

libmeter.a : $(OBJS)
	ar cvr libmeter.a $(OBJS)
	ranlib $@


#
# HPUX Makefile template
#
#
# $Id$
#
include ../Makefile.config

##  Autodepend rule defaults to disabled.
#-include ../Makefile.GNU.autodep

CXXFLAGS += -I..

OBJS = \
MeterMaker.o \
loadmeter.o \
cpumeter.o \
memmeter.o \
pagemeter.o \
swapmeter.o


CFILES := $(OBJS:.o=.cc)
DEPFILES := $(OBJS:%=.%.d)

vpath %.h $(TOP)
vpath %.cc $(TOP)

#-----------------------------------------------------------------------

all : libmeter.a

clean :
	rm -f $(OBJS) $(DEPFILES) libmeter.a *~

libmeter.a : $(OBJS)
	ar cvr libmeter.a $(OBJS)
	ranlib $@


