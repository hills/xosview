#include "xwin.h"
#include "Xrm.h"
#include <X11/Xatom.h>
#include <X11/xpm.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <vector>
#include <iostream>
#ifdef HAVE_XFT
#include <fontconfig/fontconfig.h>
#endif

namespace {

std::string trimFontName( const std::string &name ) {
  size_t first = name.find_first_not_of( " \t\r\n" );
  if ( first == std::string::npos )
    return "";
  size_t last = name.find_last_not_of( " \t\r\n" );
  return name.substr( first, last - first + 1 );
}

std::vector<std::string> splitXlfd( const std::string &fontName ) {
  std::string name = trimFontName( fontName );
  if ( name.size() >= 2 && name[0] == '"' && name[name.size()-1] == '"' )
    name = name.substr( 1, name.size() - 2 );
  if ( name.empty() || name[0] != '-' )
    return std::vector<std::string>();

  std::vector<std::string> parts;
  size_t start = 0;
  while ( start <= name.size() ) {
    size_t dash = name.find( '-', start );
    if ( dash == std::string::npos ) {
      parts.push_back( name.substr( start ) );
      break;
    }
    parts.push_back( name.substr( start, dash - start ) );
    start = dash + 1;
  }

  if ( parts.size() != 15 || !parts[0].empty() )
    return std::vector<std::string>();
  return std::vector<std::string>( parts.begin() + 1, parts.end() );
}

std::string joinXlfd( const std::vector<std::string> &fields ) {
  std::string name = "-";
  for ( size_t i = 0; i < fields.size(); i++ ) {
    if ( i )
      name += '-';
    name += fields[i];
  }
  return name;
}

bool isScalableXlfd( const std::vector<std::string> &fields ) {
  return fields.size() == 14 && fields[6] == "0";
}

std::string scaledXlfdName( const std::vector<std::string> &fields,
                            int pixelHeight ) {
  std::vector<std::string> scaled = fields;
  char size[32];
  snprintf( size, sizeof(size), "%d", pixelHeight );
  scaled[6] = size;
  scaled[7] = "0";
  scaled[8] = "0";
  scaled[9] = "0";
  scaled[11] = "0";
  return joinXlfd( scaled );
}

int xlfdPixelSize( const std::string &fontName ) {
  std::vector<std::string> fields = splitXlfd( fontName );
  if ( fields.size() != 14 || fields[6].empty() ||
       !isdigit( static_cast<unsigned char>( fields[6][0] ) ) )
    return 0;
  return atoi( fields[6].c_str() );
}

std::string closestBitmapFont( Display *display,
                               const std::vector<std::string> &fields,
                               int pixelHeight ) {
  std::vector<std::string> patternFields( 14, "*" );
  for ( int i = 0; i < 6; i++ )
    patternFields[i] = fields[i];
  patternFields[10] = fields[10];
  patternFields[12] = fields[12];
  patternFields[13] = fields[13];

  int count = 0;
  char **names = XListFonts( display, joinXlfd( patternFields ).c_str(),
                             4096, &count );
  if ( !names )
    return "";

  std::vector<std::string> bestFields;
  int bestDiff = 0x7fffffff;
  int bestPixel = 0;
  for ( int i = 0; i < count; i++ ) {
    std::vector<std::string> candidate = splitXlfd( names[i] );
    int pixel = xlfdPixelSize( names[i] );
    if ( candidate.size() != 14 || pixel <= 0 )
      continue;
    int diff = pixel > pixelHeight ? pixel - pixelHeight
                                   : pixelHeight - pixel;
    if ( diff < bestDiff ||
         ( diff == bestDiff &&
           ( pixelHeight >= bestPixel ? pixel > bestPixel
                                      : pixel < bestPixel ) ) ) {
      bestDiff = diff;
      bestPixel = pixel;
      bestFields = candidate;
    }
  }
  XFreeFontNames( names );
  return bestFields.empty() ? "" : joinXlfd( bestFields );
}

std::string lowerFontString( const std::string &text ) {
  std::string result = text;
  for ( size_t i = 0; i < result.size(); i++ )
    result[i] = static_cast<char>( tolower( static_cast<unsigned char>( result[i] ) ) );
  return result;
}

bool containsFontText( const std::string &text, const char *needle ) {
  return lowerFontString( text ).find( needle ) != std::string::npos;
}

bool fontWeightIsBold( const std::string &weight ) {
  std::string w = lowerFontString( weight );
  return containsFontText( w, "bold" ) || containsFontText( w, "demi" ) ||
         containsFontText( w, "black" ) || containsFontText( w, "heavy" );
}

bool fontSpacingIsMono( const std::vector<std::string> &fields ) {
  std::string spacing = lowerFontString( fields[10] );
  std::string family = lowerFontString( fields[1] );
  return spacing == "m" || spacing == "c" ||
         containsFontText( family, "mono" ) ||
         containsFontText( family, "courier" ) ||
         containsFontText( family, "fixed" ) ||
         containsFontText( family, "typewriter" );
}

bool fontSlantIsItalic( const std::string &slant ) {
  std::string s = lowerFontString( slant );
  return s == "i" || s == "o" || containsFontText( s, "italic" ) ||
         containsFontText( s, "oblique" );
}

bool fontSpecWantsBold( const std::string &fontName ) {
  std::vector<std::string> fields = splitXlfd( fontName );
  if ( fields.size() == 14 )
    return fontWeightIsBold( fields[2] );
  std::string name = lowerFontString( fontName );
  return containsFontText( name, "bold" ) || containsFontText( name, "demi" ) ||
         containsFontText( name, "black" ) || containsFontText( name, "heavy" );
}

bool fontSpecWantsItalic( const std::string &fontName ) {
  std::vector<std::string> fields = splitXlfd( fontName );
  if ( fields.size() == 14 )
    return fontSlantIsItalic( fields[3] );
  std::string name = lowerFontString( fontName );
  return containsFontText( name, "italic" ) || containsFontText( name, "oblique" );
}

int scalableFontScore( const std::vector<std::string> &fields,
                       bool wantBold, bool wantItalic ) {
  int score = 0;
  bool bold = fontWeightIsBold( fields[2] );
  bool italic = fontSlantIsItalic( fields[3] );

  if ( bold == wantBold )
    score += 4;
  if ( italic == wantItalic )
    score += 2;
  if ( fontSpacingIsMono( fields ) )
    score += 8;

  std::string registry = lowerFontString( fields[12] );
  if ( registry.find( "iso8859" ) != std::string::npos ||
       registry.find( "iso10646" ) != std::string::npos )
    score += 2;
  if ( registry == "adobe-fontspecific" )
    score -= 4;

  std::string family = lowerFontString( fields[1] );
  if ( containsFontText( family, "dingbat" ) ||
       containsFontText( family, "symbol" ) )
    score -= 10;
  return score;
}

} // namespace

