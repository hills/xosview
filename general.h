#ifndef General_h
#define General_h

//  Copyright (c) 1996, 1997-2002 by Brian Grayson (bgrayson@netbsd.org)
//
//  This file was written by Brian Grayson for the NetBSD and xosview
//    projects.
//  This file may be distributed under terms of the GPL or of the BSD
//    license, whichever you choose.  The full license notices are
//    contained in the files COPYING.GPL and COPYING.BSD, which you
//    should have received.  If not, contact one of the xosview
//    authors for a copy.
//
#define GENERAL_H_CVSID "$Id$"

  /*  This file should be included by every .cc file, after any system
      includes but before any local includes.  It should NOT be included
      by any .h files unless there is a REALLY good reason.  */


#ifdef __GNUC__
  /*  Grab _G_HAVE_BOOL, if possible.  */
#include <_G_config.h>
#endif

#ifndef _G_HAVE_BOOL
  /*  Every GNU system has _G_config.h, I believe, which tells us
      whether or not the bool define exists.  However, for simplicity,
      let's just redefine them all.  The following lines are directly
      from g++-include's bool.h file.  bgrayson */

#undef FALSE
#undef false
#undef TRUE
#undef true
enum bool { FALSE = 0, false = 0, TRUE = 1, true = 1 };
#endif


  /*  Let's add a handy macro for inserting the CVS id string into the
      .cc (and hence .o) files, so that 'ident' can be used on the
      executable.  */

  /*  Note:  If your compiler chokes on __attribute__ stuff, add
      -DNO_ADD_ID_STR to CFLAGS in the Makefile.  gcc handles
      __attribute__ fine, and both of the primary authors use gcc, so
      things are a little gcc-biased...  :)  */

  /*  NetBSD note:  In their semi-infinite wisdom, the NetBSD
      file <sys/cdefs.h> (which is included via a circuitous
      route by some of the xosview source) automatically
      redefs __attribute__(x) to be nothing if one is using
      gcc-2.4.x or earlier.  This could be considered a
      feature, but in our current particular instance,
      it hides information if one is using gcc-2.4.5
      (NetBSD-1.1 and earlier's standard gcc).  Basically, by
      nullifying __attribute__, gcc says "cvsid defined but not
      used", whereas if the __attribute__ were left alone,
      gcc-2.4.5 would say "Ignoring unknown attribute "unused"",
      followed by "cvsid defined but not used", which to me is much
      clearer about what is going on.  bgrayson 9/96  */

#if !defined( NO_ADD_ID_STR) && defined(__GNUC__)
    /*  If it's GNUC, we'll assume it can handle the __attribute__
	((unused)) modifier.  Notice that 2.4.5 can parse this, but will
	not handle ((unused)).  So, for gcc 2.4.5 (and below), set
	NO_ADD_ID_STR, or live with the warning messages.  */
    /*  Notice that we need to use a unique-within-the-cpp'd-file
     *  identifier for the char*'s, hence the definition of
     *  CVSID_DOT_H2 etc., which are occasionally needed.  */
  #define CVSID(s) \
      static char* cvsid __attribute__ ((unused)) = (s)
  #define CVSID_DOT_H(s) \
      static char* cvsid_dot_h __attribute__ ((unused)) = (s)
  #define CVSID_DOT_H2(s) \
      static char* cvsid_dot_h2 __attribute__ ((unused)) = (s)
  #define CVSID_DOT_H3(s) \
      static char* cvsid_dot_h3 __attribute__ ((unused)) = (s)
#else
    /*  Don't put the CVS Id strings in the .o files -- the
	compiler will probably print tons of warning about
	unused variables if we did.  */
  #define CVSID(s)
  #define CVSID_DOT_H(s)
  #define CVSID_DOT_H2(s)
  #define CVSID_DOT_H3(s)
#endif

#endif
