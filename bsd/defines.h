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
#elif defined _BSDI_VERSION
#define XOSVIEW_BSDI
#endif

/* UVM appeared on NetBSD 1.4 and OpenBSD 2.9. */
#if ( defined(__NetBSD__) && __NetBSD_Version >= 104000000 ) || \
    ( defined(__OpenBSD__) && OpenBSD >= 200105 )
#define UVM 1
#endif

/* swapctl appeared on NetBSD 1.3. and OpenBSD 2.6 */
#if ( defined(__NetBSD__) && __NetBSD_Version >= 103000000 ) || \
    ( defined(__OpenBSD__) && OpenBSD >= 199912 )
#define HAVE_SWAPCTL 1
#endif

/* devstat appeared on FreeBSD 3.0. */
#if ( defined(__FreeBSD__) && __FreeBSD_version >= 300005 )
#define HAVE_DEVSTAT 1
#endif

/* kvm_getswapinfo appeared on FreeBSD 4.0 */
#if ( defined(__FreeBSD__) && __FreeBSD_version >= 400000 )
#define USE_KVM_GETSWAPINFO 1
#endif

/* Our battery meter is only supported on OpenBSD and NetBSD. */
#if ( defined(__OpenBSD__) || defined(__NetBSD__) )
#define HAVE_BATTERY_METER 1
#endif


#endif
