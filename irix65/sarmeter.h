//  
//  Initial port performed by Stefan Eilemann (eilemann@gmail.com)
//

#ifndef _SARMETER_H_
#define _SARMETER_H_

#include "fieldmetergraph.h"
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/elog.h>

// some structs 
typedef struct {
    unsigned int recsize;
    unsigned int numrec;
}
header;

typedef struct {
    char           name[12];
    char           pad1[68];
    struct iotime  stat;
    char           pad2[4];
} 
diskinfo;

#define MAX_DISKS 16
#define BUFSIZE   0x2000

// common function for all sar based graphs
class SarMeter
{
public:
    static SarMeter *Instance();

    struct GfxInfo {
        unsigned int swapBuf;
    };

    struct DiskInfo {
        unsigned int nDevices;

        unsigned int read[MAX_DISKS];
        unsigned int write[MAX_DISKS];
    };

    GfxInfo *getGfxInfo( void )
        {
            checkSadc();
            return &_gi.info;
        }

    DiskInfo *getDiskInfo( void )
        {
            checkSadc();
            return &_di.info;
        }

private:
    SarMeter();
    ~SarMeter(void) {}

    int  setupSadc( void );

    void checkSadc( void );

    bool readLine( void );
    void parseBuffer( void );
    void forwardBufferTo( char *ptr );

    void newGfxInfo( void );
    void newDiskInfo( void );


    static SarMeter *_instance;
    int    _input;
    off_t  _bufSize;
    char   _buf[BUFSIZE];

    struct {
        gfxinfo current;
        gfxinfo last;
        GfxInfo info;
    } _gi;

    struct {
        diskinfo current[MAX_DISKS];
        diskinfo last[MAX_DISKS];
        DiskInfo info;
    } _di;
};

#endif
