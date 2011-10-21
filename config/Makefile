all: ../configure

../configure: configure.in aclocal.m4
	autoconf configure.in > ../configure
	chmod 755 ../configure

clean:
	rm -f *~

#  NOTE:  To update to a new version of autoconf, the config.sub,
#  config.guess, and install-sh scripts need to be copied manually
#  from the autoconf tarball.  (At least, the autoreconf -i flag
#  didn't seem to do the trick.)
#  Updated to autoconf-2.52 on Feb 17, 2001
#  - bgrayson
