#include "xwin.h"
#include "Xrm.h"
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


//-----------------------------------------------------------------------------

//  argc is a reference, so that the changes to argc by XrmParseCommand are
//  noticed by the caller (XOSView, in this case).  BCG
XWin::XWin(){
}
//-----------------------------------------------------------------------------

XWin::XWin( int argc, char *argv[], int x, int y, int width, int height ){
  fprintf (stderr, "This constructor call is not supported!  (%s:%d)\n",
    __FILE__, __LINE__);
  exit (-1);
  //  FIXME BCG  This constructor needs to do much of the work of the above
  //  one.  Or, we need to drop this as a constructor.  As it is, it is
  //  VERY MUCH out of date.
  x_ = x;
  y_ = y;
  width_ = width;
  height_ = height;
  (void) argc;
  (void) argv;
}
//-----------------------------------------------------------------------------

void XWin::XWinInit (int argc, char** argv, char* geometry, Xrm* xrm) {
  (void) argc;
  (void) argv;  //  Avoid gcc warnings about unused variables.
  //  Eventually, we may want to have XWin handle some arguments other
  //  than resources, so argc and argv are left as parameters.  BCG

  geometry_ = geometry;  //  Save for later use.
  width_ = height_ = x_ = y_ = 0;
  xrmptr_ = xrm;

  name_ = "";
  font_ = NULL;
  done_ = 0;  
  
  // Set up the default Events
  events_ = NULL;
  addEvent( new Event( this, ConfigureNotify, configureEvent ) );
  addEvent( new Event( this, ClientMessage, deleteEvent ) );
  addEvent( new Event( this, MappingNotify, mappingNotify ) );

  //openDisplay();  //  Done explicitly in xosview.cc.
}

XWin::~XWin( void ){

  // delete the events
  Event *event = events_;
  while ( event != NULL ){
    Event *save = event->next_;
    delete event;
    event = save;
  }
  
  XFree( title_.value );
  XFree( iconname_.value );
  XFree( sizehints_ );
  XFree( wmhints_ );
  XFree( classhints_ );
  XFreeGC( display_, gc_ );
  XFreeFont( display_, font_ );
  XDestroyWindow( display_, window_ );
  // close the connection to the display
  XCloseDisplay( display_ );
}
//-----------------------------------------------------------------------------

void XWin::init( int argc, char **argv ){
  XGCValues            gcv;
  XSetWindowAttributes xswa;

  setFont();
  setColors();
  getGeometry();
  
  window_ = XCreateSimpleWindow(display_, DefaultRootWindow(display_), 
				sizehints_->x, sizehints_->y,
				sizehints_->width, sizehints_->height, 
				1,
				fgcolor_, bgcolor_);

  setHints( argc, argv );

  // Finally, create a graphics context for the main window
  gcv.font = font_->fid;
  gcv.foreground = fgcolor_;
  gcv.background = bgcolor_;
  gc_ = XCreateGC(display_, window_,
		  (GCFont | GCForeground | GCBackground), &gcv);
                
  // Set main window's attributes (colormap, bit_gravity)
  xswa.colormap = colormap_;
  xswa.bit_gravity = NorthWestGravity;
  XChangeWindowAttributes(display_, window_,
			  (CWColormap | CWBitGravity), &xswa);

  // add the events
  Event *tmp = events_;
  while ( tmp != NULL ){
    selectEvents( tmp->mask_ );
    tmp = tmp->next_;
  }

  // Map the main window
  map();
  flush();
  if(XGetWindowAttributes(display_, window_, &attr_) == 0){
    cerr <<"Error getting attributes of Main." <<endl;
    exit(2);
  }
}
//-----------------------------------------------------------------------------

void XWin::setFont( void ){
  // Set up the font
  if ( font_ != NULL )
    return;
  const char* fontName = getResource("font");

  if ((font_ = XLoadQueryFont(display_, fontName)) == NULL){
    cerr <<name_ <<": display " <<DisplayString(display_)
      <<" cannot load font " << fontName << endl;
    exit(1);
  }
}
//-----------------------------------------------------------------------------

