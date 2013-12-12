//
//  Copyright (c) 2012 by Tomi Tapper <tomi.o.tapper@jyu.fi>
//
//  File based on linux/lmstemp.* by
//  Copyright (c) 2000, 2006 by Leopold Toetsch <lt@toetsch.at>
//
//  This file may be distributed under terms of the GPL
//
//
//
#ifndef _BSDSENSOR_H_
#define _BSDSENSOR_H__


#include "fieldmeter.h"
#include <string>

#define NAMESIZE 32


class BSDSensor : public FieldMeter {
public:
	BSDSensor( XOSView *parent, const char *name, const char *high,
	           const char *low, const char *label, const char *caption, int nbr );
	~BSDSensor( void );

	const char *name( void ) const { return "BSDSensor"; }
	void checkevent( void );
	void checkResources( void );

protected:
	void getsensor( void );

private:
	void updateLegend( void );
	char name_[NAMESIZE], highname_[NAMESIZE], lowname_[NAMESIZE];
	char val_[NAMESIZE], highval_[NAMESIZE], lowval_[NAMESIZE];
	char unit_[8];
	unsigned long actcolor_, highcolor_, lowcolor_;
	float high_, low_;
	bool hashigh_;
	int nbr_;
};


#endif
