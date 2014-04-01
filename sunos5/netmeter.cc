//  
//  Rewritten for Solaris by Arno Augustin 1999
//  augustin@informatik.uni-erlangen.de
//

#include "netmeter.h"
#include <stdlib.h>
#include <string.h>


NetMeter::NetMeter( XOSView *parent, kstat_ctl_t *kc, float max )
  : FieldMeterGraph( parent, 3, "NET", "IN/OUT/IDLE" ){
  _kc = kc;
  _maxpackets = max;
  _lastBytesIn = _lastBytesOut = 0;
}

NetMeter::~NetMeter( void ){
}

void NetMeter::checkResources( void ){
  FieldMeterGraph::checkResources();

  setfieldcolor( 0, parent_->getResource("netInColor") );
  setfieldcolor( 1, parent_->getResource("netOutColor") );
  setfieldcolor( 2, parent_->getResource("netBackground") );
  priority_ = atoi( parent_->getResource("netPriority") );
  dodecay_ = parent_->isResourceTrue("netDecay");
  useGraph_ = parent_->isResourceTrue("netGraph");
  SetUsedFormat( parent_->getResource("netUsedFormat") );
  _netIface = parent_->getResource("netIface");
  if (_netIface[0] == '-') {
    _ignored = true;
    _netIface.erase(0, _netIface.find_first_not_of("- "));
  }
}

void NetMeter::checkevent( void ){
  getnetstats();
  drawfields();
}

void NetMeter::getnetstats( void ){
  uint64_t nowBytesIn = 0, nowBytesOut = 0;
  kstat_named_t *k;
  total_ = _maxpackets;

  IntervalTimerStop();
  for (kstat_t *ksp = _kc->kc_chain; ksp != NULL; ksp = ksp->ks_next) {
    if ( ksp->ks_type != KSTAT_TYPE_NAMED ||
         strncmp(ksp->ks_class, "net", 3) ||
         strncmp(ksp->ks_module, "link", 4) )
      continue;
    if ( _netIface != "False" &&
         ( (!_ignored && ksp->ks_name != _netIface) ||
           ( _ignored && ksp->ks_name == _netIface) ) )
      continue;
    if ( kstat_read(_kc, ksp, NULL) == -1 )
      continue;

    XOSDEBUG("%s: ", ksp->ks_name);
    if ( (k = (kstat_named_t *)kstat_data_lookup(ksp, "rbytes64")) == NULL ) {
      if ( (k = (kstat_named_t *)kstat_data_lookup(ksp, "rbytes")) == NULL )
        continue;
      nowBytesIn += k->value.ul;
      XOSDEBUG("%lu bytes received ", k->value.ul);
    }
    else {
      nowBytesIn += k->value.ui64;
      XOSDEBUG("%llu bytes received ", k->value.ui64);
    }

    if ( (k = (kstat_named_t *)kstat_data_lookup(ksp, "obytes64")) == NULL ) {
      if ( (k = (kstat_named_t *)kstat_data_lookup(ksp, "obytes")) == NULL )
        continue;
      nowBytesOut += k->value.ul;
      XOSDEBUG("%lu bytes sent.\n", k->value.ul);
    }
    else {
      nowBytesOut += k->value.ui64;
      XOSDEBUG("%llu bytes sent.\n", k->value.ui64);
    }
  }

  uint64_t correction = 0x10000000;
  correction *= 0x10;
  /*  Deal with 32-bit wrap by making last value 2^32 less.  Yes,
   *  this is a better idea than adding to nowBytesIn -- the
   *  latter would only work for the first wrap (1+2^32 vs. 1)
   *  but not for the second (1+2*2^32 vs. 1) -- 1+2^32 -
   *  (1+2^32) is still too big.  */
  if (nowBytesIn < _lastBytesIn)
    _lastBytesIn -= correction;
  if (nowBytesOut < _lastBytesOut)
    _lastBytesOut -= correction;
  if(_lastBytesIn == 0)
    _lastBytesIn = nowBytesIn;
  if(_lastBytesOut == 0)
    _lastBytesOut = nowBytesOut;

  double t = IntervalTimeInSecs();
  fields_[0] = (double)(nowBytesIn - _lastBytesIn) / t;
  fields_[1] = (double)(nowBytesOut - _lastBytesOut) / t;

  IntervalTimerStart();
  _lastBytesIn = nowBytesIn;
  _lastBytesOut = nowBytesOut;

  if (total_ < fields_[0] + fields_[1])
    total_ = fields_[0] + fields_[1];
  fields_[2] = total_ - fields_[0] - fields_[1];
  setUsed(fields_[0] + fields_[1], total_);
}
