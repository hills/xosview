//
//  Copyright (c) 1994, 1995, 2002, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include "general.h"
#include "xosview.h"
#include "meter.h"
#include "MeterMaker.h"
#if (defined(XOSVIEW_NETBSD) || defined(XOSVIEW_FREEBSD) || defined(XOSVIEW_OPENBSD))
#include "kernel.h"
#endif

static const char * const versionString = "xosview version: Git";

static const char NAME[] = "xosview@";

#if !defined(__GNUC__)

#define MIN(x,y)		\
(				\
    x < y ? x : y		\
)

#define MAX(x,y)		\
(				\
    x > y ? x : y		\
)

#else

#define MIN(x,y)		\
({				\
    const typeof(x) _x = x;	\
    const typeof(y) _y = y;	\
				\
    (void) (&_x == &_y); 	\
				\
    _x < _y ? _x : _y;		\
})

#define MAX(x,y)		\
({				\
    const typeof(x) _x = x;	\
    const typeof(y) _y = y;	\
				\
    (void) (&_x == &_y); 	\
				\
    _x > _y ? _x : _y;		\
})

#endif // sgi

double MAX_SAMPLES_PER_SECOND = 10;

XOSView::XOSView( const char * instName, int argc, char *argv[] ) : XWin(),
						xrm(Xrm("xosview", instName)){
  // Check for version arguments first.  This allows
  // them to work without the need for a connection
  // to the X server
  checkVersion(argc, argv);

  setDisplayName (xrm.getDisplayName( argc, argv));
  openDisplay();  //  So that the Xrm class can contact the display for its
		  //  default values.
  //  The resources need to be initialized before calling XWinInit, because
  //  XWinInit looks at the geometry resource for its geometry.  BCG
  xrm.loadAndMergeResources (argc, argv, display_);
  XWinInit (argc, argv, NULL, &xrm);
#if 1	//  Don't enable this yet.
  MAX_SAMPLES_PER_SECOND = atof(getResource("samplesPerSec"));
  if (!MAX_SAMPLES_PER_SECOND)
    MAX_SAMPLES_PER_SECOND = 10;
#endif
  usleeptime_ = (unsigned long) (1000000/MAX_SAMPLES_PER_SECOND);
  if (usleeptime_ >= 1000000) {
    /*  The syscall usleep() only takes times less than 1 sec, so
     *  split into a sleep time and a usleep time if needed.  */
    sleeptime_ = usleeptime_ / 1000000;
    usleeptime_ = usleeptime_ % 1000000;
  } else { sleeptime_ = 0; }
#if (defined(XOSVIEW_NETBSD) || defined(XOSVIEW_FREEBSD) || defined(XOSVIEW_OPENBSD))
  BSDInit();	/*  Needs to be done before processing of -N option.  */
#endif

  hmargin_  = atoi(getResource("horizontalMargin"));
  vmargin_  = atoi(getResource("verticalMargin"));
  vspacing_ = atoi(getResource("verticalSpacing"));
  hmargin_  = MAX(0, hmargin_);
  vmargin_  = MAX(0, vmargin_);
  vspacing_ = MAX(0, vspacing_);

  checkArgs (argc, argv);  //  Check for any other unhandled args.
  xoff_ = hmargin_;
  yoff_ = 0;
  nummeters_ = 0;
  meters_ = NULL;
  name_ = const_cast<char *>("xosview");
  _isvisible = false;
  _ispartiallyvisible = false;
  exposed_once_flag_ = 0;

  expose_flag_ = 1;

  //  set up the X events
  addEvent( new Event( this, ConfigureNotify,
		      (EventCallBack)&XOSView::resizeEvent ) );
  addEvent( new Event( this, Expose,
		      (EventCallBack)&XOSView::exposeEvent ) );
  addEvent( new Event( this, KeyPress,
		      (EventCallBack)&XOSView::keyPressEvent ) );
  addEvent( new Event( this, VisibilityNotify,
                      (EventCallBack)&XOSView::visibilityEvent ) );
  addEvent( new Event( this, UnmapNotify,
                      (EventCallBack)&XOSView::unmapEvent ) );

  // add or change the Resources
  MeterMaker mm(this);

  // see if legends are to be used
  checkOverallResources ();

  // add in the meters
  mm.makeMeters();
  for (int i = 1 ; i <= mm.n() ; i++)
    addmeter(mm[i]);

  if (nummeters_ == 0)
  {
    fprintf (stderr, "No meters were enabled!  Exiting...\n");
    exit (0);
  }

  //  Have the meters re-check the resources.
  checkMeterResources();

  // determine the width and height of the window then create it
  figureSize();
  init( argc, argv );
  title( winname() );
  iconname( winname() );
  dolegends();
  resize();
}

