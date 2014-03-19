//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#include "llist.h"
#include <stdio.h>

LList::LNode::LNode( void *data ){
  data_ = data;
  next_ = NULL;
  prev_ = NULL;
}

LList::LList( void ){
  n_ = 0;
  top_ = NULL;
  btm_ = NULL;
  for ( int i = 0 ; i < MAXCURR ; i++ )
    curr_[i] = NULL;
  cmp_fun_ = NULL;
}

LList::LList( int ( *cmp_fun )( void *data, void *key ) ){
  n_ = 0;
  top_ = NULL;
  btm_ = NULL;
  for ( int i = 0 ; i < MAXCURR ; i++ )
    curr_[i] = NULL;
  cmp_fun_ = cmp_fun;
}

LList::~LList( void ){
  while ( n_ )
    pop();
}

int LList::push( void *data ){
  if ( !n_ ) {
    top_ = new LNode( data );
    if ( top_ == NULL ) return ( 0 );
    n_ = 1;
    btm_ = top_;
    return ( 1 );
  }

  btm_->next_ = new LNode( data );
  if ( btm_->next_ == NULL ) return ( 0 );
  n_++;
  btm_->next_->prev_ = btm_;
  btm_ = btm_->next_;
  return ( 1 );
}

void *LList::pop( void ){
  void *temp;
  LNode *temp2;

  if ( !n_ ) return ( NULL );

  temp = btm_->data_;
  if ( n_ == 1 ) {
    delete top_;
    top_ = NULL;
    btm_ = NULL;
    n_ = 0;
    return ( temp );
  }

  n_--;
  temp2 = btm_->prev_;
  btm_->prev_->next_ = NULL;
  delete btm_;
  btm_ = temp2;
  return ( temp );
}

void *LList::dequeue( void ){
  void *temp;
  LNode *temp2;

  if ( !n_ ) return ( NULL );

  if ( n_ == 1 ) {
    temp = top_->data_;
    n_ = 0;
    delete top_;
    top_ = NULL;
    btm_ = NULL;
    return ( temp );
  }

  n_--;
  temp2 = top_->next_;
  temp = top_->data_;
  top_->next_->prev_ = NULL;
  delete top_;
  top_ = temp2;
  return ( temp );
}

int LList::insert( void *data, void *key ){
  LNode *current, *temp;

  current = top_;

  /*  Empty List  */
  if ( !n_ ) {
    if ( ( top_ = new LNode( data ) ) == NULL ) return ( 0 );
    btm_ = top_;
    n_++;
    return ( 1 );
  }

  while ( ( cmp_fun_( current->data_, key ) < 0 ) &&
	  ( current->next_ != NULL ) )
    current = current->next_;

  if ( ( temp = new LNode( data ) ) == NULL ) return ( 0 );

  n_++;

  /*  Add To End of List  */
  if ( ( current->next_ == NULL ) &&
       ( cmp_fun_( current->data_, key ) < 0 ) ) {
    temp->prev_ = btm_;
    current->next_ = temp;
    btm_ = temp;
    return ( 1 );
  }

  /*  Add To Top of List  */
  if ( ( current->prev_ == NULL ) &&
       ( cmp_fun_( current->data_, key ) > 0 ) ) {
    temp->next_ = current;
    current->prev_ = temp;
    top_ = temp;
    return ( 1 );
  }

  /*  Middle Of List  */
  temp->next_ = current;
  temp->prev_ = current->prev_;
  current->prev_->next_ = temp;
  current->prev_ = temp;
  return ( 1 );
}

void *LList::find( void *key ){
  LNode *temp;

  temp = findnode( key );
  if ( temp == NULL ) return ( NULL );
  return ( temp->data_ );
}

void *LList::removematch( void *key ){
  LNode *ptr;

  ptr = findnode( key );

  if ( ptr == NULL ) return ( NULL );

  return ( deletenode( ptr ) );
}

