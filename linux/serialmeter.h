//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _SERIALMETER_H_
#define _SERIALMETER_H_

#include "bitmeter.h"


class SerialMeter : public BitMeter {
public:
  enum Device { S0, S1, S2, S3 };

  SerialMeter( XOSView *parent, Device device,
	       const char *title = "", const char *legend ="",
	       int dolegends = 0, int dousedlegends = 0 );
  ~SerialMeter( void );

  void checkevent( void );

  void checkResources( void );
protected:
  unsigned short int _port;

  static const unsigned short int Ports[4];

  void getserial( void );
  bool getport(unsigned short int port);
};

#endif
