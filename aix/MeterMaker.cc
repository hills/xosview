//
//  Copyright (c) 2026 by Antoni Sawicki ( as@tenoware.com )
//
//  This file may be distributed under terms of the GPL
//

#include "MeterMaker.h"
#include "xosview.h"

#include "cpumeter.h"
#include "diskmeter.h"
#include "loadmeter.h"
#include "memmeter.h"
#include "netmeter.h"
#include "pagemeter.h"
#include "swapmeter.h"

#include <stdlib.h>


MeterMaker::MeterMaker( XOSView *xos ){
  _xos = xos;
}

void MeterMaker::makeMeters( void ){
  if ( _xos->isResourceTrue( "load" ) )
    push( new LoadMeter( _xos ) );

  if ( _xos->isResourceTrue( "cpu" ) )
    push( new CPUMeter( _xos ) );

  if ( _xos->isResourceTrue( "mem" ) )
    push( new MemMeter( _xos ) );

  if ( _xos->isResourceTrue( "swap" ) )
    push( new SwapMeter( _xos ) );

  if ( _xos->isResourceTrue( "page" ) )
    push( new PageMeter( _xos, atof( _xos->getResource( "pageBandwidth" ) ) ) );

  if ( _xos->isResourceTrue( "disk" ) )
    push( new DiskMeter( _xos, atof( _xos->getResource( "diskBandwidth" ) ) ) );

  if ( _xos->isResourceTrue( "net" ) )
    push( new NetMeter( _xos, atof( _xos->getResource( "netBandwidth" ) ) ) );
}
