//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _XOSVIEW_H_
#define _XOSVIEW_H_

#include "xwin.h"
#include "Xrm.h"  //  For Xrm resource manager class.
#include <stdio.h>

/*  Take at most n samples per second (default of 10)  */
extern double MAX_SAMPLES_PER_SECOND;

class Meter;

class XOSView : public XWin {
public:
  XOSView( const char* instName, int argc, char *argv[] );
  ~XOSView( void );

  void figureSize ( void );
  void resize( void );
  void draw ( void );
  void run( void );
  const char *winname( void );

  // used by meter makers
  int xoff(void) const { return xoff_; }
  int newypos( void );

  enum windowVisibilityState { FULLY_VISIBLE, PARTIALLY_VISIBILE, OBSCURED };
  enum windowVisibilityState getWindowVisibilityState(void) { return windowVisibility; }

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

  int caption_, legend_, xoff_, yoff_, nummeters_, usedlabels_;
  int hmargin_, vmargin_, vspacing_;
  unsigned long sleeptime_, usleeptime_;

  void usleep_via_select( unsigned long usec );
  void addmeter( Meter *fm );
  void checkMeterResources( void );

  int findx( void );
  int findy( void );
  void dolegends( void );

  void checkOverallResources();
  void resizeEvent( XConfigureEvent &event );
  void exposeEvent( XExposeEvent &event );
  void keyPressEvent( XKeyEvent &event );
  void visibilityEvent( XVisibilityEvent &event );
  void unmapEvent( XUnmapEvent &event);
  void checkVersion(int argc, char *argv[]) const;

private:

  bool _deferred_resize;
  bool _deferred_redraw;
  enum windowVisibilityState windowVisibility;
};

#ifdef DEBUG
#define XOSDEBUG(...) { \
  fprintf(stderr, "%s:%d: ", __func__, __LINE__); \
  fprintf(stderr, __VA_ARGS__); \
}
#else
#define XOSDEBUG(...)
#endif

#endif
