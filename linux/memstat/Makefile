CC     = gcc
CFLAGS = -D__KERNEL__ -I/usr/include/linux -Wall -Wstrict-prototypes \
         -O2 -fomit-frame-pointer -pipe -DMODULE
FEXT   = c


OBJS = \
memstat.o

DEPFILES := $(OBJS:%=.%.d)
CFILES := $(OBJS:.o=.$(FEXT))

#-----------------------------------------------------------------------

all: memstat.o

#Memstat.o: memstat.o
#	ld -m elf_i386 -r -o Memstat.o memstat.o

clean :
	rm -f $(DEPFILES) $(EXE) $(OBJS) *~ TAGS Memstat.o

tags TAGS :
	etags $(CFILES) *.h

%.o : %.$(FEXT)
	$(CC) $(CFLAGS) -c $< -o $@

.%.o.d : %.$(FEXT)
	@echo building dependencies for $<
	@$(SHELL) -ec '$(CC) -M $(CFLAGS) $< \
          | sed '\''s/$*.o/$*.o $@/'\'' > $@'

-include $(DEPFILES)
