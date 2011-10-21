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

  /*  This file should be included by every .cc file, after any system
      includes but before any local includes.  It should NOT be included
      by any .h files unless there is a REALLY good reason.  */

#if !defined(__GNUC__) && !defined(__sun)
  /*  Every GNU system has _G_config.h, I believe, which tells us
      whether or not the bool define exists.  However, for simplicity,
      let's just redefine them all.  The following lines are directly
      from g++-include's bool.h file.  bgrayson */
  /* eile: The MIPSPro compiler on IRIX does not like the redefinition
     of 'bool'. Nowadays every c++ compiler should define bool anyway....

#undef FALSE
#undef false
#undef TRUE
#undef true
enum bool { FALSE = 0, false = 0, TRUE = 1, true = 1 };
  */

#endif

#endif