void XWin::setHints( int argc, char *argv[] ){
  // Set up class hint
  if((classhints_ = XAllocClassHint()) == NULL){
    cerr <<"Error allocating class hint!" <<endl;
    exit(1);
  }
  classhints_->res_name = name_;
  classhints_->res_class = name_;

  // Set up the window manager hints
  if((wmhints_ = XAllocWMHints()) == NULL){
    cerr <<"Error allocating Window Manager hints!" <<endl;
    exit(1);
  }
  wmhints_->flags = (InputHint|StateHint);
  wmhints_->input = True;
  wmhints_->initial_state = NormalState;

  // Set up XTextProperty for window name and icon name
  if(XStringListToTextProperty(&name_, 1, &title_) == 0){
    cerr <<"Error creating XTextProperty!" <<endl;
    exit(1);
  }
  if(XStringListToTextProperty(&name_, 1, &iconname_) == 0){
    cerr <<"Error creating XTextProperty!" <<endl;
    exit(1);
  }

  XSetWMProperties(display_, window_, &title_, &iconname_, argv, argc,
		   sizehints_, wmhints_, classhints_);

  // Set up the Atoms for delete messages
  wm_ = XInternAtom( display(), "WM_PROTOCOLS", False );
  wmdelete_ = XInternAtom( display(), "WM_DELETE_WINDOW", False );
  XChangeProperty( display(), window(), wm_, XA_ATOM, 32,
		   PropModeReplace, (unsigned char *)(&wmdelete_), 1 );
}
//-----------------------------------------------------------------------------

void XWin::openDisplay( void ){
  // Open connection to display selected by user
  if ((display_ = XOpenDisplay (display_name_)) == NULL) {
    cerr <<"Can't open display named " << display_name_ <<endl;
    exit(1);
  }

  colormap_ = DefaultColormap( display_, screen() );  
}
//-----------------------------------------------------------------------------

void XWin::setColors( void ){
  XColor               color;  
  
  // Main window's background color
  if (XParseColor(display_, colormap_, getResource("background"),
		  &color) == 0 ||
      XAllocColor(display_, colormap_, &color) == 0)
    bgcolor_ = WhitePixel(display_, DefaultScreen(display_));
  else
    bgcolor_ = color.pixel;

  // Main window's foreground color */
  if (XParseColor(display_, colormap_, getResource("foreground"),
		  &color) == 0 ||
      XAllocColor(display_, colormap_, &color) == 0)
    fgcolor_ = BlackPixel(display_, DefaultScreen(display_));
  else
    fgcolor_ = color.pixel;
}
//-----------------------------------------------------------------------------

void XWin::getGeometry( void ){
  char                 default_geometry[80];
  int                  bitmask;

  // Fill out a XsizeHints structure to inform the window manager
  // of desired size and location of main window.
  if((sizehints_ = XAllocSizeHints()) == NULL){
    cerr <<"Error allocating size hints!" <<endl;
    exit(1);
  }
  sizehints_->flags = PSize;
  sizehints_->height = height_;
  sizehints_->min_height = sizehints_->height;
  sizehints_->width = width_;
  sizehints_->min_width = sizehints_->width;
  sizehints_->x = x_;
  sizehints_->y = y_;

  // Construct a default geometry string
  sprintf(default_geometry, "%dx%d+%d+%d", sizehints_->width,
	  sizehints_->height, sizehints_->x, sizehints_->y);

  // Process the geometry specification
  bitmask =  XGeometry(display_, DefaultScreen(display_), 
		       getResourceOrUseDefault("geometry", geometry_), default_geometry,
		       0, 
		       1, 1, 0, 0, &(sizehints_->x), &(sizehints_->y),
		       &(sizehints_->width), &(sizehints_->height));

  // Check bitmask and set flags in XSizeHints structure
  if (bitmask & (XValue | YValue)){
    sizehints_->flags |= PPosition;
    width_ = sizehints_->width;
    height_ = sizehints_->height;
  }
}
//-----------------------------------------------------------------------------

void XWin::selectEvents( long mask ){
  XWindowAttributes    xAttr;
  XSetWindowAttributes xSwAttr;

  if ( XGetWindowAttributes( display_, window_, &xAttr ) != 0 ){
    xSwAttr.event_mask = xAttr.your_event_mask | mask;
    XChangeWindowAttributes( display_, window_, CWEventMask, &xSwAttr );
  }
}

void XWin::ignoreEvents( long mask ){
  XWindowAttributes    xAttr;
  XSetWindowAttributes xSwAttr;

  if ( XGetWindowAttributes( display_, window_, &xAttr ) != 0 ){
    xSwAttr.event_mask = xAttr.your_event_mask & mask;
    XChangeWindowAttributes( display_, window_, CWEventMask, &xSwAttr );
  }
}
//-----------------------------------------------------------------------------

