//  
// $Id$
//  Initial port performed by Stefan Eilemann (eile@sgi.com)
//
#include "MeterMaker.h"
#include "xosview.h"

#include "loadmeter.h"

#include "cpumeter.h"
#include "memmeter.h"
#if 0
#include "swapmeter.h"
#include "pagemeter.h"
#endif

#include <stdlib.h>


MeterMaker::MeterMaker(XOSView *xos)
{
    _xos = xos;
}

void MeterMaker::makeMeters(void)
{
    if (_xos->isResourceTrue("load"))
        push(new LoadMeter(_xos));

    // Standard meters (usually added, but users could turn them off)
    if (_xos->isResourceTrue("cpu")) {
        bool any = 0;
        int cpuCount = CPUMeter::countCPUs();

        if( strncmp( _xos->getResource("cpuFormat"), "single", 2) == 0 || 
            strncmp( _xos->getResource("cpuFormat"), "both", 2) == 0 )
        {
            push(new CPUMeter(_xos, -1));
            any = 1;
        }

        if( strncmp( _xos->getResource("cpuFormat"), "all", 2) == 0 || 
            strncmp( _xos->getResource("cpuFormat"), "both", 2) == 0 )
        {
            for (int i = 0 ; i < cpuCount ; i++)
                push(new CPUMeter(_xos, i));
            any = 1;
        }

        if( strncmp( _xos->getResource("cpuFormat"), "auto", 2) == 0 )
        {
            push(new CPUMeter(_xos, -1));

            if( cpuCount>1 )
                for (int i = 0 ; i < cpuCount ; i++)
                    push(new CPUMeter(_xos, i));
            any = 1;
        }

        if( !any )
        {
            int cpuCount = CPUMeter::countCPUs();
            for (int i = 0 ; i < cpuCount ; i++)
                push(new CPUMeter(_xos, i));
        }            

    }

    if (_xos->isResourceTrue("mem"))
        push(new MemMeter(_xos));

#if 0
    if (_xos->isResourceTrue("swap"))
        push(new SwapMeter(_xos, kc));

    if (_xos->isResourceTrue("page"))
        push(new PageMeter(_xos, kc,
            atof(_xos->getResource("pageBandwidth"))));
#endif
}

#if 0 
// starts /usr/lib/sa/sadc, from where certain meters read data
int MeterMaker::setupSadc()
{
    char    sadcPath[] = "/usr/lib/sa/sadc";
    int fd[2];
    int input = 0;

    if (pipe(fd) == -1)
        perror("setupSadc: pipe");

    if ( fork()==0 )    // child
    {
        close(1);       // move fd[write] to stdout
        dup(fd[1]);
        close(fd[0]);   // close other end of the pipe

        // sadc wants number of loops: 31536000 is one year
        if (execlp (sadc_path, sadc_path, "1", "31536000", 0) == -1)
            perror("setupSadc: exec sadc");
    }

    input = fd[0];
    close(fd[1]);   // Close other end of the pipe

    return input;
}


#endif
