//
//  Copyright (c) 1994, 1995, 2006, 2008 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "Xrm.h"
#include "Xrmcommandline.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>	//  For snprintf().
#include <ctype.h>
#include <unistd.h>  //  for access(), etc.  BCG
#include <iostream>

extern char *defaultXResourceString;


bool Xrm::_initialized = false;

Xrm::Xrm(const char *instanceName, int argc, char **argv){
  std::cerr << " Error:  This constructor is not supported yet." << std::endl;
  exit (-1);
  _db = NULL;
  _class = _instance = NULLQUARK;
  getDisplayName(argc, argv);

  (void) instanceName;
  //  End unsupported constructor.  !!!!!!!! BCG
}

Xrm::Xrm(const char *className, const char *instanceName){
  XrmInitialize ();

  //  Initialize everything to NULL.
  _db = NULL;
  _class = _instance = NULLQUARK;

  // init the _instance and _class Quarks
  _instance = XrmStringToQuark(instanceName);
  initClassName(className);
}

const char*
Xrm::getDisplayName (int argc, char** argv)
{
  (void) argc;  //  Avoid gcc warnings.
  //  See if '-display foo:0' is on the command line, and return it if it is.
  char** argp;

  for (argp = argv; (*argp != NULL) &&
      (strncasecmp (*argp, "-display", 9)); argp++)
    ;  //  Don't do anything.

  //  If we found -display and the next word exists...
  if (*argp && *(++argp))
    _display_name = *argp;
  else
    _display_name = "";
  return _display_name;
  //  An empty display string means use the DISPLAY environment variable.
}

const char *Xrm::getResource(const char *rname) const{
  char frn[1024], fcn[1024];
  snprintf(frn, 1024, "%s.%s", instanceName(), rname);
  snprintf(fcn, 1024, "%s.%s", className(), rname);

  XrmValue val;
  val.addr = NULL;
  char *type;
  XrmGetResource(_db, frn, fcn, &type, &val);
  //  This case here is a hack, because we are currently moving from
  //  always making the instance name be "xosview" to allowing
  //  user-specified ones.  And unfortunately, the class name is
  //  XOsview, and not xosview, so our old defaults (xosview.font)
  //  will not be found when searching for XOsview.font.  bgrayson Dec. 1996
  if (!val.addr)
  {
    //  Let's try with a non-uppercased class name.
    char fcn_lower[1024];
    strncpy(fcn_lower, className(), 1024);
    char *p = fcn_lower;
    while (p && *p)
      {
      *p = tolower(*p);
      p++;
      }
    snprintf(fcn, 1024, "%s.%s", fcn_lower, rname);
    XrmGetResource(_db, frn, fcn, &type, &val);
  }

  return val.addr;
}

Xrm::~Xrm(){
  XrmDestroyDatabase(_db);
}