void XOSView::checkVersion(int argc, char *argv[]) const
    {
    for (int i = 0 ; i < argc ; i++)
        if (!strncasecmp(argv[i], "-v", 2)
          || !strncasecmp(argv[i], "--version", 10))
            {
            std::cerr << versionString << std::endl;
            exit(0);
            }
    }

void XOSView::figureSize ( void ) {
  if ( legend_ ){
    if ( !usedlabels_ )
      xoff_ = textWidth( "XXXXX" );
    else
      xoff_ = textWidth( "XXXXXXXXX" );

    yoff_ = caption_ ? textHeight() + textHeight() / 4 : 0;
  }
  static int firsttime = 1;
  if (firsttime) {
    firsttime = 0;
    width_ = findx();
    height_ = findy();
  }
  else
  {
  }
}

void XOSView::checkMeterResources( void ){
  MeterNode *tmp = meters_;

  while ( tmp != NULL ){
    tmp->meter_->checkResources();
    tmp = tmp->next_;
  }
}

int XOSView::newypos( void ){
  return 15 + 25 * nummeters_;
}

void XOSView::dolegends( void ){
  MeterNode *tmp = meters_;
  while ( tmp != NULL ){
    tmp->meter_->docaptions( caption_ );
    tmp->meter_->dolegends( legend_ );
    tmp->meter_->dousedlegends( usedlabels_ );
    tmp = tmp->next_;
  }
}

void XOSView::addmeter( Meter *fm ){
  MeterNode *tmp = meters_;

  if ( meters_ == NULL )
    meters_ = new MeterNode( fm );
  else {
    while ( tmp->next_ != NULL )
      tmp = tmp->next_;
    tmp->next_ = new MeterNode( fm );
  }
  nummeters_++;
}

