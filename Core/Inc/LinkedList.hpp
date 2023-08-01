////////////////////////////////////////////////////////////////////////////////
// LinedList.hpp
// A FIFO linked list.
// Elements must have a "ListNext" data member.
// This version is not lock-free, and may need to be protected by a critical region.
//
// Copyright (c) 2023 Jonathan Engdahl
// BSD license -- see the accompanying LICENSE file
//
///////////////////////////////////////////////////////////////////////////////


#ifndef LinkedList_HPP
#define LinkedLIst_HPP

#include "cmsis.h"


///////////////////////////////////////////////////////////////////////////////
// class LinkedList
//
// template parameter T      - Type of data this FIFO will hold.
//
/////////////////////////////////////////////////////////////////////////////

template<typename T>
class LinkedList
    {
    protected:

    T *head = 0;
    T **tail = &head;


    public:

    ///////////////////////////////////////////////////////////////////////////////
    //  LinkedList::operator bool
    //
    // returns true if list has any content
    ///////////////////////////////////////////////////////////////////////////////

    inline operator bool() { return head != 0; }


    ///////////////////////////////////////////////////////////////////////////////
    //  LinkedList::add
    //
    // Adds an entry to the list.
    //
    // input: value  The value to be added.
    ///////////////////////////////////////////////////////////////////////////////

    void add(T *value)
        {
        value->ListNext = 0;
        *tail = value;
        tail = &value->ListNext;
        }


    /////////////////////////////////////////////////////////////////////////////
    //  LinkedList::take
    //
    // Takes an entry from the list.
    //
    // arg: reference to where to put the value taken
    // return: true if a value was taken, false if the list was empty
    ////////////////////////////////////////////////////////////////////////////

    bool take(T *&value)
        {
        T *x;

        x = head;
        if(x == 0)return false;
        head = x->ListNext;
        if(head == 0)
            {
            tail = &head;
            }
        x->ListNext = 0;
        value = x;
        return true;
        }
    };


#endif // LINKEDLIST_HPP
