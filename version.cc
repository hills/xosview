/*  This file is included by xosview.cc, to set up the version
 *  string.  */

/*  WHEN CHANGING the version number:
    1.  Make sure you catch all of these files:
      version.cc
      xosview.1
      config/Makefile.top.in
    2.  Make sure CHANGES, the man page, and the list below to
    all be roughly correct.
*/
/*  *****  THIS FILE is included by xosview.cc, and is never used
 *  on its own.  */
static const char * const versionString = "xosview version 1.7.1";
static const char * const version_cc_cvsID = "$Id$";

/*  Version 1.7.0:  has pixmap support, OpenBSD interrupt meter,
 *  NetBSD battery meter, some more Solaris support,
 *  sliding-graphs, and more.  */

/*  Version 1.6.2.b:  now has FreeBSD libdevstat support, for fixed
    diskmeter.  bgrayson  */

/*  Version 1.6.2.a:  a snapshot that contains, among other
 *  things, the merged-in patches from NetBSD pkgsrc, in addition
 *  to fixes to allow NetBSD-alpha to mostly work.  bgrayson  */
