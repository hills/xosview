//
//  Copyright (c) 2007 by Samuel Thibault ( samuel.thibault@ens-lyon.org )
//
//  This file may be distributed under terms of the GPL
//

#define _GNU_SOURCE
#include <fcntl.h>
#include <error.h>

#include <mach.h>
#include <mach/mach_traps.h>
#include <mach/default_pager.h>
#include <hurd.h>
#include <hurd/paths.h>

mach_port_t get_def_pager(void) {
  mach_port_t def_pager = MACH_PORT_NULL;
  kern_return_t err;
  mach_port_t host;

  err = get_privileged_ports (&host, 0);
  if (err == EPERM) {
    def_pager = file_name_lookup (_SERVERS_DEFPAGER, O_READ, 0);
    if (def_pager == MACH_PORT_NULL)
      error (0, errno, _SERVERS_DEFPAGER);
    return def_pager;
  } else if (err) {
    error (0, err, "get_privileged_ports");
    return MACH_PORT_NULL;
  } else {
    err = vm_set_default_memory_manager (host, &def_pager);
    mach_port_deallocate (mach_task_self(), host);
    if (err) {
      error (0, err, "vm_set_default_memory_manager");
      return MACH_PORT_NULL;
    }
    return def_pager;
  }
}
