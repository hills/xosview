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

AC_DEFUN(ICE_CXX_BOOL,
[
AC_REQUIRE([AC_PROG_CXX])
AC_MSG_CHECKING(whether ${CXX} supports bool types)
AC_CACHE_VAL(ice_cv_have_bool,
[
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
AC_TRY_COMPILE(,[bool b = true;],
ice_cv_have_bool=yes,
ice_cv_have_bool=no)
AC_LANG_RESTORE
])
AC_MSG_RESULT($ice_cv_have_bool)
if test "$ice_cv_have_bool" = yes; then
AC_DEFINE(HAVE_BOOL)
fi
])dnl

AC_DEFUN(ICE_CXX_LONG_LONG,
[
AC_REQUIRE([AC_PROG_CXX])
AC_MSG_CHECKING(whether ${CXX} supports long long types)
AC_CACHE_VAL(ice_cv_have_long_long,
[
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
AC_TRY_COMPILE(,[long long x; x = (long long)0;],
ice_cv_have_long_long=yes,
ice_cv_have_long_long=no)
AC_LANG_RESTORE
])
AC_MSG_RESULT($ice_cv_have_long_long)
if test "$ice_cv_have_long_long" = yes; then
AC_DEFINE(LONG_LONG,long long)
else
AC_DEFINE(LONG_LONG,long)
fi
])dnl

dnl	For gcc-based (or primarily-gcc) OS's, set EXTRA_CXXFLAGS to -Wall -O4 -pipe.
AC_DEFUN(AC_GCC_EXTRA_CXXFLAGS, [
	EXTRA_CXXFLAGS="-Wall -O4 -pipe"
])

AC_DEFUN(SMP_LINUX,
[
AC_MSG_CHECKING(for SMP)
AC_EGREP_CPP(yes,
[#include <linux/autoconf.h>
#ifdef CONFIG_SMP
yes
#endif
], smp=yes, smp=no)
AC_MSG_RESULT($smp)
])dnl


AC_DEFUN(AC_SYS_LINUX_VERS,[[
changequote(<<, >>)
<<
LVERSION=`uname -r`
LVERSION=`expr $LVERSION : '\([0-9]*\.[0-9]*\)'`
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
AC_MSG_CHECKING(for MODVERSIONS)
AC_EGREP_CPP(yes,
[#include <linux/config.h>
#ifdef CONFIG_MODVERSIONS
yes
#endif
], [USE_MOD_VERSIONS=-DMODVERSIONS] AC_MSG_RESULT(yes), AC_MSG_RESULT(no))
SMP_LINUX
INSTALL_ARGS='-s -m 4755'
fi
,
AC_SYS_LINUX_VERS
if test "$LVERSION" = "2.0" -o "$LVERSION" = "2.2"
then
        MEMSTAT=MemStat
        echo "enabled  the Linux $LVERSION memstat module by default"
dnl
dnl If this module is to be built then check to see if we can
dnl use MODVERSIONS.
dnl
dnl        AC_CHECK_HEADER(linux/modversions.h, [USE_MOD_VERSIONS=-DMODVERSIONS])
        AC_MSG_CHECKING(for MODVERSIONS)
        AC_EGREP_CPP(yes,
[#include <linux/config.h>
#ifdef CONFIG_MODVERSIONS
yes
#endif
], [USE_MOD_VERSIONS=-DMODVERSIONS] AC_MSG_RESULT(yes), AC_MSG_RESULT(no))
        SMP_LINUX
else
        MEMSTAT=
        echo "disabled the Linux $LVERSION memstat module by default"
fi
)
INSTALL_ARGS='-s -m 4755'
if test "$smp" = "yes"
then
LINUX_SMP="-D__SMP__"
fi
AC_SUBST(LINUX_SMP)
]
	NetMeter_Default_Setting=False
)


dnl  ***  Below this line are the *BSD/BSDI and HPUX macros.  ***

dnl  ***  This one isn't actually used yet.  - bgrayson  ***
AC_DEFUN(AC_XOSV_BSD_COMMON, [
dnl  The BSD versions need to link with libkvm, and have the BSD install flags.
	EXTRALIBS="-lkvm $XPMLIB"
	INSTALL_ARGS='-s -g kmem -m 02555'
])

AC_DEFUN(AC_XOSV_NETBSD, [
dnl  Remember the full version in host_os_full
	host_os_full=$host_os
dnl  We need to strip the version numbers off the $host_os string (netbsd1.1)
dnl  Let's just be lazy -- set host_os to be netbsd.  
	host_os=netbsd
dnl
dnl Netbsd needs to link with libkvm
dnl
        EXTRALIBS="-lkvm $XPMLIB"
        INSTALL_ARGS='-s -g kmem -m 02555'
	NetMeter_Default_Setting=True
	AC_DEFINE(XOSVIEW_NETBSD)
])

AC_DEFUN(AC_XOSV_FREEBSD, [
dnl
dnl FreeBSD also needs to link with libkvm
dnl
        EXTRALIBS="-lkvm $XPMLIB $DEVSTATLIB"
        INSTALL_ARGS='-s -g kmem -m 02555'
	NetMeter_Default_Setting=True
	AC_DEFINE(XOSVIEW_FREEBSD)
])

AC_DEFUN(AC_XOSV_OPENBSD, [
dnl
dnl OpenBSD also needs to link with libkvm
dnl
        EXTRALIBS="-lkvm $XPMLIB"
        INSTALL_ARGS='-s -g kmem -m 02555'
	NetMeter_Default_Setting=True
	AC_DEFINE(XOSVIEW_OPENBSD)
])

AC_DEFUN(AC_XOSV_BSDI, [
dnl
dnl BSDI (surprise, surprise) also needs to link with libkvm
dnl BSDI before 4.0 should probably have CXX=shlicc++ too so use
dnl gmake CXX=shlicc++ on bsdi [23].x
dnl
	EXTRALIBS="-lkvm $XPMLIB"
	INSTALL_ARGS='-s -g kmem -m 02555'
	NetMeter_Default_Setting=True
	AC_DEFINE(XOSVIEW_BSDI)
])

AC_DEFUN(AC_XOSV_HPUX, [
dnl
dnl No special config options for HPUX.
dnl
])

AC_DEFUN(AC_XOSV_IRIX65, [
	dnl	
	EXTRALIBS="-lrpcsvc"
    AC_DEFINE(_G_HAVE_BOOL)
    AC_DEFINE(HAVE_SNPRINTF)
])
