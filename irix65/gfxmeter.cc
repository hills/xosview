//  
// $Id$
//  Initial port performed by Stefan Eilemann (eile@sgi.com)
//

#include "gfxmeter.h"
#include "xosview.h"

#include <stdlib.h>
#include <unistd.h>
#include <invent.h>

// GfxMeter display swapbuffers per second. max is base rate for one gfx pipe.

GfxMeter::GfxMeter(XOSView *parent, int max)
        : FieldMeterGraph(parent, 2, "GFX","SWAPBUF/S", 1, 1, 0 ),
          SarMeter(),
          lastPos(0),
          lastSwapBuf(0)
{
    inventory_t *inv;

    lastalarmstate = -1;
    total_ = 0;
    nPipes = 0;

    setinvent();
    for( inv = getinvent(); inv != NULL; inv = getinvent() )
    {
        if ( inv->inv_class==INV_GRAPHICS )
        {
            nPipes++;
        }
    }
    total_ = nPipes * max;

    input = setupSar( "-g" );

    if ( total_==0 || input==0 ) {
        parent_->done(1);
        return;
    }
}

GfxMeter::~GfxMeter(void)
{
}

void GfxMeter::checkResources(void)
{
    FieldMeterGraph::checkResources();

    swapgfxcol_ = parent_->allocColor(parent_->getResource( "gfxSwapColor" ));
    warngfxcol_ = parent_->allocColor(parent_->getResource( "gfxWarnColor" ));
    critgfxcol_ = parent_->allocColor(parent_->getResource( "gfxCritColor" ));

    setfieldcolor(0, parent_->getResource( "gfxSwapColor" ));
    setfieldcolor(1, parent_->getResource("gfxIdleColor"));
    priority_ = atoi (parent_->getResource("gfxPriority"));
    useGraph_ = parent_->isResourceTrue("gfxGraph");
    dodecay_ = parent_->isResourceTrue( "gfxDecay" );
    SetUsedFormat(parent_->getResource("gfxUsedFormat"));

    warnThreshold = atoi (parent_->getResource("gfxWarnThreshold")) * nPipes;
    critThreshold = atoi (parent_->getResource("gfxCritThreshold")) * nPipes;

    if (dodecay_){
        //  Warning:  Since the gfxmeter changes scale occasionally, old
        //  decay values need to be rescaled.  However, if they are rescaled,
        //  they could go off the edge of the screen.  Thus, for now, to
        //  prevent this whole problem, the gfx meter can not be a decay
        //  meter.  The gfx is a decaying average kind of thing anyway,
        //  so having a decaying gfx average is redundant.
        cerr << "Warning:  The gfxmeter can not be configured as a decay\n"
             << "  meter. See the source code (" <<__FILE__<< ") for further\n"
             << "  details.\n";
        dodecay_ = 0;
    }
}

void GfxMeter::checkevent(void)
{
    getgfxinfo();
    drawfields();
}

void GfxMeter::getgfxinfo(void)
{
    if( lastPos >= 50000 )
        lastPos = 0;

    size_t remaining = 50000 - lastPos;
    size_t currPos = 0;
    int    found = 0;

    lastPos += readLine( input, &buf[lastPos], remaining );

    while( currPos < lastPos )
    {
        remaining = lastPos-currPos;

//        fprintf( stderr, "currPos %d, lastPos %d, remaining %d\n", currPos,
//            lastPos, remaining );

        char *ptr = (char *)memchr( &buf[currPos], 'S', remaining );
        
        if( ptr == NULL )
            break;

        if( ptr < buf+lastPos+24+sizeof(gfxinfo) &&
            memcmp( ptr, "SarmagicGFX", 11 ) == 0 )
        {
            ptr += 24;
//            fprintf( stderr, "found gfxinfo pos at 0x%x, %d bytes in buf\n",
//                ptr, ptr-buf );
            memcpy( &gi, ptr, sizeof( gfxinfo ));
//            fprintf( stderr, "swp %d\n", gi.gswapbuf );
            found = 1;

            // found record, move data
            ptr += sizeof( gfxinfo );
            size_t pos = ptr-buf;
            
//            fprintf( stderr, "memmove 0x%x, 0x%x, %d (% d - %d)\n",  buf, ptr,
//                lastPos - pos, lastPos, pos );

            memmove( buf, ptr, lastPos - pos );
            lastPos = lastPos - pos;
            break;
        }
                
        currPos = ( (char *)ptr - buf ) + 1;
    }
    
    if( !found )
        return;

    // got data
    unsigned int swapBuf = gi.gswapbuf - lastSwapBuf;
    if( swapBuf > 1000 )
        swapBuf = 0;

    lastSwapBuf = gi.gswapbuf;


    fields_[0] = (float) swapBuf;
    
    if ( fields_[0] < warnThreshold ) 
        alarmstate = 0;
    else
        if ( fields_[0] >= critThreshold )
            alarmstate = 2;
        else
            alarmstate = 1;
  
    if ( alarmstate != lastalarmstate )
    {
        if ( alarmstate == 0 )
            setfieldcolor( 0, swapgfxcol_ );
        else
            if ( alarmstate == 1 )
                setfieldcolor( 0, warngfxcol_ );
            else
                setfieldcolor( 0, critgfxcol_ );
        if (dolegends_)
            drawlegend();
        lastalarmstate = alarmstate;
    }
  
    if ( fields_[0]*5.0<total_ )
        total_ = fields_[0];
    else
        if ( fields_[0]>total_ )
            total_ = fields_[0]*5.0;
      
    if ( total_ < 1.0)
        total_ = 1.0;
    
    fields_[1] = (float) (total_ - fields_[0]);

    setUsed(fields_[0], (float) 1.0);
}
