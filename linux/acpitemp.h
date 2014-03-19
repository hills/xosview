//
//  Copyright (c) 2009 by Tomi Tapper <tomi.o.tapper@jyu.fi>
//
//  File based on lmstemp.* by
//  Copyright (c) 2000, 2006 by Leopold Toetsch <lt@toetsch.at>
//
//  This file may be distributed under terms of the GPL
//
//
//
#ifndef _ACPITEMP_H_
#define _ACPITEMP_H_


#include "fieldmeter.h"
#include "xosview.h"

#define PATH_SIZE 128


class ACPITemp : public FieldMeter {
public:
  ACPITemp( XOSView *parent, const char *tempfile, const char *highfile, const char *label, const char * caption);
  ~ACPITemp( void );

  const char *name( void ) const { return "ACPITemp"; }
  void checkevent( void );

  void checkResources( void );
protected:

  void getacpitemp( void );
  int  checkacpi(const char* tempfile, const char* highfile);
private:
  char _tempfile[PATH_SIZE];
  char _highfile[PATH_SIZE];
  int  _high;
  bool _usesysfs;
  unsigned long _actcolor, _highcolor;
};


#endif
