//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  NetBSD port:  
//  Copyright (c) 1995, 1996, 1997-2002 by Brian Grayson (bgrayson@netbsd.org)
//
//  This file was written by Brian Grayson for the NetBSD and xosview
//    projects.
//  This file may be distributed under terms of the GPL or of the BSD
//    license, whichever you choose.  The full license notices are
//    contained in the files COPYING.GPL and COPYING.BSD, which you
//    should have received.  If not, contact one of the xosview
//    authors for a copy.
//

#ifndef _PAGEMETER_H_
#define _PAGEMETER_H_

#include "fieldmetergraph.h"
#if defined(UVM)
#include <sys/param.h>
#if defined(XOSVIEW_NETBSD) && (__NetBSD_Version__ > 105010000 /* > 1.5A */)
// No includes needed -- uvm_extern.h is now self-contained.  Noticed
// by Bernd Ernesti.
#else
// Earlier versions of NetBSD still required vm/vm.h to be included.
#include <vm/vm.h>
#endif
#include <uvm/uvm_extern.h>
#else
#include <sys/vmmeter.h>
#endif

class PageMeter : public FieldMeterGraph {
public:
  PageMeter( XOSView *parent, double total );
  ~PageMeter( void );

  const char *name( void ) const { return "PageMeter"; }  
  void checkevent( void );

  void checkResources( void );
protected:

  void getpageinfo( void );
private:
#if defined(UVM)
# ifdef VM_UVMEXP2
  struct uvmexp_sysctl prev_;
# else
  struct uvmexp	prev_;
# endif
#else
  struct vmmeter prev_;
#endif
};


#endif
