//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _METER_H_
#define _METER_H_

class XOSView;

class Meter {
public:
  Meter( XOSView *parent, const char *title = "", const char *legend ="", 
	 int dolegends = 0, int dousedlegends = 0 );
  virtual ~Meter( void );

  virtual const char *name( void ) const { return "Meter"; }
  void resize( int x, int y, int width, int height );
  virtual void checkevent( void ) = 0;
  virtual void draw( void ) = 0;
  void title( const char *title );
  const char *title( void ) { return title_; }
  void legend( const char *legend );
  const char *legend( void ) { return legend_; }
  void dolegends( int val ) { dolegends_ = val; }
  void dousedlegends( int val ) { dousedlegends_ = val; }
  int requestevent( void ){ 
    int rval = counter_ % priority_;
    counter_ = (counter_ + 1) % priority_;
    return !rval;
  }

  virtual void checkResources( void );

protected:
  XOSView *parent_;
  int x_, y_, width_, height_, dolegends_, dousedlegends_;
  int priority_, counter_;
  char *title_, *legend_;
  unsigned long textcolor_;

private:
};

#endif
