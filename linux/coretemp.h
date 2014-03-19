//
//  Copyright (c) 2008-2014 by Tomi Tapper <tomi.o.tapper@jyu.fi>
//
//  File based on linux/lmstemp.* by
//  Copyright (c) 2000, 2006 by Leopold Toetsch <lt@toetsch.at>
//
//  This file may be distributed under terms of the GPL
//
//

#ifndef _CORETEMP_H_
#define _CORETEMP_H_

#include "fieldmeter.h"
#include "xosview.h"
#include <string>
#include <vector>


class CoreTemp : public FieldMeter {
public:
  CoreTemp( XOSView *parent, const char *label, const char *caption, int pkg, int cpu);
  ~CoreTemp( void );

  const char *name( void ) const { return "CoreTemp"; }
  void checkevent( void );
  void checkResources( void );

  static unsigned int countCores( unsigned int pkg );
  static unsigned int countCpus( void );

protected:
  void getcoretemp( void );

private:
  void findSysFiles( void );
  int _pkg, _cpu, _high;
  std::vector<std::string> _cpus;
  unsigned long _actcolor, _highcolor;
};


#endif
