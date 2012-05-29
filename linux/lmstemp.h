//
//  Copyright (c) 2000, 2006 by Leopold Toetsch <lt@toetsch.at>
//
//  File based on btrymeter.* by
//  Copyright (c) 1997 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
//
//
#ifndef _LMSTEMP_H_
#define _LMSTEMP_H_


#include "fieldmeter.h"


class LmsTemp : public FieldMeter {
public:
  LmsTemp( XOSView *parent, const char *tempfile, const char *highfile, const char *label, const char * caption);
  ~LmsTemp( void );

  const char *name( void ) const { return "LmsTemp"; }
  void checkevent( void );

  void checkResources( void );
protected:

  void getlmstemp( void );
  bool checksensors(int isproc, const char *dir, const char* tempfile, const char* highfile);
private:
  char *_tempfile, *_highfile;
  int _high;
  int _isproc;
};


#endif
