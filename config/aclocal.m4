dnl
dnl This file containes a macro for each os xosview has been ported to.
dnl Each macro can add specific config options that apply to only that
dnl specific port.
dnl

AC_DEFUN(AC_XOSV_LINUX, [
dnl
dnl Add a switch to add -DUSESYSCALLS for linux.
dnl
AC_ARG_ENABLE([linux-syscalls],
[  --enable-linux-syscalls use system calls when possible],

if test "$enableval" = "no"
then
        echo "disabled Linux system calls"
else
        DEFINES="$DEFINES -DUSESYSCALLS"
        echo "enabled  Linux system calls"
fi
,
DEFINES="$DEFINES -DUSESYSCALLS"
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
fi
,
MEMSTAT=
echo "disabled the Linux memstat module by default"
)
AC_SUBST(MEMSTAT)
])


AC_DEFUN(AC_XOSV_NETBSD, [
dnl
dnl Nethsd needs to link with libknm
dnl
        EXTRALIBS=-lkvm
])


AC_DEFUN(AC_XOSV_HPUX, [
dnl
dnl No special config options for HPUX.
dnl
])
