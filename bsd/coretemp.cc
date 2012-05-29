//
//  Copyright (c) 2008 by Tomi Tapper <tomi.o.tapper@jyu.fi>
//
//  Read coretemp reading with sysctl and display actual temperature.
//  If actual >= high, actual temp changes color to indicate alarm.
//
//  File based on linux/lmstemp.* by
//  Copyright (c) 2000, 2006 by Leopold Toetsch <lt@toetsch.at>
//
//  This file may be distributed under terms of the GPL
//
//
//
#include "coretemp.h"
#include "xosview.h"
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/param.h>
#include <stdlib.h>

CoreTemp::CoreTemp( XOSView *parent, const char *label, const char *caption, int cpu)
  : FieldMeter( parent, 2, label, caption, 1, 1, 1 ) {
  _cpu = cpu;
  checkResources();
}

CoreTemp::~CoreTemp( void ) {

}

void CoreTemp::checkResources( void ) {
  FieldMeter::checkResources();

  setfieldcolor( 0, parent_->getResource( "coretempActColor" ) );
  setfieldcolor( 1, parent_->getResource( "coretempIdleColor") );

  priority_ = atoi( parent_->getResource( "coretempPriority" ) );
  _oldtotal = total_ = atoi( parent_->getResourceOrUseDefault( "coretempHighest", "100" ) );
  SetUsedFormat( parent_->getResource( "coretempUsedFormat" ) );
}

void CoreTemp::checkevent( void ) {
  getcoretemp();
  drawfields();
}

// Temperatures can be read with sysctl dev.cpu.%d.temperature.
// Values are in degrees Celsius (FreeBSD < 7.2) or in
// 10*degrees Kelvin (FreeBSD >= 7.3).
void CoreTemp::getcoretemp( void ) {
  int val = 0, high = 0;
  size_t size = sizeof(val);
  char name[25];

  fields_[0] = 0.0;
  if ( _cpu >= 0 ) {    // one core
    snprintf(name, 25, "dev.cpu.%d.temperature", _cpu);
    sysctlbyname(name, &val, &size, NULL, 0);
    snprintf(name, 25, "dev.cpu.%d.coretemp.tjmax", _cpu);
    sysctlbyname(name, &high, &size, NULL, 0);
#if __FreeBSD_version < 702106
    fields_[0] = (float)val;
    total_ = (float)high;
#else
    fields_[0] = ((float)val - 2732.0) / 10.0;
    total_ = ((float)high - 2732.0) / 10.0;
#endif
  }
  else if ( _cpu == -1 ) {  // average
    int cpus = countCpus();
    float tempval = 0.0, temphigh = 0.0;
    for (int i=0; i<cpus; i++) {
      snprintf(name, 25, "dev.cpu.%d.temperature", i);
      sysctlbyname(name, &val, &size, NULL, 0);
      snprintf(name, 25, "dev.cpu.%d.coretemp.tjmax", i);
      sysctlbyname(name, &high, &size, NULL, 0);
      tempval += (float)val;
      temphigh += (float)val;
    }
#if __FreeBSD_version < 702106
    fields_[0] = tempval;
    total_ = temphigh;
#else
    fields_[0] = ((float)val - 2732.0) / 10.0;
    total_ = ((float)high - 2732.0) / 10.0;
#endif
    fields_[0] /= (float)cpus;
    total_ /= (float)cpus;
  }
  else if ( _cpu == -2 ) {  // maximum
    int cpus = countCpus();
    float tempval = -2732.0, temphigh = -2732.0;
    for (int i=0; i<cpus; i++) {
      snprintf(name, 25, "dev.cpu.%d.temperature", i);
      sysctlbyname(name, &val, &size, NULL, 0);
      snprintf(name, 25, "dev.cpu.%d.coretemp.tjmax", i);
      sysctlbyname(name, &high, &size, NULL, 0);
      if (val > tempval)
        tempval = (float)val;
      if (high > temphigh)
        temphigh = (float)high;
    }
#if __FreeBSD_version < 702106
    fields_[0] = tempval;
    total_ = highval;
#else
    fields_[0] = ((float)tempval - 2732.0) / 10.0;
    total_ = ((float)high - 2732.0) / 10.0;
#endif
  }
  else {    // should not happen
    std::cerr << "Unknown CPU core number in coretemp." << std::endl;
    parent_->done(1);
    return;
  }

  fields_[1] = total_ - fields_[0];
  if (fields_[1] <= 0) {   // alarm
    fields_[1] = 0;
    setfieldcolor( 0, parent_->getResource( "coretempHighColor" ) );
  }
  else
    setfieldcolor( 0, parent_->getResource( "coretempActColor" ) );

  setUsed(fields_[0], total_);

  if (total_ != _oldtotal) {
    char l[25];
    snprintf(l, 25, "TEMPERATURE (C)/%d", (int)total_);
    legend(l);
    drawlegend();
    _oldtotal = total_;
  }
}

int CoreTemp::countCpus( void ) {
  int cpus = 0;
  size_t size = sizeof(cpus);
  sysctlbyname("kern.smp.cpus", &cpus, &size, NULL, 0);
  return cpus;
}
