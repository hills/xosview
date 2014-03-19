//
//  Copyright (c) 1997, 2005, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _BTRYMETER_H_
#define _BTRYMETER_H_

#include "fieldmeter.h"
#include "xosview.h"
#include <string>


class BtryMeter : public FieldMeter {
public:
  BtryMeter( XOSView *parent );
  ~BtryMeter( void );

  const char *name( void ) const { return "BtryMeter"; }
  void checkevent( void );

  void checkResources( void );

  // some basic fields of 'info','alarm','state'
  // XXX: should be private
  struct acpi_batt {
          int alarm;              // in mWh
          int design_capacity;    // in mWh
          int last_full_capacity; // in mWh
          int charging_state; // charged=0,discharging=-1,charging=1
          int present_rate; // in mW, 0=unknown
          int remaining_capacity; // in mW
  };
  acpi_batt battery;

  static bool has_source( void );
protected:

  void getpwrinfo( void );
private:

  bool getapminfo( void );
  bool getacpi_or_sys_info( void );

  bool use_apm;
  bool use_acpi;
  bool use_syspower;

  bool acpi_battery_present(const std::string& filename);
  bool acpi_parse_battery(const std::string& filename);
  bool sys_battery_present(const std::string& filename);
  bool sys_parse_battery(const std::string& filename);

  static bool has_acpi(void);
  static bool has_apm(void);
  static bool has_syspower(void);

  void handle_apm_state(void);
  void handle_acpi_state(void);

  int apm_battery_state;
  int old_apm_battery_state;

  int acpi_charge_state;
  int old_acpi_charge_state;

  int acpi_sum_cap;
  int acpi_sum_remain;
  int acpi_sum_rate;
  int acpi_sum_alarm;

};


#endif
