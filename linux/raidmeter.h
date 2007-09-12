//
//  Copyright (c) 1999, 2006 Thomas Waldmann ( ThomasWaldmann@gmx.de )
//  based on work of Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
#ifndef _RAIDMETER_H_
#define _RAIDMETER_H_

#include "bitfieldmeter.h"

#define MAX_MD 8

class RAIDMeter : public BitFieldMeter {
public:
  RAIDMeter( XOSView *parent, int raiddev = 0);
  ~RAIDMeter( void );

  void checkevent( void );

  void checkResources( void );

  static int countRAIDs( void );

protected:

  int _raiddev;
  static int mdnum;

  char state[20],
       type[20],
       working_map[20],
       resync_state[20];
  int  disknum;

  unsigned long doneColor_, todoColor_, completeColor_;

  int find1(const char *key, const char *findwhat, int num1);
  int find2(const char *key, const char *findwhat, int num1, int num2);

  int raidparse(char *cp);

  void getRAIDstate( void );
};

#endif
