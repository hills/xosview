//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  This file may be distributed under terms of the GPL
//
#include "Xrm.h"
//#include "defaultstring.h"
#include "Xrmcommandline.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>	//  For sprintf().
#include <ctype.h>
#include <iostream.h>
#include <unistd.h>  //  for access(), etc.  BCG


extern char *defaultXResourceString;


bool Xrm::_initialized = false;

Xrm::Xrm(const char *instanceName, int argc, char **argv){
  cerr << " Error:  This constructor is not supported yet.\n";
  exit (-1);
  _db = NULL;
  _class = _instance = NULLQUARK;
  getDisplayName(argc, argv);

  (void) instanceName;
  //  End unsupported constructor.  !!!!!!!! BCG
}

Xrm::Xrm(const char *instanceName){
  XrmInitialize ();

  //  Initialize everything to NULL.
  _db = NULL;
  _class = _instance = NULLQUARK;

  // init the _instance and _class Quarks
  _instance = XrmStringToQuark(instanceName);
  initClassName();

}

const char*
Xrm::getDisplayName (int argc, char** argv)
{
  (void) argc;  //  Avoid gcc warnings.
  //  See if '-display foo:0' is on the command line, and return it if it is.
  char** argp;

  for (argp = argv; (*argp != NULL) && (strcmp (*argp, "-display")); argp++)
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
  strcpy(frn, instanceName());
  strcat(frn, ".");
  strcat(frn, rname);
  
  strcpy(fcn, className());
  strcat(fcn, ".");
  strcat(fcn, rname);

  XrmValue val;
  val.addr = NULL;
  char *type;
  XrmGetResource(_db, frn, fcn, &type, &val);

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
    cerr << "Error:  Xrm:loadAndMergeResources() called twice!\n";
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

  // Get the app-defaults
  strcpy(rfilename, "/usr/lib/X11/app-defaults/");
  strcat(rfilename, XrmQuarkToString(_class));
  if (rfilename != NULL)
    XrmCombineFileDatabase (rfilename, &_db, 1);

  //  Now, check for an XOSView file in the XAPPLRESDIR directory...
  char* xappdir = getenv ("XAPPLRESDIR");
  if (xappdir != NULL)
  {
    char xappfile[1024];
    sprintf (xappfile, "%s/%s", xappdir, className());
    if (!access (xappfile, X_OK | R_OK))
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
    strcpy(userrfilename, getenv("HOME"));
    strcat(userrfilename, "/.Xdefaults");
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

void Xrm::initClassName(void){
  char className[256];
  strcpy(className, instanceName());

  className[0] = toupper(className[0]);
  if (className[0] == 'X')
      className[1] = toupper(className[1]);

  _class = XrmStringToQuark(className);  
}



//------------  Some debugging functions follow.  -----------------------
inline ostream &operator<<(ostream &os, const XrmBinding &b){
  switch (b){
  case XrmBindTightly:
    return os << ".";
  case XrmBindLoosely:
    return os << "*";
  default:
    cerr <<"ostream operator<<(ostream &, const XrmBinding &) : "
      <<"Unknown XrmBinding!";
    return os;
  }

  return os;
}

ostream &Xrm::dump(ostream &os) const {
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
  
  ostream *os = (ostream *)closure;
  (void) type;  //  Avoid gcc warnings.

  //cerr <<"type = " <<XrmQuarkToString(*type) <<endl;

  int i = 0;
  while (quarks[i] != NULLQUARK){
    *os <<bindings[i] <<XrmQuarkToString(quarks[i]);
    i++;
  }
  *os <<": " <<value->addr <<"\n";

  return False;
}