//-----------------------------------------------------------------------------

//  argc is a reference, so that the changes to argc by XrmParseCommand are
//  noticed by the caller (XOSView, in this case).  BCG
XWin::XWin() {
}
//-----------------------------------------------------------------------------

XWin::XWin( int argc, char *argv[], int x, int y, int width, int height ) {

  std::cerr << "This constructor call is not supported! (" << __FILE__
       << ":" << __LINE__ << ")" << std::endl;
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
  fontScaling_ = !strncasecmp( getResourceOrUseDefault( "fontScaling", "True" ),
                               "True", 4 );

  window_ = None;
  gc_ = None;
  name_ = (char *)malloc(0);
  font_ = NULL;
#ifdef HAVE_XFT
  xftFont_ = NULL;
  xftDraw_ = NULL;
  useXft_ = false;
#endif
  baseFontHeight_ = fontPixelHeight_ = lastFontSizeRequest_ = 0;
  fontScale_ = 1.0;
  fallbackFontSearched_ = false;
  fontName_.clear();
  fallbackFontName_.clear();
  done_ = 0;

  // Set up the default Events
  events_ = NULL;
  addEvent( new Event( this, ClientMessage, &XWin::deleteEvent ) );
  addEvent( new Event( this, MappingNotify, &XWin::mappingNotify ) );

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
  if ( gc_ != None )
    XFreeGC( display_, gc_ );
  if ( font_ )
    XFreeFont( display_, font_ );
#ifdef HAVE_XFT
  if ( xftDraw_ )
    XftDrawDestroy( xftDraw_ );
  if ( xftFont_ )
    XftFontClose( display_, xftFont_ );
#endif
  if ( window_ != None )
    XDestroyWindow( display_, window_ );
  // close the connection to the display
  XCloseDisplay( display_ );
}
//-----------------------------------------------------------------------------

