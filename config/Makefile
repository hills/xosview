all: ../configure

../configure: configure.in aclocal.m4
	autoconf configure.in > ../configure
	chmod 755 ../configure

clean:
	rm -f *~
