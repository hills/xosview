//  
// $Id$
//  Initial port performed by Stefan Eilemann (eile@sgi.com)
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

size_t SarMeter::readLine( int input, char *buf, size_t max )
{
    int readBytes, ret;

    // use fgets ??

    for( readBytes = 0 ; readBytes < max-100; )
    {
        ret = read( input, &buf[readBytes], 100);
        if( ret < 0 )
            return readBytes;

        readBytes += ret;

        if( ret < 100 )
            return readBytes;
    }
    return readBytes;
}

void SarMeter::checkSadc( void )
{
    if( _lastPos >= 50000 )
        _lastPos = 0;

    size_t remaining = 50000 - _lastPos;
    size_t currPos = 0;

    _lastPos += readLine( _input, &_buf[_lastPos], remaining );

    while( currPos < _lastPos )
    {
        remaining = _lastPos-currPos;

//        fprintf( stderr, "currPos %d, _lastPos %d, remaining %d\n", currPos,
//            _lastPos, remaining );

        char *ptr = (char *)memchr( &_buf[currPos], 'S', remaining );
        
        if( ptr == NULL )
            break;

        if( (ptr+24+sizeof(gfxinfo)) < (_buf+_lastPos) &&
            memcmp( ptr, "Sarmagic", 8 ) == 0 )
        {
            ptr += 8;
            
            if( memcmp( ptr, "GFX", 3 ) == 0 )
            {
                // already found a new record
                if( _giNew )
                    return;

                ptr += 16;
//              fprintf( stderr,"found gfxinfo pos at 0x%x, %d bytes in _buf\n",
//                    ptr, ptr-_buf );
                memcpy( &_gi, ptr, sizeof( gfxinfo ));
//                fprintf( stderr, "swp %d\n", _gi.gswapbuf );
                _giNew = 1;

                // found record, move data
                ptr += sizeof( gfxinfo );
                size_t pos = ptr-_buf;
                
//                fprintf( stderr, 
//                    "memmove gfx 0x%x, 0x%x, %d[0x%x] (%d[0x%x] - %d[0x%x])\n",
//                    _buf, ptr, _lastPos - pos, _lastPos - pos, _lastPos,
//                    _lastPos, pos, pos );

                memmove( _buf, ptr, _lastPos - pos );
                _lastPos = _lastPos - pos;
                
                currPos = 0;
            }
            else if( memcmp( ptr, "NEODISK", 7 ) == 0 )
            {
                // already found a new record
                if( _diNew )
                    return;

                ptr += 16;
//              fprintf(stderr,"found diskinfo pos at 0x%x, %d bytes in _buf\n",
//                    ptr, ptr-_buf );
                memcpy( &_di, ptr, sizeof( diskinfo ));
#if 0
                fprintf( stderr, "diskinfo {\n  %s\n", _di.name );
                fprintf( stderr, "  stat {\n" );
                fprintf( stderr, "    ios {\n" );
                fprintf( stderr, "      %d, %d, %d, %d\n",
                    _di.stat.ios.io_ops,
                    _di.stat.ios.io_misc,
                    _di.stat.ios.io_qcnt,
                    _di.stat.ios.io_unlog );
                fprintf( stderr, "    }\n" );
                fprintf( stderr, "    %d %d %d %d %d\n", 
                    _di.stat.io_bcnt,
                    _di.stat.io_resp,
                    _di.stat.io_act,
                    _di.stat.io_wops,
                    _di.stat.io_wbcnt );
#endif
//                _diNew = 1;

                // found record, move data
                ptr += sizeof( diskinfo );
                size_t pos = ptr-_buf;
                
//                fprintf( stderr,
//                    "memmove dsk 0x%x, 0x%x, %d[0x%x] (%d[0x%x] - %d[0x%x])\n",
//                    _buf, ptr, _lastPos - pos, _lastPos - pos, _lastPos,
//                    _lastPos, pos, pos );

                memmove( _buf, ptr, _lastPos - pos );
                _lastPos = _lastPos - pos;
                
                currPos = 0;
            }
            else
                currPos = ( (char *)ptr - _buf ) + 1;
        }
        else
            currPos = ( (char *)ptr - _buf ) + 1;
    }
}
// starts /usr/bin/sar, from where data is read from
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
