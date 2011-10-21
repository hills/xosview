//
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//  2007 by Samuel Thibault ( samuel.thibault@ens-lyon.org )
//
//  This file may be distributed under terms of the GPL
//
//  Most of this code was written by Werner Fink <werner@suse.de>
//  Only small changes were made on my part (M.R.)
//

#ifndef _LOADMETER_H_
#define _LOADMETER_H_


#include "fieldmetergraph.h"


class LoadMeter : public FieldMeterGraph {
public:
  LoadMeter( XOSView *parent );
  ~LoadMeter( void );

  const char *name( void ) const { return "LoadMeter"; }
  void checkevent( void );

  void checkResources( void );
protected:

  void getloadinfo( void );
  unsigned long procloadcol_, warnloadcol_, critloadcol_;
private:
  int warnThreshold, critThreshold, alarmstate, lastalarmstate;
};


#endif
