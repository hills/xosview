dnl
dnl This file containes a macro for each os xosview has been ported to.
dnl Each macro can add specific config options that apply to only that
dnl specific port.
dnl
dnl $Id$
dnl

dnl Make an absolute symbol for the top of the configuration.
dnl
AC_DEFUN([CF_TOP_SRCDIR],
[TOP_SRCDIR=`cd $srcdir;pwd`
AC_SUBST(TOP_SRCDIR)
])dnl


AC_DEFUN(AC_SYS_LINUX_VERS,[[
changequote(<<, >>)
<<
LVERSION=`uname -r`
LVERSION=`expr $LVERSION : '\([0-9]*\.[0-9]*\)'`
if test "$LVERSION" = "2.2"
then
    LVERSION=2.1
fi
>>
changequote([, ])
]])

AC_DEFUN(AC_XOSV_LINUX, [
EXTRALIBS=$XPMLIB
EXTRA_OUT_FILES="$EXTRA_OUT_FILES \
  linux/memstat/Makefile:config/Makefile.linux.memstat.in"

dnl
dnl Define GNULIBC for the new GNU libc for linux
dnl
dnl Assume "linux-gnu" is GNU libc and linux-gnulibc1 is the old libc
dnl
if test "$host_os" = "linux-gnu"; then
AC_DEFINE(GNULIBC)
fi

dnl
dnl Add a switch to add -DUSESYSCALLS for linux.
dnl
AC_ARG_ENABLE([linux-syscalls],
[  --enable-linux-syscalls use system calls when possible],

if test "$enableval" = "no"
then
        echo "disabled Linux system calls"
else
        AC_DEFINE(USESYSCALLS)
        echo "enabled  Linux system calls"
fi
,
AC_DEFINE(USESYSCALLS)
echo "enabled  Linux system calls by default"
)

dnl
dnl Add a switch which will build the memstat kernel module
dnl
AC_ARG_ENABLE([linux-memstat],
[  --enable-linux-memstat  build the linux memstat kernel module],

if test "$enableval" = "no"
then
        MEMSTAT=
        echo "disabled the Linux memstat module"
else
        AC_SYS_LINUX_VERS
        MEMSTAT=MemStat
        echo "enabled  the Linux $LVERSION memstat module"

dnl
dnl If this module is to be built then check to see if we can
dnl use MODVERSIONS.
dnl
AC_CHECK_HEADER(linux/modversions.h, [USE_MOD_VERSIONS=-DMODVERSIONS])
INSTALL_ARGS='-s -m 4755'
fi
,
AC_SYS_LINUX_VERS
if test "$LVERSION" = "2.0"
then
        MEMSTAT=MemStat
        echo "enabled  the Linux $LVERSION memstat module by default"
dnl
dnl If this module is to be built then check to see if we can
dnl use MODVERSIONS.
dnl
        AC_CHECK_HEADER(linux/modversions.h, [USE_MOD_VERSIONS=-DMODVERSIONS])
else
        MEMSTAT=
        echo "disabled the Linux $LVERSION memstat module by default"
fi
)
INSTALL_ARGS='-s -m 4755'
])


AC_DEFUN(AC_XOSV_BSD_COMMON, [
dnl  The BSD versions need to link with libkvm, and have the BSD install flags.
	EXTRALIBS="-lkvm $XPMLIB"
	INSTALL_ARGS='-s -g kmem -m 02555'
])
AC_DEFUN(AC_XOSV_NETBSD, [
dnl  We need to strip the version numbers off the $host_os string (netbsd1.1)
dnl  Let's just be lazy -- set host_os to be netbsd.  
	host_os=netbsd
dnl
dnl Netbsd needs to link with libkvm
dnl
        EXTRALIBS="-lkvm $XPMLIB"
        INSTALL_ARGS='-s -g kmem -m 02555'
	AC_DEFINE(XOSVIEW_NETBSD)
])

AC_DEFUN(AC_XOSV_FREEBSD, [
dnl
dnl FreeBSD also needs to link with libkvm
dnl
        EXTRALIBS="-lkvm $XPMLIB $DEVSTATLIB"
        INSTALL_ARGS='-s -g kmem -m 02555'
	AC_DEFINE(XOSVIEW_FREEBSD)
])

AC_DEFUN(AC_XOSV_OPENBSD, [
dnl
dnl OpenBSD also needs to link with libkvm
dnl
        EXTRALIBS="-lkvm $XPMLIB"
        INSTALL_ARGS='-s -g kmem -m 02555'
	AC_DEFINE(XOSVIEW_OPENBSD)
])

AC_DEFUN(AC_XOSV_HPUX, [
dnl
dnl No special config options for HPUX.
dnl
])
