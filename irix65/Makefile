# Generated automatically from Makefile.irix65.in by configure.
#
# Irix65 Makefile template
#
#
#
include ../Makefile.config

##  Autodepend rule defaults to disabled.
#-include ../Makefile.GNU.autodep

CXXFLAGS += -I..

OBJS = \
MeterMaker.o \
loadmeter.o \
cpumeter.o \
memmeter.o 
#pagemeter.o \
#swapmeter.o


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


