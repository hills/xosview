//  
// $Id$
//  Initial port performed by Stefan Eilemann (eile@sgi.com)
//

#include "sarmeter.h"

#include <fcntl.h>

size_t SarMeter::readLine( int input, char *buf, size_t max )
{
    int readBytes, ret;

    // use fgets ??

    for( readBytes = 0 ; readBytes < max-100; )
    {
        ret = read( input, &buf[readBytes], 100);
        readBytes += ret;

        if( ret < 100 )
            return readBytes;
    }
    return readBytes;
}


// starts /usr/bin/sar, from where data is read from
int SarMeter::setupSar( const char *option )
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
