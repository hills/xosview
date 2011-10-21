dnl
dnl This file containes a macro for each os xosview has been ported to.
dnl Each macro can add specific config options that apply to only that
dnl specific port.
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

dnl	For gcc-based (or primarily-gcc) OS's, set EXTRA_CXXFLAGS to -Wall -O4
AC_DEFUN(AC_GCC_EXTRA_CXXFLAGS, [
	EXTRA_CXXFLAGS="-W -Wall -O3"
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

INSTALL_ARGS='-m 755'
]
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

AC_DEFUN(AC_XOSV_BSDI, [
dnl
dnl BSDI (surprise, surprise) also needs to link with libkvm
dnl BSDI before 4.0 should probably have CXX=shlicc++ too so use
dnl gmake CXX=shlicc++ on bsdi [23].x
dnl
	EXTRALIBS="-lkvm $XPMLIB"
	INSTALL_ARGS='-s -g kmem -m 02555'
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

AC_DEFUN(AC_XOSV_GNU, [
EXTRALIBS=$XPMLIB
])

dnl MY_C_SWITCH(switch)
dnl -------------------
dnl try to compile and link a simple C program with the switch compile switch
dnl "${CC-cc} $CFLAGS $1 conftest.c -o conftest"
dnl sets my_cc_switch to switch if it worked
dnl my_cc_switch is not modified elsewhere

AC_DEFUN(MY_CXX_SWITCH,[
        AC_MSG_CHECKING(for [$1] as CXX compilation switch)
        cat > conftest.c <<__EOF
int main() { return 0;}
__EOF
        my_c='${CXX-cc} $CXXFLAGS $1 conftest.c -o conftest${ac_exeext}'
        (eval echo configure:__oline__: \"$my_c\") 1>&5
        (eval $my_c 1>/dev/null 2>conftest.log)
        cat conftest.log 1>&5
        if grep <conftest.log option  >/dev/null ||
           grep <conftest.log ERROR >/dev/null
        then
                my_cxx_switch="no"
                AC_MSG_RESULT(no)
        else
                my_cxx_switch="yes"
                AC_MSG_RESULT(yes)
        fi
        rm -rf conftest*
])
