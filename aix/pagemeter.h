//
//  Copyright (c) 2026 by Antoni Sawicki ( as@tenoware.com )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _PAGEMETER_H_
#define _PAGEMETER_H_

#include "fieldmetergraph.h"
#include "xosview.h"


class PageMeter : public FieldMeterGraph {
public:
  PageMeter( XOSView *parent, float max );
  ~PageMeter( void );

  const char *name( void ) const { return "PageMeter"; }
  void checkevent( void );

  void checkResources( void );

protected:
  double pageinfo_[2][2];
  int pageindex_;
  float maxspeed_;
  bool ok_;

  void getpageinfo( void );
};

#endif
