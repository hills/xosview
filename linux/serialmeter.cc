//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

//
//  In order to use this new serial meter, xosview needs to be suid root.
//  

#include "serialmeter.h"
#include "xosview.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdlib.h>

#include <unistd.h>
#include <asm/io.h>
#include <linux/serial_reg.h>


const unsigned short int SerialMeter::Ports[4] = {
  0x03f8, 0x02f8, 0x03e8, 0x02e8
};


SerialMeter::SerialMeter( XOSView *parent,
			  Device device,
			  const char *title, const char *, int dolegends,
			  int dousedlegends )
: BitMeter( parent, title, 
	    "LSR bits(0-7), MSR bits(0-7)", 16, 
	    dolegends, dousedlegends ) {

  _port = Ports[device];
  if (!getport(_port + UART_LSR) || !getport(_port + UART_MSR))
    {
      cerr << "SerialMeter::SerialMeter() : "
        << "xosview must be suid root to use the serial meter." <<endl;
      exit(1);
    }
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
}

bool SerialMeter::getport(unsigned short int port){
  return ioperm(port, 1, 1) != -1;
}

void SerialMeter::getserial( void ){
  // get the LSR and MSR
  unsigned char lsr = inb(_port + UART_LSR);
  unsigned char msr = inb(_port + UART_MSR);

  //cerr <<"lsr = " <<(int)lsr <<", msr = " <<(int)msr <<endl;

  setBits(0, lsr);
  setBits(8, msr);
}


