//  
// $Id$
//  Initial port performed by Stefan Eilemann (eile@sgi.com)
//

#ifndef _SARMETER_H_
#define _SARMETER_H_

#include "fieldmetergraph.h"
#include <unistd.h>
#include <sys/sysinfo.h>

// common function for all sar based graphs
class SarMeter
{
public:
    static SarMeter *Instance();

    gfxinfo *getGfxInfo( void )
        {
            checkSadc();

            if( _giNew )
            {
                _giNew = 0;
                return &_gi;
            }
            else
                return NULL;
        }

private:
    SarMeter()
            : _lastPos(0),
              _giNew(0)
        {
            _input = setupSadc();
        }

    ~SarMeter(void) {}

    size_t readLine( int input, char *buf, size_t max );
    int    setupSadc( void );
    void   checkSadc( void );

    static SarMeter *_instance;
    int    _input;
    off_t  _lastPos;
    char   _buf[50000];

    gfxinfo _gi;
    bool    _giNew;
};

#endif