void XWin::init( int argc, char **argv ){
  XGCValues            gcv;
  XSetWindowAttributes xswa;
  Pixmap background_pixmap;

  setFont();
  setColors();
  getGeometry();

  borderwidth_ = atoi(getResourceOrUseDefault("borderwidth", "1"));

  window_ = XCreateSimpleWindow(display_, DefaultRootWindow(display_),
				sizehints_->x, sizehints_->y,
				sizehints_->width, sizehints_->height,
				borderwidth_,
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

  // If there is a pixmap file, set it as the background
  if (getPixmap(&background_pixmap))
    XSetWindowBackgroundPixmap(display_,window_,background_pixmap);

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
    std::cerr <<"Error getting attributes of Main." <<std::endl;
    exit(2);
  }
#ifdef HAVE_XFT
  initXftDraw();
#endif

  //  Create stipple pixmaps.
  stipples_[0] = createPixmap("\000\000", 2, 2);
  stipples_[1] = createPixmap("\002\000\001\000", 2, 4);
  stipples_[2] = createPixmap("\002\001", 2, 2);
  stipples_[3] = createPixmap("\002\003\001\003", 2, 4);
  doStippling_ = isResourceTrue("enableStipple");
}
//-----------------------------------------------------------------------------

void XWin::setFont( void ){
  // Set up the font
  if ( font_ != NULL )
    return;
  fontName_ = getResource("font");

  if ((font_ = XLoadQueryFont(display_, fontName_.c_str())) == NULL){
    std::cerr <<name_ <<": font \"" << fontName_
	      << "\" does not exist; see xlsfonts(1)"
	      << std::endl;
    exit(1);
  }
  baseFontHeight_ = fontPixelHeight_ = lastFontSizeRequest_ = textHeight();
  fontScale_ = 1.0;
#ifdef HAVE_XFT
  xftFont_ = openXftFont( baseFontHeight_ );
  useXft_ = xftFont_ != NULL;
#endif
}
//-----------------------------------------------------------------------------

bool XWin::fontHeightClose( XFontStruct *font, int pixelHeight ) const {
  int height = font->ascent + font->descent;
  int diff = height > pixelHeight ? height - pixelHeight
                                  : pixelHeight - height;
  int tolerance = pixelHeight / 10 + 2;
  return diff <= tolerance;
}

