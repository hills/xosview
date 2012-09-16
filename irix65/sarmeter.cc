//  
//  Initial port performed by Stefan Eilemann (eilemann@gmail.com)
//

#include "sarmeter.h"

#include <fcntl.h>

SarMeter *SarMeter::_instance = NULL;

SarMeter *SarMeter::Instance()
{
    if( _instance == NULL )
        _instance = new SarMeter();

    return _instance;
}

SarMeter::SarMeter( void )
    : _bufSize(0)
{
    _input = setupSadc();
    
    _gi.last.gswapbuf = 0;
    _gi.info.swapBuf = 0;
    
    for( int i=0; i<MAX_DISKS; i++ )
    {
        _di.last[i].stat.io_bcnt  = 0;
        _di.last[i].stat.io_wbcnt = 0;
        
        _di.info.nDevices = 0;
        _di.info.read[i] = 0;
        _di.info.write[i] = 0;
    }
}

bool SarMeter::readLine( void )
{
    while( _bufSize < BUFSIZE-100 )
    {
        int ret = read( _input, &_buf[_bufSize], 100);

        if( ret < 0 )
            return false; // error equals eof in our case

        _bufSize += ret;

        if( ret < 100 ) // eof reached
            return false;
    }

    // buffer full
    return true;
}

void SarMeter::checkSadc( void )
{
    bool dataInPipe = true;

    while( dataInPipe )
    {
        dataInPipe = readLine();
        parseBuffer();
    }
}

void SarMeter::parseBuffer( void )
{
    while( _bufSize > 100 )
    {
        // look for 'S' (starting of Sarmagic)
        char *ptr = (char *)memchr( _buf, 'S', _bufSize );
        
        // not found -- discard this buffer
        if( ptr == NULL )
        {
            _bufSize = 0;
            return;
        }

        // not enough data read
        if( (ptr+100) > (_buf+_bufSize) )
            return;


        // check for SarmagicGFX
        if( memcmp( ptr, "SarmagicGFX", 11 ) == 0 )
        {
            forwardBufferTo( ptr );

            // data is not complete in buffer
            if( _bufSize < 24 + sizeof( gfxinfo ))
                return;

            // retrieve gfxinfo structure
            ptr = _buf + 24;
            memcpy( &_gi.current, ptr, sizeof( gfxinfo ));
            ptr += sizeof( gfxinfo );

            forwardBufferTo( ptr );
            newGfxInfo();
        }
        // check for SarmagicNEODISK
        else if( memcmp( ptr, "SarmagicNEODISK", 15 ) == 0 )
        {
            forwardBufferTo( ptr );

            ptr = _buf + 20;

            // number of records [devices]
            int num;
            memcpy( &num, ptr, 4 );
            ptr += 4;

            if( num > MAX_DISKS ) num = MAX_DISKS;

            // data is not complete in buffer
            if( _bufSize < 24 + num*sizeof( diskinfo ))
                return;

            _di.info.nDevices = num;

            // read disk info
            for( int i=0; i<num; i++ )
            {
                memcpy( &_di.current[i], ptr, sizeof( diskinfo ));
                ptr += sizeof( diskinfo );
            }
            
            forwardBufferTo( ptr );
            newDiskInfo();
        }
        else // no known Sarmagic record
            forwardBufferTo( ptr+1 );
    }
}

void SarMeter::newGfxInfo( void )
{
    if( _gi.last.gswapbuf == 0 )
    {
        _gi.last.gswapbuf = _gi.current.gswapbuf;
        return;
    }

    _gi.info.swapBuf = _gi.current.gswapbuf - _gi.last.gswapbuf;
    _gi.last.gswapbuf = _gi.current.gswapbuf;
}

void SarMeter::newDiskInfo( void )
{
    for( int i=0; i<_di.info.nDevices; i++ )
    {
        _di.info.read[i]  = 
            (_di.current[i].stat.io_bcnt - _di.current[i].stat.io_wbcnt) - 
            (_di.last[i].stat.io_bcnt    - _di.last[i].stat.io_wbcnt) ;

        _di.info.write[i] = 
            _di.current[i].stat.io_wbcnt - _di.last[i].stat.io_wbcnt;

        _di.info.read[i] *= 512;
        _di.info.write[i] *= 512;

        _di.last[i].stat.io_bcnt  = _di.current[i].stat.io_bcnt;
        _di.last[i].stat.io_wbcnt = _di.current[i].stat.io_wbcnt;        
    }
}

void SarMeter::forwardBufferTo( char *ptr )
{
    size_t moveBytes = ptr-_buf;
    size_t bytesLeft = _bufSize - moveBytes;
    memmove( _buf, ptr, bytesLeft );
    _bufSize = bytesLeft;
};


// starts /usr/bin/sar, from where data is read
int SarMeter::setupSadc( void )
{
    char    sarPath[] = "/usr/lib/sa/sadc";
    int fd[2];
    int input = 0;

    if (pipe(fd) == -1)
    {
        perror("setupSar: pipe");
        return 0;
    }

    if ( fork()==0 )    // child
    {
        close(1);       // move fd[write] to stdout
        dup(fd[1]);
        close(fd[1]);
        close(fd[0]);   // close other end of the pipe
        close(2);       // close stderr
        setbuf(stdout,NULL); // unbuffered stdout

        // sar wants number of loops: 31536000 is one year
        if (execlp (sarPath, sarPath, "1", "31536000", 0) == -1)
            perror("setupSar: exec sar");

        // not reached
        exit(0);
    }

    input = fd[0];
    close(fd[1]);   // Close other end of the pipe

    fcntl( input, F_SETFL, FNONBLK );

    return input;
}
