//  
//  Copyright (c) 1994, 1995 by Mike Romberg ( romberg@fsl.noaa.gov )
//
//  NetBSD port:  
//  Copyright (c) 1995 Brian Grayson(bgrayson@pine.ece.utexas.edu)
//
//  This file may be distributed under terms of the GPL
//

#include "cpumeter.h"
#include "xosview.h"
#include <sys/dkstat.h>         //  For CPUSTATES #define.  BCG
#include <err.h>                //  For err() and warn(), etc.  BCG
#include "netbsd.h"             //  For NetBSD-specific icky kvm_ code.  BCG
#include <stdlib.h>		//  For use of atoi  BCG


CPUMeter::CPUMeter( XOSView *parent )
: FieldMeterDecay( parent, 4, "CPU", "USR/NICE/SYS/FREE" ){
  for ( int i = 0 ; i < 2 ; i++ )
    for ( int j = 0 ; j < 4 ; j++ )
      cputime_[i][j] = 0;
  cpuindex_ = 0;
  //  The setting of the priority will be done in checkResources().  BCG
  dodecay_ = 0;

  NetBSDCPUInit();
}

CPUMeter::~CPUMeter( void ){
}

void CPUMeter::checkResources( void ){
  FieldMeter::checkResources();

  setfieldcolor( 0, parent_->getResource("cpuUserColor") );
  setfieldcolor( 1, parent_->getResource("cpuNiceColor") );
  setfieldcolor( 2, parent_->getResource("cpuSystemColor") );
  setfieldcolor( 3, parent_->getResource("cpuFreeColor") );
  priority_ = atoi (parent_->getResource("cpuPriority"));
  dodecay_ = !strcmp (parent_->getResource("cpuDecay"),"True");
}

void CPUMeter::checkevent( void ){
  getcputime();
  drawfields();
}

void CPUMeter::getcputime( void ){
  total_ = 0;


  //  Begin NetBSD-specific code...  BCG
  long tempCPU[CPUSTATES];

  NetBSDGetCPUTimes (tempCPU);

  cputime_[cpuindex_][0] = tempCPU[0];
  cputime_[cpuindex_][1] = tempCPU[1];
//  Merge System and Interrupt here...  NetBSD does not yet (Nov 95) support
//  the interrupt ticks, even though most code has been written to allow
//  it -- interrupt ticks are credited to system by the kernel.  BCG
  cputime_[cpuindex_][2] = tempCPU[2] + tempCPU[3];
  cputime_[cpuindex_][3] = tempCPU[4];
  //  End NetBSD-specific code...  BCG
  

  int oldindex = (cpuindex_+1)%2;
  for ( int i = 0 ; i < 4 ; i++ ){
    fields_[i] = cputime_[cpuindex_][i] - cputime_[oldindex][i];
    total_ += fields_[i];
  }
  used( (int)((100 * (total_ - fields_[3])) / total_) );

  cpuindex_ = (cpuindex_ + 1) % 2;
}