const std::string &XWin::fallbackScalableFont( void ) {
  if ( fallbackFontSearched_ )
    return fallbackFontName_;
  fallbackFontSearched_ = true;

  std::vector<std::string> patternFields( 14, "*" );
  patternFields[6] = "0";

  int count = 0;
  char **names = XListFonts( display_, joinXlfd( patternFields ).c_str(),
                             4096, &count );
  if ( !names )
    return fallbackFontName_;

  bool wantBold = fontSpecWantsBold( fontName_ );
  bool wantItalic = fontSpecWantsItalic( fontName_ );
  int bestMonoScore = -0x7fffffff;
  int bestAnyScore = -0x7fffffff;
  std::string bestMonoName, bestAnyName;
  for ( int i = 0; i < count; i++ ) {
    std::vector<std::string> fields = splitXlfd( names[i] );
    if ( !isScalableXlfd( fields ) )
      continue;
    int score = scalableFontScore( fields, wantBold, wantItalic );
    if ( score > bestAnyScore ) {
      bestAnyScore = score;
      bestAnyName = joinXlfd( fields );
    }
    if ( fontSpacingIsMono( fields ) && score > bestMonoScore ) {
      bestMonoScore = score;
      bestMonoName = joinXlfd( fields );
    }
  }
  XFreeFontNames( names );

  fallbackFontName_ = bestMonoName.empty() ? bestAnyName : bestMonoName;
  return fallbackFontName_;
}

std::string XWin::fallbackFontNameForPixelHeight( int pixelHeight ) {
  const std::string &base = fallbackScalableFont();
  if ( base.empty() )
    return "";
  std::vector<std::string> fields = splitXlfd( base );
  if ( fields.size() != 14 )
    return "";
  return scaledXlfdName( fields, pixelHeight );
}

std::string XWin::fontNameForPixelHeight( int pixelHeight ) {
  std::vector<std::string> fields = splitXlfd( fontName_ );
  if ( fields.size() == 14 ) {
    if ( isScalableXlfd( fields ) )
      return scaledXlfdName( fields, pixelHeight );
    std::string closest = closestBitmapFont( display_, fields, pixelHeight );
    if ( !closest.empty() )
      return closest;
  }
  if ( pixelHeight == baseFontHeight_ )
    return fontName_;
  return "";
}

bool XWin::setFontScale( double scale ) {
  fontScale_ = scale;
  if ( !fontScaling_ || !font_ || baseFontHeight_ <= 0 )
    return false;

  int pixelHeight = static_cast<int>( baseFontHeight_ * scale + 0.5 );
  if ( pixelHeight < 6 )
    pixelHeight = 6;
  else if ( pixelHeight > 200 )
    pixelHeight = 200;
  if ( pixelHeight == lastFontSizeRequest_ )
    return false;
  lastFontSizeRequest_ = pixelHeight;

#ifdef HAVE_XFT
  if ( useXft_ ) {
    XftFont *candidate = openXftFont( pixelHeight );
    if ( !candidate )
      return false;
    if ( xftFont_ )
      XftFontClose( display_, xftFont_ );
    xftFont_ = candidate;
    fontPixelHeight_ = pixelHeight;
    return true;
  }
#endif

  std::vector<std::string> candidates;
  std::string primary = fontNameForPixelHeight( pixelHeight );
  if ( !primary.empty() )
    candidates.push_back( primary );
  std::string fallback = fallbackFontNameForPixelHeight( pixelHeight );
  if ( !fallback.empty() && fallback != primary )
    candidates.push_back( fallback );

  for ( size_t i = 0; i < candidates.size(); i++ ) {
    XFontStruct *candidate = XLoadQueryFont( display_, candidates[i].c_str() );
    if ( !candidate )
      continue;
    if ( fontHeightClose( candidate, pixelHeight ) ) {
      if ( font_ )
        XFreeFont( display_, font_ );
      font_ = candidate;
      fontPixelHeight_ = pixelHeight;
      if ( gc_ != None ) {
        XGCValues gcv;
        gcv.font = font_->fid;
        XChangeGC( display_, gc_, GCFont, &gcv );
      }
      return true;
    }
    XFreeFont( display_, candidate );
  }
  return false;
}
//-----------------------------------------------------------------------------

void XWin::updateTextColor( unsigned long pixelvalue ) {
#ifdef HAVE_XFT
  updateXftColor( pixelvalue );
#endif
}

