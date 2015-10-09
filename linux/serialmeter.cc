//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

//
//  In order to use this new serial meter, xosview needs to be suid root.
//
#include "serialmeter.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

// This should go away after types.h gets fixed in the kernel.
#ifdef __alpha__
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long s64;
typedef unsigned long long u64;
#endif

#if defined(GNULIBC) || defined(__GLIBC__)
#if !defined(__powerpc__) && !defined(__hppa__) && !defined(__mips__) && !defined(__sparc__) && !defined(__sh__) && !defined(__s390__) && !defined(__s390x__) && !defined(__m68k__) && !defined(__aarch64__)
#include <sys/io.h>
#endif
#if !defined(__alpha__) && !defined(__sparc__) && !defined(__powerpc__) && !defined(__ia64__) && !defined(__hppa__) && !defined(__arm__) && !defined(__mips__) && !defined(__sh__) && !defined(__s390__) && !defined (__s390x__) && !defined(__m68k__) && !defined(__aarch64__)
#include <sys/perm.h>
#define HAVE_IOPERM
#endif
#else
#if !defined(__alpha__) && !defined(__sparc__) && !defined(__powerpc__) && !defined(__ia64__) && !defined(__m68k__)
#include <asm/io.h>
#endif
#endif
#include <linux/serial.h>


SerialMeter::SerialMeter( XOSView *parent, Device device )
  : BitMeter( parent, getTitle(device), "LSR bits(0-7), MSR bits(0-7)", 16){
  _device = device;
  _port = 0;
}

SerialMeter::~SerialMeter( void ){
}

void SerialMeter::checkevent( void ){
  getserial();
  BitMeter::checkevent();
}

void SerialMeter::checkResources( void ){
  BitMeter::checkResources();
  onColor_  = parent_->allocColor( parent_->getResource( "serialOnColor" ) );
  offColor_ = parent_->allocColor( parent_->getResource( "serialOffColor" ) );
  priority_ = atoi (parent_->getResource( "serialPriority" ) );

  _port = getPortBase(_device);
  if (!getport(_port + UART_LSR) || !getport(_port + UART_MSR)){
    std::cerr << "SerialMeter::SerialMeter() : "
         << "xosview must be suid root to use the serial meter." << std::endl;
    parent_->done(1);
  }
}

bool SerialMeter::getport(unsigned short int port){
#ifdef HAVE_IOPERM
  return ioperm(port, 1, 1) != -1;
#else
  return -1 != -1;
#endif
}

void SerialMeter::getserial( void ){
#ifdef HAVE_IOPERM
  // get the LSR and MSR
  unsigned char lsr = inb(_port + UART_LSR);
  unsigned char msr = inb(_port + UART_MSR);

  setBits(0, lsr);
  setBits(8, msr);
#endif
}

const char *SerialMeter::getTitle(Device dev) const {
  static const char *names[] = { "ttyS0", "ttyS1", "ttyS2", "ttyS3",
                                 "ttyS4", "ttyS5", "ttyS6", "ttyS7",
                                 "ttyS8", "ttyS9" };
  return names[dev];
}

const char *SerialMeter::getResourceName(Device dev){
  static const char *names[] = { "serial0", "serial1",
                                 "serial2", "serial3",
                                 "serial4", "serial5",
                                 "serial6", "serial7",
                                 "serial8", "serial9" };

  return names[dev];
}

unsigned short int SerialMeter::getPortBase(Device dev) const {
  static const char *deviceFile[] = { "/dev/ttyS0",
                                "/dev/ttyS1",
                                "/dev/ttyS2",
                                "/dev/ttyS3",
                                "/dev/ttyS4",
                                "/dev/ttyS5",
                                "/dev/ttyS6",
                                "/dev/ttyS7",
                                "/dev/ttyS8",
                                "/dev/ttyS9"};

  const char* res = parent_->getResource(getResourceName(dev));

  if (!strncasecmp(res, "True", 5)){ // Autodetect portbase.
    int fd;
    struct serial_struct serinfo;

    // get the real serial port (code stolen from setserial 2.11)
    if ((fd = open(deviceFile[dev], O_RDONLY|O_NONBLOCK)) < 0) {
      std::cerr << "SerialMeter::SerialMeter() : "
           << "failed to open " << deviceFile[dev] <<"." <<std::endl;
      exit(1);
    }
    if (ioctl(fd, TIOCGSERIAL, &serinfo) < 0) {
      std::cerr << "Failed to detect port base for " << deviceFile[dev]
        << std::endl;
      close(fd);
      exit(1);
    }

    close(fd);
    return serinfo.port;
  }
  else { // Use user specified port base.
    std::string s(res);
    std::istringstream istrm(s);
    unsigned short int tmp = 0;
    istrm >> std::hex >> tmp;
    return tmp;
  }

  return 0;
}
