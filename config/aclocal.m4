dnl
dnl This file containes a macro for each os xosview has been ported to.
dnl Each macro can add specific config options that apply to only that
dnl specific port.
dnl
dnl $Id$
dnl

AC_DEFUN(AC_XOSV_LINUX, [

EXTRA_OUT_FILES="$EXTRA_OUT_FILES \
  linux/memstat/Makefile:config/Makefile.linux.memstat.in"

dnl
dnl Define GNULIBC for the new GNU libc for linux
dnl
if test "$host_os" == "linux-gnulibc2"; then
AC_DEFINE(GNULIBC)
echo "GNULIBC"
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
        MEMSTAT=MemStat
        echo "enabled  the Linux memstat module"

dnl
dnl If this module is to be built then check to see if we can
dnl use MODVERSIONS.
dnl
AC_CHECK_HEADER(linux/modversions.h, [USE_MOD_VERSIONS=-DMODVERSIONS])

fi
,
MEMSTAT=
echo "disabled Linux memstat module by default"
dnl
dnl If this module is to be built then check to see if we can
dnl use MODVERSIONS.
dnl
AC_CHECK_HEADER(linux/modversions.h, [USE_MOD_VERSIONS=-DMODVERSIONS])

)
AC_SUBST(MEMSTAT)

INSTALL_ARGS='-s -m 4755'
])


AC_DEFUN(AC_XOSV_NETBSD, [
dnl  We need to strip the version numbers off the $host_os string (netbsd1.1)
dnl  Let's just be lazy -- set host_os to be netbsd.  
	host_os=netbsd
dnl
dnl Netbsd needs to link with libkvm
dnl
        EXTRALIBS=-lkvm
        INSTALL_ARGS='-s -g kmem -m 02555'
	AC_DEFINE(XOSVIEW_NETBSD)
])
dnl  For now, make FreeBSD be a superset of NetBSD.
AC_DEFUN(AC_XOSV_FREEBSD, [
AC_XOSV_NETBSD
AC_DEFINE(XOSVIEW_FREEBSD)
])


AC_DEFUN(AC_XOSV_HPUX, [
dnl
dnl No special config options for HPUX.
dnl
])