#ifdef HAVE_XFT
XftFont *XWin::openXftFont( int pixelHeight ) {
  FcInit();
  FcPattern *pattern = FcPatternCreate();
  if ( !pattern )
    return NULL;

  FcPatternAddString( pattern, FC_FAMILY, (const FcChar8 *)"monospace" );
  FcPatternAddInteger( pattern, FC_WEIGHT,
                       fontSpecWantsBold( fontName_ ) ? FC_WEIGHT_BOLD
                                                      : FC_WEIGHT_NORMAL );
  FcPatternAddInteger( pattern, FC_SLANT,
                       fontSpecWantsItalic( fontName_ ) ? FC_SLANT_ITALIC
                                                        : FC_SLANT_ROMAN );
  FcPatternAddInteger( pattern, FC_SPACING, FC_MONO );
  FcPatternAddDouble( pattern, FC_PIXEL_SIZE, pixelHeight );
  FcPatternAddBool( pattern, FC_ANTIALIAS, FcTrue );
  FcPatternAddBool( pattern, FC_HINTING, FcTrue );
  FcPatternAddInteger( pattern, FC_RGBA, FC_RGBA_RGB );

  FcConfig *config = FcConfigGetCurrent();
  if ( config )
    FcConfigSubstitute( config, pattern, FcMatchPattern );
  FcDefaultSubstitute( pattern );

  FcPattern *matched = NULL;
  if ( config ) {
    FcResult result;
    matched = FcFontMatch( config, pattern, &result );
  }
  XftFont *font = XftFontOpenPattern( display_, matched ? matched : pattern );
  if ( matched )
    FcPatternDestroy( matched );
  FcPatternDestroy( pattern );
  return font;
}

void XWin::initXftDraw( void ) {
  if ( !useXft_ )
    return;
  Visual *visual = attr_.visual ? attr_.visual
                                : DefaultVisual( display_, screen() );
  xftDraw_ = XftDrawCreate( display_, window_, visual, colormap_ );
  if ( !xftDraw_ ) {
    useXft_ = false;
    return;
  }
  updateXftColor( fgcolor_ );
}

bool XWin::validXftPixel( unsigned long pixelvalue ) const {
  if ( pixelvalue > 0xffffffffUL )
    return false;
  Visual *visual = attr_.visual ? attr_.visual
                                : DefaultVisual( display_, DefaultScreen( display_ ) );
  if ( !visual )
    return false;
  if ( visual->c_class == TrueColor || visual->c_class == DirectColor ) {
    unsigned long mask = visual->red_mask | visual->green_mask |
                         visual->blue_mask;
    return mask == 0 || ( pixelvalue & ~mask ) == 0;
  }
  unsigned long entries = visual->map_entries > 0
                          ? static_cast<unsigned long>( visual->map_entries )
                          : 256;
  return pixelvalue < entries;
}

void XWin::updateXftColor( unsigned long pixelvalue ) {
  if ( !useXft_ || !xftDraw_ || !validXftPixel( pixelvalue ) )
    return;
  XColor color;
  color.pixel = pixelvalue;
  if ( !XQueryColor( display_, colormap_, &color ) )
    return;
  xftColor_.pixel = pixelvalue;
  xftColor_.color.red = color.red;
  xftColor_.color.green = color.green;
  xftColor_.color.blue = color.blue;
  xftColor_.color.alpha = 0xffff;
}
#endif
//-----------------------------------------------------------------------------

