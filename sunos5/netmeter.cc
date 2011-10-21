//  
//  Rewritten for Solaris by Arno Augustin 1999
//  augustin@informatik.uni-erlangen.de
//


#include "netmeter.h"
#include "xosview.h"
#include <stdlib.h>

NetMeter::NetMeter( XOSView *parent, kstat_ctl_t *_kc, float max )
  : FieldMeterGraph( parent, 3, "NET", "IN/OUT/IDLE" ){
  kc = _kc;
  maxpackets_ = max;
  nnet=0;
  kstat_named_t     *k;
  for (kstat_t *ksp = kc->kc_chain; ksp != NULL && nnet <NNETS;
       ksp = ksp->ks_next) {
    if (ksp->ks_type == KSTAT_TYPE_NAMED ){
      if(kstat_read(kc, ksp, NULL) != -1 && strncmp(ksp->ks_name, "lo0",3)) {
	k = (kstat_named_t *)(ksp->ks_data);
	if(!strncmp(k->name, "ipackets",8)) {
	  nnets[nnet] = ksp;
	  packetsize[nnet] = GUESS_MTU;
	  // check if bytes r/w also available
	  for(unsigned int j=0; j< ksp->ks_ndata; j++, k++) {
	    if(!strncmp(k->name, "obytes", 6)) { // search for byte fields
	      packetsize[nnet] = 1;
	    }
	  }
#ifdef DEBUG
	  printf("Found NET: %s %s\n", ksp->ks_name,
		 packetsize[nnet] == 1 ? "(Bytes)" : "(Packets Only)");
#endif		 
	  nnet++;
	}
      }
    }

  }
  IntervalTimerStart();
  total_ = max;
  _lastBytesIn = _lastBytesOut = 0;

}

NetMeter::~NetMeter( void ){
}

void NetMeter::checkResources( void ){
  FieldMeterGraph::checkResources();

  setfieldcolor( 0, parent_->getResource("netInColor") );
  setfieldcolor( 1, parent_->getResource("netOutColor") );
  setfieldcolor( 2, parent_->getResource("netBackground") );
  priority_ = atoi (parent_->getResource("netPriority") );
  dodecay_ = parent_->isResourceTrue("netDecay");
  useGraph_ = parent_->isResourceTrue("netGraph");
  SetUsedFormat (parent_->getResource("netUsedFormat"));

}

void NetMeter::checkevent( void ){
  
  //  Reset total_ to expected maximum.  If it is too low, it
  //  will be adjusted in adjust().  bgrayson
  total_ = maxpackets_;
  
  fields_[0] = fields_[1] = 0;
  long long nowBytesIn=0, nowBytesOut=0, mtu;
  
  IntervalTimerStop();
  kstat_named_t     *k;
  for (int i = 0; i < nnet; i++) { // see man kstat......
    kstat_t *ksp=nnets[i];
    k = (kstat_named_t *)(ksp->ks_data);
    if (kstat_read(kc, ksp, 0) == -1)
      continue;
    
    mtu = packetsize[i];
    for(unsigned int j=0, found=0; j< ksp->ks_ndata && found < 2; j++, k++) {
      if(!strncmp(k->name, mtu == 1 ? "rbytes" : "ipackets", 5)) {
	nowBytesIn += k->value.ul*mtu;
	found++;
      } else if(!strncmp(k->name, mtu == 1 ? "obytes" :"opackets", 5)) {
	nowBytesOut += k->value.ul*mtu;
	found++;
      }
	
    }
  }

  long long correction = 0x10000000;
  correction *= 0x10;
  /*  Deal with 32-bit wrap by making last value 2^32 less.  Yes,
   *  this is a better idea than adding to nowBytesIn -- the
   *  latter would only work for the first wrap (1+2^32 vs. 1)
   *  but not for the second (1+2*2^32 vs. 1) -- 1+2^32 -
   *  (1+2^32) is still too big.  */
  if (nowBytesIn < _lastBytesIn) _lastBytesIn -= correction;
  if (nowBytesOut < _lastBytesOut) _lastBytesOut -= correction;
  if(_lastBytesIn == 0) _lastBytesIn = nowBytesIn;
  if(_lastBytesOut == 0) _lastBytesOut = nowBytesOut;
  float t = (1.0) / IntervalTimeInSecs();
  fields_[0] = (float)(nowBytesIn - _lastBytesIn) * t;
  _lastBytesIn = nowBytesIn;
  fields_[1] = (float)(nowBytesOut - _lastBytesOut) * t;
  _lastBytesOut = nowBytesOut;
//  End BSD-specific code.  BCG

  adjust();
  fields_[2] = total_ - fields_[0] - fields_[1];
    /*  The fields_ values have already been scaled into bytes/sec by
     *  the manipulations (* t) above.  */
  setUsed (fields_[0]+fields_[1], total_);
  IntervalTimerStart();
  drawfields();
}

void NetMeter::adjust(void){
  if (total_ < (fields_[0] + fields_[1]))
    total_ = fields_[0] + fields_[1];
}

