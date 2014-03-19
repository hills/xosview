//
//  Copyright (c) 1999 by Brian Grayson (bgrayson@netbsd.org)
//
//  This file may be distributed under terms of the GPL or of the BSD
//    license, whichever you choose.  The full license notices are
//    contained in the files COPYING.GPL and COPYING.BSD, which you
//    should have received.  If not, contact one of the xosview
//    authors for a copy.
//

#ifndef _IRQRATEMETER_H_
#define _IRQRATEMETER_H_

#include "fieldmetergraph.h"
#include "xosview.h"


class IrqRateMeter : public FieldMeterGraph {
public:
	IrqRateMeter( XOSView *parent );
	~IrqRateMeter( void );

	const char *name( void ) const { return "IrqRateMeter"; }
	void checkevent( void );
	void checkResources( void );

private:
	uint64_t *irqs_, *lastirqs_;
	unsigned int irqcount_;

protected:
	void getinfo( void );
};


#endif
