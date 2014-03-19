//
//  Copyright (c) 2013 by Tomi Tapper ( tomi.o.tapper@student.jyu.fi )
//
//  Based on linux/btrymeter.h:
//  Copyright (c) 1997, 2005, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _BTRYMETER_H_
#define _BTRYMETER_H_

#include "fieldmeter.h"
#include "xosview.h"


class BtryMeter : public FieldMeter {
public:
	BtryMeter( XOSView *parent );
	~BtryMeter( void );

	const char *name( void ) const { return "BtryMeter"; }
	void checkevent( void );
	void checkResources( void );

protected:
	void getstats( void );

private:
	unsigned long leftcolor_, usedcolor_, chargecolor_, fullcolor_,
	              lowcolor_, critcolor_, nonecolor_;
	unsigned int  old_state_;
};


#endif