void XWin::checkevent( void ){
  XEvent event;
  
  while ( XEventsQueued( display_, QueuedAfterReading ) ){
    XNextEvent( display_, &event );

    // call all of the Event's call back functions to process this event
    Event *tmp = events_;
    while ( tmp != NULL ){
      tmp->callBack( event );
      tmp = tmp->next_;
    }
  }
}
//-----------------------------------------------------------------------------
#if 0
void XWin::usage( void ){
  //  FIXME  We need to update this.  BCG
#warning The usage() function needs to be updated.
  cerr <<name_ <<" [-display name] [-geometry geom]" <<endl;
//    exit (1);
}
#endif
//-----------------------------------------------------------------------------

void XWin::addEvent( Event *event ){
  Event *tmp = events_;

  if ( events_ == NULL )
    events_ = event;
  else {
    while ( tmp->next_ != NULL )
      tmp = tmp->next_;
    tmp->next_ = event;
  }
}
//-----------------------------------------------------------------------------

const char *XWin::getResourceOrUseDefault( const char *name, const char* defaultVal ){

  const char* retval = xrmptr_->getResource (name);
  if (retval)
    return retval;
  else
    return defaultVal;
}

//-----------------------------------------------------------------------------

const char *XWin::getResource( const char *name ){
  const char* retval = xrmptr_->getResource (name);
  if (retval)
    return retval;
  else
  {
    cerr << "Error:  Couldn't find '" << name << "' resource in the resource database!\n";
    exit (-1);
  }
}

//-----------------------------------------------------------------------------

void XWin::dumpResources( ostream &os ){
cerr << "Function not implemented!\n";  //  BCG  FIXME  Need to make this.
(void) os;  //  Keep gcc happy.
}
//-----------------------------------------------------------------------------

unsigned long XWin::allocColor( const char *name ){
  XColor exact, closest;
  
  if ( XAllocNamedColor( display_, colormap(), name, &closest, &exact ) == 0 )
    cerr <<"XWin::allocColor() : failed to alloc : " <<name <<endl;

  return exact.pixel;
}
//-----------------------------------------------------------------------------

void XWin::configureEvent( XEvent &event ){
  x( event.xconfigure.x );
  y( event.xconfigure.y );
  width( event.xconfigure.width );
  height( event.xconfigure.height );
}
//-----------------------------------------------------------------------------

void XWin::deleteEvent( XEvent &event ){
  if ( (event.xclient.message_type == wm_ ) &&
       ((unsigned)event.xclient.data.l[0] == wmdelete_) )
    done( 1 );
}
//-----------------------------------------------------------------------------

XWin::Event::Event( XWin *parent, int event, EventCallBack callBack ){
  next_ = NULL;
  parent_ = parent;
  event_ = event;
  callBack_ = callBack;

  switch ( event_ ){
  case ButtonPress:
    mask_ = ButtonPressMask;
    break;
  case ButtonRelease:
    mask_ = ButtonReleaseMask;
    break;
  case EnterNotify:
    mask_ = EnterWindowMask;
    break;
  case LeaveNotify:
    mask_ = LeaveWindowMask;
    break;
  case MotionNotify:
    mask_ = PointerMotionMask;
    break;
  case FocusIn:
  case FocusOut:
    mask_ = FocusChangeMask;
    break;
  case KeymapNotify:
    mask_ = KeymapStateMask;
    break;
  case KeyPress:
    mask_ = KeyPressMask;
    break;
  case KeyRelease:
    mask_ = KeyReleaseMask;
    break;
  case MapNotify:
  case SelectionClear:
  case SelectionNotify:
  case SelectionRequest:
  case ClientMessage:
  case MappingNotify:
    mask_ = NoEventMask;
    break;
  case Expose:
  case GraphicsExpose:
  case NoExpose:
    mask_ = ExposureMask;
    break;
  case ColormapNotify:
    mask_ = ColormapChangeMask;
    break;
  case PropertyNotify:
    mask_ = PropertyChangeMask;
    break;
  case UnmapNotify:
  case ReparentNotify:
  case GravityNotify:
  case DestroyNotify:
  case CirculateNotify:
  case ConfigureNotify:
    mask_ = StructureNotifyMask | SubstructureNotifyMask;
    break;
  case CreateNotify:
    mask_ = SubstructureNotifyMask;
    break;
  case VisibilityNotify:
    mask_ = VisibilityChangeMask;
    break;
  // The following are used by window managers
  case CirculateRequest:
  case ConfigureRequest:
  case MapRequest:
    mask_ = SubstructureRedirectMask;
    break;
  case ResizeRequest:
    mask_ = ResizeRedirectMask;
    break;
  default:
    cerr <<"XWin::Event::Event() : unknown event type : " <<event_ <<endl;
    mask_ = NoEventMask;
    break;
  }
}


