//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _XOSVIEW_H_
#define _XOSVIEW_H_

#include "xwin.h"
#include "Xrm.h"  //  For Xrm resource manager class.

class Meter;

class XOSView : public XWin {
public:
  XOSView( int argc, char *argv[] );
  ~XOSView( void );

  void resize( void );
  void draw( void );
  void run( void );
  void keyrelease( char *ch );
  const char *winname( void );

  // used by meter makers
  int xoff(void) const { return xoff_; }
  int newypos( void );

protected:

  Xrm xrm;
  void checkArgs (int argc, char** argv) const;
  class MeterNode {
  public:
    MeterNode( Meter *fm ) { meter_ = fm;  next_ = NULL; }

    Meter *meter_;
    MeterNode *next_;
  };

  MeterNode *meters_;
  
  int legend_, xoff_, yoff_, nummeters_, usedlabels_;

  void usleep( unsigned long usec );
  void addmeter( Meter *fm );
  void checkMeterResources( void );

  int findx( void );
  int findy( void );
  void dolegends( void );

  void checkOverallResources();
  void resizeEvent( XEvent &) { resize(); draw(); }
  void exposeEvent( XExposeEvent &event );
  void keyPressEvent( XKeyEvent &event );
  void visibilityEvent( XVisibilityEvent &event );
  void unmapEvent( XUnmapEvent &event);
private:

  bool _isvisible;
};

#endif
