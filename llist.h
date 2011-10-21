//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef __LLIST_H__
#define __LLIST_H__

#include <stdio.h>

#define MAXCURR 4    //  number of 'current' pointers

class LList {
public:
  //	The first constructor is used for unordered lists ( stacks, queques ).
  //  The second constructor is used for ordered lists. It's
  //  parameter is a pointer to the function to be used for ordering the
  //  list.  This function must behave in the following way:
  //		int Cmp_Fun ( void *data, void *key )
  //
  //		*data is a pointer to the type of information
  //		to be stored in the list.
  //
  //		*key is a pointer to the location within the data
  //		which will be used to order the list.  In some
  //		cases these two pointers will be of the same
  //		type.
  //
  //		function returns :
  //
  //		0.........*data = *key
  //		> 0.......*data > *key
  //		< 0.......*data < *key

  LList( void );					//  for unordered lists
  LList( int ( *cmp_fun )( void *data, void *key ) );	//  for ordered lists

  virtual ~LList( void );  //  frees nodes but not data

  //	Stack like functions.
  //  pop returns NULL if the list is empty.
  //  push returns 1 on sucess and 0 upon failure.
  int push( void *data );
  void *pop( void );

  //	Queue like functions
  //  dequeue returns NULL if the list is empty.
  //  enqueue returns 1 on sucess and 0 upon failure.
  int enqueue( void *data ) { return( push( data ) ); }
  void *dequeue( void );

  //	Ordered list functions
  //  for insert *key points to some part of *data used for sorting.
  //  for find and removematch *key points to the "search" key.
  //  - insert returns 1 on sucess and 0 on failure
  //  - find and removematch return a pointer to the data stored in the list if
  //	sucessful and NULL if not.
  int insert( void *data, void *key );
  void *find( void *key );
  void *removematch( void *key );

  //	Misc. llist functions
  int putontop( void *data );  //  oposite of push
  void remove( void *data );   //  removes *data from the list if it's there
  void *findn( int n ); //  returns nth element if found or NULL if not
  void *operator[](int n)  //  returns nth element if found or NULL if not
    { return findn(n); }
  int index( void *data );     //  returns the index of *data or 0 if not there

  //	Sets the a pointer for the linked list to the nth element in
  //  the list. This function and the three that follow are intended to
  //  be used for a stepwise search of a linked list.  This function sets
  //  curr_ to the nth element in the list.  incc sets the same pointer
  //  to the next element in the list.	decc sets it to the previous
  //  element in the list.  findc returns a pointer to the element
  //  curr_ is "currently" pointing to. If n is not valid for the list
  //  curr_ is set to NULL.
  void setc( int n, int which = 0 );
  void incc( int which = 0 );
  void decc( int which = 0 );
  void *findc( int which = 0 );

  //	This function will save a linked list to the file pointed to
  //  by *fp.  The file should be binary.  n is saved first and then
  //  each item on the list is saved.  size is the number of bytes of
  //  each item on the list.  The list will remain intact after this
  //  function is called.
  void save( int size, FILE *fp );

  //	This function reads a linked list from the file pointed to
  //  by *fp ( previously saved by the above function ).  Space is
  //  allocated for each item, and it is placed into the list in it's
  //  old position.  size is the number of bytes each item occupies.
  //  This function will return 1 upon sucess and 0 upon failure.
  int restore( int size, FILE *fp );

  //	This function will remaove all of the elements in the linked
  //  list pointed to by L and free the memory each element occupied.
  void kill( void );

  int n( void ) const { return( n_ ); }     //  number of elements in the list
protected:

  class LNode {
  public:
    LNode( void *data = NULL );

    void *data_;
    LNode *next_;
    LNode *prev_;
  };

  int n_;			//  number of nodes in the list
  LNode *top_, *btm_;	        //  pointers to various nodes
  LNode *curr_[MAXCURR];

  //  a comparison function for ordered lists
  int ( *cmp_fun_ )( void *data, void *key );
  LNode *findnode( void *key );
  void *deletenode( LNode *ptr );
  LNode *findnnode( int i );
private:
};

#endif
