//
//  Copyright (c) 1994, 1995, 2006 by Mike Romberg ( mike.romberg@noaa.gov )
//
//  This file may be distributed under terms of the GPL
//

#ifndef _pllist_h
#define _pllist_h

#include "llist.h"

//-------------------------------------------------------------------------
//
// Pointer Linked list.  T is some pointer type.
//
//-------------------------------------------------------------------------

template <class T>
class PLList : public LList
    {
    public:
        PLList(void) : LList(){}
//        PLList(int(*cmp_fun)(T data, K key)) : LList(cmp_fun){}

        int push(const T data) { return LList::push((void *)data); }
        T pop(void) { return (T)LList::pop(); }

        int enqueue(const T data) { return LList::enqueue((void *)data); }
        T dequeue(void) { return (T)LList::dequeue(); }

//        int insert(const T data, const K key)
//          { return LList::insert((void *)data, (void *)key); }
//        T find(const K key) { return (T)LList::find((void *)key); }
//        T removematch(const K key)
//          { return (T)LList::removematch((void *)key); }

        int putontop(const T data) { return LList::putontop((void *)data); }
        void remove(const T data) { LList::remove((void *)data); }
        T findn(int n) { return (T)LList::findn(n); }
        T operator[](int n) { return findn(n); }
        int index(const T data) { return LList::index((void *)data); }

        T findc(int which = 0) { return (T)LList::findc(which); }
    };


//-------------------------------------------------------------------------
//
// Sorted Pointer List.  T is some type of pointer and K is a pointer to
// the key type for this list.
//
//-------------------------------------------------------------------------
template <class T, class K>
class PSLList : public LList
    {
    public:
        PSLList(void) : LList(){}
        PSLList(int(*cmp_fun)(T data, K key)) : LList(cmp_fun){}

        int push(const T data) { return LList::push((void *)data); }
        T pop(void) { return (T)LList::pop(); }

        int enqueue(const T data) { return LList::enqueue((void *)data); }
        T dequeue(void) { return (T)LList::dequeue(); }

        int insert(const T data, const K key)
          { return LList::insert((void *)data, (void *)key); }
        T find(const K key) { return (T)LList::find((void *)key); }
        T removematch(const K key)
          { return (T)LList::removematch((void *)key); }

        int putontop(const T data) { return LList::putontop((void *)data); }
        void remove(const T data) { LList::remove((void *)data); }
        T findn(int n) { return (T)LList::findn(n); }
        T operator[](int n) { return findn(n); }
        int index(const T data) { return LList::index((void *)data); }

        T findc(int which = 0) { return (T)LList::findc(which); }
    };


#endif
