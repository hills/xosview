//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _Xrm_h
#define _Xrm_h

#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <iostream>


class Xrm {
public:
  Xrm(const char *className, const char *instanceName);
  Xrm(const char *instanceName, int argc, char **argv);
  ~Xrm();

  const char *className(void) const { return XrmQuarkToString(_class); }
  const char *instanceName(void) const { return XrmQuarkToString(_instance); }

  const char *getResource(const char *rname) const;
  const char* getDisplayName (int argc, char** argv);
  void loadAndMergeResources(int& argc, char** argv, Display* display);

  std::ostream &dump(std::ostream &os) const;

private:
  XrmDatabase _db;
  XrmClass _class, _instance;
  const char* _display_name;  //  Used solely for getting the display's resources.

  //void getArgs(int argc, char **argv);
  static Bool enumCB(XrmDatabase *, XrmBindingList bindings,
                     XrmQuarkList quarks, XrmRepresentation *type,
                     XrmValue *value, XPointer closure);
  void initClassName(const char* className);

  static bool _initialized;
};

inline std::ostream &operator<<(std::ostream &os, const Xrm &xrm){
  return xrm.dump(os);
}

#endif
