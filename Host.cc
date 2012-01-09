//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include <stdlib.h>
#include <string.h>
#include "Host.h"

#if defined(__hpux__) || defined(__hpux)
extern int h_errno;
#endif


Host::Host(const char *hostname){
  struct hostent *hent = gethostbyname(hostname);

  check(hent);

  copy(hent);
}

Host::Host(const struct in_addr *address){
  constuct(address);
}

Host::Host(unsigned int addr){
  struct in_addr ia;
  ia.s_addr = htonl(addr);
  constuct(&ia);
}

bool Host::constuct(const struct in_addr *address){
  struct hostent *hent = gethostbyaddr((char *)address, sizeof(in_addr),
                                       AF_INET);
  bool tmp = check(hent);

  copy(hent);

  return tmp;
}

void Host::copy(const struct hostent *hent){
  _hent.h_name = NULL;
  _hent.h_aliases = NULL;
  _numAliases = 0;
  _hent.h_addrtype = -1;
  _hent.h_length = -1;
  _numAddresses = 0;
  _hent.h_addr_list = NULL;
  _valid = false;

  if (hent != NULL){
    // Then it is valid.
    _valid = true;

    // find the number of aliases.
    char **tmp = hent->h_aliases;
    while (*tmp != NULL){
      _numAliases++;
      tmp++;
    }

    // copy the official name.
    int hnamelen = strlen(hent->h_name) + 1;
    _hent.h_name = new char[hnamelen];
    strncpy((char *)_hent.h_name, hent->h_name, hnamelen);

    // copy the aliases.
    _hent.h_aliases = new char *[_numAliases + 1];
    for (int i = 0 ; i < _numAliases ; i++){
      int len = strlen(hent->h_aliases[i]) + 1;
      _hent.h_aliases[i] = new char[len + 1];
      strncpy(_hent.h_aliases[i], hent->h_aliases[i], len);
    }
    _hent.h_aliases[_numAliases] = NULL;

    // copy the address type and length.
    _hent.h_addrtype = hent->h_addrtype;
    _hent.h_length = hent->h_length;

    // find the number of addresses.
    char **taddr = hent->h_addr_list;
    while (*taddr != NULL){
      _numAddresses++;
      taddr++;
    }

    // copy the addresses.
    _hent.h_addr_list = new char *[_numAddresses + 1];
    for (int j = 0 ; j < _numAddresses ; j++){
      _hent.h_addr_list[j] = (char *)new in_addr;
      memcpy(_hent.h_addr_list[j], hent->h_addr_list[j], sizeof(in_addr));
    }
    _hent.h_addr_list[_numAddresses] = NULL;
  }
}

Host::~Host(void){
  clear();
}

void Host::clear(void){
  _valid = false;

  delete[] _hent.h_name;
  _hent.h_name = NULL;

  for (int i = 0 ; i < _numAliases ; i++)
    delete[] _hent.h_aliases[i];
  delete[] _hent.h_aliases;
  _hent.h_aliases = NULL;

  for (int j = 0 ; j < _numAddresses ; j++)
    delete _hent.h_addr_list[j];
  delete[] _hent.h_addr_list;
  _hent.h_addr_list = NULL;

  _numAliases = 0;
  _hent.h_addrtype = -1;
  _hent.h_length = -1;
  _numAddresses = 0;
}

bool Host::check(const struct hostent *hent) {
  if (hent != NULL)  // all is well
    return true;

  _valid = false;
  _failure = h_errno;
  return false;
}

int Host::reasonForFailure(void) const{
  if (_valid)
    return 0; //NETDB_SUCCESS;

  return _failure;
}

bool Host::tryAgain(void) const {
  if (reasonForFailure() == TRY_AGAIN)
    return true;

  return false;
}

bool Host::operator==(const Host& host) const {
  if (valid() != host.valid())
    return false;

  if (!valid())  // they are both invalid
    return true;

  // now check all of the addresses for each host.
  for (int i = 0 ; i < _numAddresses ; i++)
    for (int j = 0 ; j < host.numAddresses() ; j++)
      if (address(i)->s_addr == host.address(j)->s_addr)
        return true;

  return false;
}

std::ostream &Host::print(std::ostream& os) const {
  /*  Cast 'this' to a char*, so we don't need to create a Host::! operator.*/
  if (!*((char*)this))
    return os <<"Invalid Host.  h_errno was = " <<_failure <<"\n";

  os <<"---- Host ----\n"
     <<"Official Name    = " <<officialName() <<"\n"
     <<"Aliases          = ";

  for (int i = 0 ; i < numAliases() ; i++){
    os <<alias(i);
    if (i != numAliases() - 1)
      os <<", ";
  }
  os <<"\n";

  os <<"Address Type     = " <<addrType() <<"\n"
     <<"Address Length   = " <<addrLength() <<"\n"
     <<"Addresses        = ";

  for (int j = 0 ; j < numAddresses() ; j++){
    os <<strAddress(j);
    if (j != numAddresses() - 1)
      os <<", ";
  }

  return os;
}