void XWin::setHints( int argc, char *argv[] ){
  // Set up class hint
  if((classhints_ = XAllocClassHint()) == NULL){
    std::cerr <<"Error allocating class hint!" <<std::endl;
    exit(1);
  }
  //  We have to cast away the const's.
  classhints_->res_name = (char*) xrmptr_->instanceName();
  classhints_->res_class = (char*) xrmptr_->className();

  // Set up the window manager hints
  if((wmhints_ = XAllocWMHints()) == NULL){
    std::cerr <<"Error allocating Window Manager hints!" <<std::endl;
    exit(1);
  }
  wmhints_->flags = (InputHint|StateHint);
  wmhints_->input = True;
  wmhints_->initial_state = NormalState;

  // Set up XTextProperty for window name and icon name
  if(XStringListToTextProperty(&name_, 1, &title_) == 0){
    std::cerr <<"Error creating XTextProperty!" <<std::endl;
    exit(1);
  }
  if(XStringListToTextProperty(&name_, 1, &iconname_) == 0){
    std::cerr <<"Error creating XTextProperty!" <<std::endl;
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
    std::cerr <<"Can't open display named " << display_name_ <<std::endl;
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

int XWin::getPixmap(Pixmap *pixmap)
{
  char	*pixmap_file;
  XWindowAttributes	root_att;
  XpmAttributes		pixmap_att;

  pixmap_file = (char*) getResourceOrUseDefault("pixmapName",NULL);

  if (!pixmap_file)
    return 0;

  XGetWindowAttributes(display_, DefaultRootWindow(display_),&root_att);
  pixmap_att.closeness=30000;
  pixmap_att.colormap=root_att.colormap;
  pixmap_att.valuemask=XpmSize|XpmReturnPixels|XpmColormap|XpmCloseness;

  if (XpmReadFileToPixmap(display_,DefaultRootWindow(display_),pixmap_file,
      pixmap, NULL, &pixmap_att))
  {
    std::cerr << "Pixmap " << pixmap_file  << " not found" << std::endl;
    std::cerr << "Defaulting to blank" << std::endl;
    pixmap = NULL;
    return 0;
  }

  return 1;
}

//-----------------------------------------------------------------------------
void XWin::getGeometry( void ){
  char                 default_geometry[80];
  int                  bitmask;

  // Fill out a XsizeHints structure to inform the window manager
  // of desired size and location of main window.
  if((sizehints_ = XAllocSizeHints()) == NULL){
    std::cerr <<"Error allocating size hints!" <<std::endl;
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
  snprintf(default_geometry, 80, "%dx%d+%d+%d", sizehints_->width,
	  sizehints_->height, sizehints_->x, sizehints_->y);

  // Process the geometry specification
  bitmask =  XGeometry(display_, DefaultScreen(display_),
		       getResourceOrUseDefault("geometry", geometry_),
                       default_geometry,
		       0,
		       1, 1, 0, 0, &(sizehints_->x), &(sizehints_->y),
		       &(sizehints_->width), &(sizehints_->height));

  // Check bitmask and set flags in XSizeHints structure
  if (bitmask & (WidthValue | HeightValue)){
    sizehints_->flags |= PPosition;
    width_ = sizehints_->width;
    height_ = sizehints_->height;
  }

  if (bitmask & (XValue | YValue)){
    sizehints_->flags |= USPosition;
    x_ = sizehints_->x;
    y_ = sizehints_->y;
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
    std::cerr << "Error:  Couldn't find '" << name << "' resource in the resource database!\n";
    exit (-1);
      /*  Some compilers aren't smart enough to know that exit() exits.  */
    return NULL;
  }
}

//-----------------------------------------------------------------------------

void XWin::dumpResources( std::ostream &os ){
std::cerr << "Function not implemented!\n";  //  BCG  FIXME  Need to make this.
(void) os;  //  Keep gcc happy.
}
//-----------------------------------------------------------------------------

unsigned long XWin::allocColor( const char *name ){
  XColor exact, closest;

  if ( XAllocNamedColor( display_, colormap(), name, &closest, &exact ) == 0 )
    std::cerr <<"XWin::allocColor() : failed to alloc : " <<name <<std::endl;

  return exact.pixel;
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
    std::cerr <<"XWin::Event::Event() : unknown event type : " <<event_ <<std::endl;
    mask_ = NoEventMask;
    break;
  }
}