int LList::putontop( void *data ){
  LNode *buff;

  if ( ( buff = new LNode( data ) ) == NULL ) return ( 0 );

  top_->prev_ = buff;
  buff->next_ = top_;
  top_ = buff;
  n_++;
  return ( 1 );
}

void LList::remove( void *data ){
  LNode *tmp;
  int found = 0;

  tmp = top_;
  while ( (!found) && (tmp != NULL) ) {
    if ( tmp->data_ == data ) found = 1;
    if ( !found ) tmp = tmp->next_;
  }

  if ( (tmp == NULL) || (!found) ) return;

  deletenode( tmp );
}

void *LList::findn( int n ){
  LNode *temp;

  temp = findnnode( n );
  if ( temp == NULL ) return ( NULL );
  return ( temp->data_ );
}

int LList::index( void *data ){
  int a = 1;
  LNode *tmp;

  tmp = top_;
  while ( tmp != NULL ) {
    if ( tmp->data_ == data ) return ( a );
    a++;
    tmp = tmp->next_;
  }
  return ( 0 );
}

void LList::setc( int n, int which ){
  curr_[which] = findnnode( n );
}

void LList::incc( int which ){
  if ( curr_[which] != NULL ) curr_[which] = curr_[which]->next_;
}

void LList::decc( int which ){
  if ( curr_[which] != NULL ) curr_[which] = curr_[which]->prev_;
}

void *LList::findc( int which ){
  if ( curr_[which] == NULL ) return ( NULL );

  return ( curr_[which]->data_ );
}

void LList::save( int size, FILE *fp ){
  int i;
  void *buf;

  fwrite( &n_, sizeof( int ), 1, fp );	/*  save n  */

  setc( 1 );
  for ( i = 1 ; i <= n_ ; i ++ ) {
    buf = findc();
    fwrite ( buf, size, 1, fp );
    incc();
  }
}

int LList::restore( int size, FILE *fp ){
  int i;
  void *buf;

  fread ( &i, sizeof ( int ), 1, fp );

  for ( ; i > 0 ; i-- ) {
    if ( ( buf = new char[size] ) == NULL ) return ( 0 );
    if ( ! push( buf ) ) return ( 0 );
  }

  return ( 1 );
}

void LList::kill( void ){
//  while ( n_ ) {
//    delete pop();
//  }
}





LList::LNode *LList::findnode( void *key ){
  LNode *current;

  current = top_;

  if ( current == NULL ) return ( NULL );

  while ( ( cmp_fun_( current->data_, key ) ) &&
	  ( current != NULL ) )
    current = current->next_;

  if ( current == NULL ) return ( NULL );

  return ( current );
}

void *LList::deletenode( LNode *ptr ){
  void *rtn;

  if ( ( top_ == NULL ) || ( ptr == NULL ) ) return ( NULL );

  if ( n_ == 1 ) {
    rtn = top_->data_;
    n_ = 0;
    delete top_;
    top_ = btm_ = NULL;
    return ( rtn );
  }

  n_--;

  if ( ptr->prev_ == NULL ) {
    rtn = ptr->data_;
    top_ = top_->next_;
    top_->prev_ = NULL;
    delete ptr;
    return ( rtn );
  }

  if ( ptr->next_ == NULL ) {
    rtn = ptr->data_;
    btm_ = btm_->prev_;
    btm_->next_ = NULL;
    delete ptr;
    return ( rtn );
  }

  ptr->prev_->next_ = ptr->next_;
  ptr->next_->prev_ = ptr->prev_;
  rtn = ptr->data_;
  delete ptr;
  return ( rtn );
}

LList::LNode *LList::findnnode( int i ){
  int j;
  LNode *current;

  if ( (i > n_) || (i < 1) ) return ( NULL );

  if ( i <= n_ / 2 ) {
    current = top_;
    for ( j = 1 ; j < i ; j++ ) current = current->next_;
    return ( current );
  }

  current = btm_;
  for ( j = n_ ; j > i ; j-- ) current = current->prev_;
  return ( current );
}
