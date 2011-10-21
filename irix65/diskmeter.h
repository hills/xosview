//  
//  Initial port performed by Stefan Eilemann (eilemann@gmail.com)
//

#ifndef _DISKMETER_H_
#define _DISKMETER_H_

#include "fieldmetergraph.h"

class DiskMeter : public FieldMeterGraph 
{
public:
    DiskMeter( XOSView *parent, float max );
    ~DiskMeter( void );
    
    const char *name( void ) const { return "DiskMeter"; }
    void checkevent( void );
    
    void checkResources( void );
protected:
    
    void getdiskinfo( void );
private:
    float maxspeed_;
};

#endif
