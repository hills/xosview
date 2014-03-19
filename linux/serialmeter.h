//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _SERIALMETER_H_
#define _SERIALMETER_H_

// hack for not having linux/serial_reg.h, (Debian bug #427599)
#define UART_LSR        5
#define UART_MSR        6

#include "bitmeter.h"
#include "xosview.h"


class SerialMeter : public BitMeter {
public:
  enum Device { S0, S1, S2, S3, S4, S5, S6, S7, S8, S9 };
  static int numDevices(void) { return 10; }

  SerialMeter( XOSView *parent, Device device);
  ~SerialMeter( void );

  static const char *getResourceName(Device dev);

  void checkevent( void );

  void checkResources( void );

private:
  unsigned short int _port;
  Device _device;

  void getserial( void );
  bool getport(unsigned short int port);
  const char *getTitle(Device dev) const;
  unsigned short int getPortBase(Device dev) const;
};

#endif
