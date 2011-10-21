//
//  Copyright (c) 1996 by Massimiliano Ghilardi ( ghilardi@cibs.sns.it )
//  2007 by Samuel Thibault ( samuel.thibault@ens-lyon.org )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _PAGEMETER_H_
#define _PAGEMETER_H_

#include "fieldmetergraph.h"

class PageMeter : public FieldMeterGraph {
public:
  PageMeter( XOSView *parent, float max );
  ~PageMeter( void );

  const char *name( void ) const { return "PageMeter"; }
  void checkevent( void );

  void checkResources( void );
protected:
  float pageinfo_[2][2];
  int pageindex_;
  float maxspeed_;

  void getpageinfo( void );
  void updateinfo(void);
private:
};

#endif