//---------------------------------------------------------------------
//  This function uses XrmParseCommand, and updates argc and argv through it.
void Xrm::loadAndMergeResources(int& argc, char** argv, Display* display){

  // init the database if it needs it
  if (!_initialized){
    XrmInitialize();
    _initialized = true;
  }
  else
  {
    std::cerr << "Error:  Xrm:loadAndMergeResources() called twice!" << std::endl;
    exit (-1);
  }
  //  This is ugly code.  According to X and Xt rules, many files need
  //  to be checked for resource settings.  Since we aren't (yet) using
  //  Xt or any other package, we need to do all of these checks
  //  individually.  BCG
//  =========== BEGIN X Resource lookup and merging ==========

//  This all needs to be done in the proper order:
/*
Listed from weakest to strongest:
  (from code-builtin-resources) (Stored in the string in defaultstring.h)
  app-defaults
  XOSView (from XAPPLRESDIR directory)
  from RESOURCE_MANAGER property on server (reads .Xdefaults if needed)
  from file specified in XENVIRONMENT
  from command line (i.e., handled with XrmParseCommand)
*/

  // Get resources from the various resource files

  //  Put the default, compile-time options as the lowest priority.
  _db = XrmGetStringDatabase (defaultXResourceString);

  //  Merge in the system resource database.
  char rfilename[2048];
  int result;
  const int rlen = sizeof rfilename;

  // Get the app-defaults
  result = snprintf(rfilename, sizeof rfilename, "/etc/X11/app-defaults/%s",
    XrmQuarkToString(_class));
  if (result >= 0 && result < rlen)
    XrmCombineFileDatabase (rfilename, &_db, 1);
  result = snprintf(rfilename, sizeof rfilename, "/usr/lib/X11/app-defaults/%s",
    XrmQuarkToString(_class));
  if (result >= 0 && result < rlen)
    XrmCombineFileDatabase (rfilename, &_db, 1);
  result = snprintf(rfilename, sizeof rfilename, "/usr/X11R6/lib/X11/app-defaults/%s",
    XrmQuarkToString(_class));
  if (result >= 0 && result < rlen)
    XrmCombineFileDatabase (rfilename, &_db, 1);
  result = snprintf(rfilename, sizeof rfilename, "/usr/share/X11/app-defaults/%s",
    XrmQuarkToString(_class));
  if (result >= 0 && result < rlen)
    XrmCombineFileDatabase (rfilename, &_db, 1);
  //  Try a few more, for SunOS/Solaris folks.
  result = snprintf(rfilename, sizeof rfilename, "/usr/openwin/lib/X11/app-defaults/%s",
    XrmQuarkToString(_class));
  if (result >= 0 && result < rlen)
    XrmCombineFileDatabase (rfilename, &_db, 1);
  result = snprintf(rfilename, sizeof rfilename, "/usr/local/X11R6/lib/X11/app-defaults/%s",
      XrmQuarkToString(_class));
  if (result >= 0 && result < rlen)
    XrmCombineFileDatabase (rfilename, &_db, 1);

  //  Now, check for an XOSView file in the XAPPLRESDIR directory...
  char* xappdir = getenv ("XAPPLRESDIR");
  if (xappdir != NULL)
  {
    char xappfile[1024];
    snprintf (xappfile, 1024, "%s/%s", xappdir, className());
    // this did not work for XAPPLRESDIR
    //if (!access (xappfile, X_OK | R_OK))
    if (!access (xappfile, R_OK))
    {
      XrmCombineFileDatabase (xappfile, &_db, 1);
      }
  }

  //  Now, check the display's RESOURCE_MANAGER property...
  char* displayString = XResourceManagerString (display);
  if (displayString != NULL)
  {
    XrmDatabase displayrdb = XrmGetStringDatabase (displayString);
    XrmMergeDatabases (displayrdb, &_db);  //  Destroys displayrdb when done.
  }

  //  And check this screen of the display...
  char* screenString =
              XScreenResourceString (DefaultScreenOfDisplay(display));
  if (screenString != NULL)
  {
    XrmDatabase screenrdb = XrmGetStringDatabase (screenString);
    XrmMergeDatabases (screenrdb, &_db);  //  Destroys screenrdb when done.
  }

  //  Now, check for a user resource file, and merge it in if there is one...
  if ( getenv( "HOME" ) != NULL ){
    char userrfilename[1024];
    char *home = getenv("HOME");
    snprintf(userrfilename, 1024, "%s/.Xdefaults", home);
    //  User file overrides system (_db).
    XrmCombineFileDatabase (userrfilename, &_db, 1);
  }

  //  Second-to-last, parse any resource file specified in the
  //  environment variable XENVIRONMENT.
  char* xenvfile;
  if ((xenvfile = getenv ("XENVIRONMENT")) != NULL)
  {
    //  The XENVIRONMENT file overrides all of the above.
    XrmCombineFileDatabase (xenvfile, &_db, 1);
  }
  //  Command-line resources override system and user defaults.
  XrmDatabase cmdlineRdb_ = NULL;
  XrmParseCommand (&cmdlineRdb_, options, NUM_OPTIONS, instanceName(),
		    &argc, argv);
  XrmCombineDatabase (cmdlineRdb_, &_db, 1);  //  Keeps cmdlineRdb_ around.
//  =========== END X Resource lookup and merging ==========
}

void Xrm::initClassName(const char* name){
  char className[256];
  strncpy(className, name, 255);  //  Avoid evil people out there...

  className[0] = toupper(className[0]);
  if (className[0] == 'X')
      className[1] = toupper(className[1]);

  _class = XrmStringToQuark(className);
}



//------------  Some debugging functions follow.  -----------------------
inline std::ostream &operator<<(std::ostream &os, const XrmBinding &b){
  switch (b){
  case XrmBindTightly:
    return os << ".";
  case XrmBindLoosely:
    return os << "*";
  default:
    std::cerr <<"std::ostream operator<<(std::ostream &, const XrmBinding &) : "
      <<"Unknown XrmBinding!";
    return os;
  }

  return os;
}

std::ostream &Xrm::dump(std::ostream &os) const {
  os <<"--- Xrm --- class: " <<XrmQuarkToString(_class)
     <<", instance: " <<XrmQuarkToString(_instance) <<"\n";

  XrmName names[] = { _instance, NULLQUARK };
  XrmClass classes[] = { _class, NULLQUARK };

  XrmEnumerateDatabase(_db, names, classes, XrmEnumAllLevels, enumCB,
                       (XPointer)&os);

  return os;
}

Bool Xrm::enumCB(XrmDatabase *, XrmBindingList bindings,
                 XrmQuarkList quarks, XrmRepresentation *type,
                 XrmValue *value, XPointer closure) {

  std::ostream *os = (std::ostream *)closure;
  (void) type;  //  Avoid gcc warnings.

  //std::cerr <<"type = " <<XrmQuarkToString(*type) <<std::endl;

  int i = 0;
  while (quarks[i] != NULLQUARK){
    *os <<bindings[i] <<XrmQuarkToString(quarks[i]);
    i++;
  }
  *os <<": " <<value->addr <<"\n";

  return False;
}
