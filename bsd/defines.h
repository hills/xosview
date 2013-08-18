#ifndef __defines_h__
#define __defines_h__

#include <sys/param.h>

/* The BSD variant. */
#if defined __FreeBSD__
#define XOSVIEW_FREEBSD
#elif defined __NetBSD__
#define XOSVIEW_NETBSD
#elif defined __OpenBSD__
#define XOSVIEW_OPENBSD
#elif defined __DragonFly__
#define XOSVIEW_DFBSD
#endif

#if !( defined(XOSVIEW_FREEBSD) || \
       defined(XOSVIEW_NETBSD)  || \
       defined(XOSVIEW_OPENBSD) || \
       defined(XOSVIEW_DFBSD) )
#error "Unsupported BSD variant."
#endif

/* UVM appeared on NetBSD 1.4 and OpenBSD 2.9. */
#if ( defined(__NetBSD__) && __NetBSD_Version__ >= 104000000 ) || \
    ( defined(__OpenBSD__) && OpenBSD >= 200105 )
#define HAVE_UVM 1
#endif

/* swapctl appeared on NetBSD 1.3. and OpenBSD 2.6 */
#if ( defined(__NetBSD__) && __NetBSD_Version__ >= 103000000 ) || \
    ( defined(__OpenBSD__) && OpenBSD >= 199912 )
#define HAVE_SWAPCTL 1
#endif

/* devstat appeared on FreeBSD 3.0. */
#if ( defined(__FreeBSD__) && __FreeBSD_version >= 300005 ) || \
      defined(__DragonFly__)
#define HAVE_DEVSTAT 1
#endif

/* kvm_getswapinfo appeared on FreeBSD 4.0 */
#if ( defined(__FreeBSD__) && __FreeBSD_version >= 400000 ) || \
      defined(__DragonFly__)
#define USE_KVM_GETSWAPINFO 1
#endif

/* Helper defines for battery meter. */
#define XOSVIEW_BATT_NONE         0
#define XOSVIEW_BATT_CHARGING     1
#define XOSVIEW_BATT_DISCHARGING  2
#define XOSVIEW_BATT_FULL         4
#define XOSVIEW_BATT_LOW          8
#define XOSVIEW_BATT_CRITICAL    16


#endif
