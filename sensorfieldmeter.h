//
//  Copyright (c) 2014 by Tomi Tapper <tomi.o.tapper@jyu.fi>
//
//  This file may be distributed under terms of the GPL
//
//  Put code common to *BSD and Linux sensor meters here.
//

#ifndef _SENSORFIELDMETER_H_
#define _SENSORFIELDMETER_H_

#include "fieldmeter.h"
#include "xosview.h"


class SensorFieldMeter : public FieldMeter {
public:
  SensorFieldMeter( XOSView *parent, const char *title = "",
                    const char *legend = "", int docaptions = 0,
                    int dolegends = 0, int dousedlegends = 0 );
  ~SensorFieldMeter( void );

protected:
  void updateLegend( void );
  void checkFields( double low, double high );
  char unit_[8];
  double high_, low_;
  bool has_high_, has_low_, negative_;
  unsigned long actcolor_, highcolor_, lowcolor_;

private:

};


#endif