int XOSView::findx( void ){
  if ( legend_ ){
    if ( !usedlabels_ )
      return textWidth( "XXXXXXXXXXXXXXXXXXXXXXXX" );
    else
      return textWidth( "XXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
  }
  return 80;
}

int XOSView::findy( void ){
  if ( legend_ )
    return 10 + textHeight() * nummeters_ * ( caption_ ? 2 : 1 );

  return 15 * nummeters_;
}

void XOSView::checkOverallResources() {
  //  Check various resource values.

  //  Set 'off' value.  This is not necessarily a default value --
  //    the value in the defaultXResourceString is the default value.
  usedlabels_ = legend_ = caption_ = 0;

  setFont();

   // use captions
  if ( isResourceTrue("captions") )
      caption_ = 1;

  // use labels
  if ( isResourceTrue("labels") )
      legend_ = 1;

  // use "free" labels
  if ( isResourceTrue("usedlabels") )
      usedlabels_ = 1;
}

const char *XOSView::winname( void ){
  char host[100];
  gethostname( host, 99 );
  static char name[101];	/*  We return a pointer to this,
				    so it can't be local.  */
  snprintf( name, 100, "%s%s", NAME, host);
  //  Allow overriding of this name through the -title option.
  return getResourceOrUseDefault ("title", name);
}


void  XOSView::resize( void ){
  int spacing = vspacing_+1;
  int topmargin = vmargin_;
  int rightmargin = hmargin_;
  int newwidth = width_ - xoff_ - rightmargin;
/*
  int newheight = (height_ - (10 + 5 * (nummeters_ - 1) +
                              nummeters_ * yoff_)) / nummeters_;
*/
  int newheight =
        (height_ -
         (topmargin + topmargin + (nummeters_-1)*spacing + nummeters_*yoff_)
        ) / nummeters_;
  newheight = (newheight >= 2) ? newheight : 2;


  int counter = 1;
  MeterNode *tmp = meters_;
  while ( tmp != NULL ) {
/*
    tmp->meter_->resize( xoff_, 5 * counter + counter * yoff_ +
                         (counter - 1) * newheight, newwidth, newheight );
*/
    tmp->meter_->resize( xoff_,
                         topmargin + counter*yoff_ + (counter-1)*(newheight+spacing),
                        newwidth, newheight );
    tmp = tmp->next_;

    counter++;
  }
}


XOSView::~XOSView( void ){
  MeterNode *tmp = meters_;
  while ( tmp != NULL ){
    MeterNode *save = tmp->next_;
    delete tmp->meter_;
    delete tmp;
    tmp = save;
  }
}

void XOSView::reallydraw( void ){
  XOSDEBUG("Doing draw.\n");
  clear();
  MeterNode *tmp = meters_;

  while ( tmp != NULL ){
    tmp->meter_->draw();
    tmp = tmp->next_;
  }
  flush();

  expose_flag_ = 0;
}

void XOSView::draw ( void ) {
  if (hasBeenExposedAtLeastOnce() && isAtLeastPartiallyVisible())
    reallydraw();
  else {
    if (!hasBeenExposedAtLeastOnce()) {
      XOSDEBUG("Skipping draw:  not yet exposed.\n");
    } else if (!isAtLeastPartiallyVisible()) {
      XOSDEBUG("Skipping draw:  not visible.\n");
    }
  }
}
void XOSView::keyrelease( char *ch ){
/*  WARNING:  This code is not called by anything.  */
(void) ch;  /*  To avoid gcc warnings.  */
}

void XOSView::run( void ){
  int counter = 0;

  while( !done_ ){
    checkevent();

    if (_isvisible){
      MeterNode *tmp = meters_;
      while ( tmp != NULL ){
        if ( tmp->meter_->requestevent() )
          tmp->meter_->checkevent();
        tmp = tmp->next_;
      }

      flush();
    }
#ifdef HAVE_USLEEP
    /*  First, sleep for the proper integral number of seconds --
     *  usleep only deals with times less than 1 sec.  */
    if (sleeptime_) sleep((unsigned int)sleeptime_);
    if (usleeptime_) usleep( (unsigned int)usleeptime_);
#else
    usleep_via_select ( usleeptime_ );
#endif
    counter = (counter + 1) % 5;
  }
}

void XOSView::usleep_via_select( unsigned long usec ){
  struct timeval time;

  time.tv_sec = (int)(usec / 1000000);
  time.tv_usec = usec - time.tv_sec * 1000000;

  select( 0, 0, 0, 0, &time );
}

void XOSView::keyPressEvent( XKeyEvent &event ){
  char c = 0;
  KeySym key;

  XLookupString( &event, &c, 1, &key, NULL );

  if ( (c == 'q') || (c == 'Q') )
    done_ = 1;
}

void XOSView::checkArgs (int argc, char** argv) const
{
  //  The XWin constructor call in the XOSView constructor above
  //  modifies argc and argv, so by this
  //  point, all XResource arguments should be removed.  Since we currently
  //  don't have any other command-line arguments, perform a check here
  //  to make sure we don't get any more.
  if (argc == 1) return;  //  No arguments besides X resources.

  //  Skip to the first real argument.
  argc--;
  argv++;
  while (argc > 0 && argv && *argv)
  {
    switch (argv[0][1]) {
      case 'n': //  Check for -name option that was already parsed
		//  and acted upon by main().
		if (!strncasecmp(*argv, "-name", 6))
		{
		  argv++;	//  Skip arg to -name.
		  argc--;
		}
		break;
#if (defined(XOSVIEW_NETBSD) || defined(XOSVIEW_FREEBSD) || defined(XOSVIEW_OPENBSD))
      case 'N': if (strlen(argv[0]) > 2)
      		  SetKernelName(argv[0]+2);
		else
		{
		  SetKernelName(argv[1]);
		  argc--;
		  argv++;
		}
		break;
#endif
	      /*  Fall through to default/error case.  */
      default:
      		std::cerr << "Ignoring unknown option '" << argv[0] << "'.\n";
	  	break;
    }
    argc--;
    argv++;
  }
}

void XOSView::exposeEvent( XExposeEvent &event ) {
  _isvisible = true;
  if ( event.count == 0 )
  {
    expose_flag_++;
    draw();
  }
  XOSDEBUG("Got expose event.\n");
  if (!exposed_once_flag_) { exposed_once_flag_ = 1; draw(); }
}

void XOSView::resizeEvent( XEvent & ) {
  resize();
  expose_flag_++;
  draw();
}


void XOSView::visibilityEvent( XVisibilityEvent &event ){
  _ispartiallyvisible = false;
  if (event.state == VisibilityPartiallyObscured){
    _ispartiallyvisible = true;
  }

  if (event.state == VisibilityFullyObscured){
    _isvisible = false;
  }
  else {
    _isvisible = true;
  }
  XOSDEBUG("Got visibility event; %d and %d\n",
      _ispartiallyvisible, _isvisible);
}

void XOSView::unmapEvent( XUnmapEvent & ev ){
  /* unclutter creates a subwindow of our window if it hides the cursor,
     we get the unmap event if the cursor is moved again. Don't treat it
     as main window unmap */
  if(ev.window == window_)
    _isvisible = false;
}
