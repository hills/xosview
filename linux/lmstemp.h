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

#include "sensorfieldmeter.h"
#include "xosview.h"
#include <string>


class LmsTemp : public SensorFieldMeter {
public:
  LmsTemp( XOSView *parent, const char *name, const char *tempfile,
           const char *highfile, const char *lowfile, const char *label,
           const char *caption, unsigned int nbr );
  ~LmsTemp( void );

  const char *name( void ) const { return "LmsTemp"; }
  void checkevent( void );
  void checkResources( void );

protected:
  void getlmstemp( void );
  bool checksensors( const char *name, const char *tempfile,
                     const char *highfile, const char *lowfile );
private:
  void determineScale( void );
  void determineUnit( void );
  std::string _tempfile, _highfile, _lowfile;
  unsigned int _nbr;
  double _scale;
  bool _isproc, _name_found, _temp_found, _high_found, _low_found;
};


#endif
