/*
Copyright (c) 2016 Walter William Karas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Unit testing for list.h and bidir_list.h .

#define BIDIR 1
#define STORE_TAIL true

#if BIDIR

#include "bidir_list.h"
#include "bidir_list.h"

#else

#include "list.h"
#include "list.h"

#endif

// Put a breakpoint on this function to break after a check fails.
void bp() { }

#include <cstdlib>
#include <iostream>

void check(bool expr, int line)
  {
    if (!expr)
      {
        std::cout << "*** fail line " << line << std::endl;
        bp();
        std::exit(1);
      }
  }

#define CHK(EXPR) check((EXPR), __LINE__)

using namespace abstract_container;

#if BIDIR

typedef p_bidir_list list_t;

#else

typedef p_list<STORE_TAIL> list_t;

#endif

list_t lst;

const unsigned num_e = 5;

typedef list_t::elem elem_t;

elem_t e[num_e];

// Purge test list, mark all elements as detached.
//
void init()
  {
    lst.purge();

    for (unsigned i = 0; i < num_e; ++i)
      lst.make_detached(e + i);
  }

// Check if list structure is sane, and elements of list are in ascending
// order by address.
//
void scan()
  {
    elem_t *last = nullptr;

    for (unsigned i = 0; i < num_e; ++i)
      if (!lst.is_detached(e + i))
        {
          #if BIDIR
          CHK(lst.link(e + i, reverse) == last);
          #endif
          if (last)
            CHK(lst.link(last) == (e + i));
          else
            CHK(lst.start() == (e + i));
          last = e + i;
        }

    CHK(lst.start(reverse) == last);
    CHK(lst.empty() == (last == nullptr));
    if (last)
      CHK(lst.link(last) == nullptr); 
    else
      CHK(lst.start() == nullptr);
  }

#define SCAN { std::cout << "SCAN line " << __LINE__ << std::endl; scan(); }

int main()
  {
    #if BIDIR or STORE_TAIL
    CHK(sizeof(lst) == (2 * sizeof(void *)));
    #else
    CHK(sizeof(lst) == sizeof(void *));
    #endif

    #if BIDIR
    CHK(sizeof(e[0]) == (2 * sizeof(void *)));
    #else
    CHK(sizeof(e[0]) == sizeof(void *));
    #endif

    CHK(lst.empty());

    init(); SCAN

    lst.push(e + 2); SCAN
    lst.insert(e + 2, e + 4); SCAN
    lst.insert(e + 2, e + 0, reverse); SCAN
    lst.insert(e + 2, e + 3); SCAN
    lst.insert(e + 2, e + 1, reverse); SCAN

    lst.remove(e + 2); lst.make_detached(e + 2); SCAN
    lst.remove(e + 0); lst.make_detached(e + 0); SCAN
    lst.remove(e + 4); lst.make_detached(e + 4); SCAN
    lst.remove(e + 3); lst.make_detached(e + 3); SCAN
    lst.remove(e + 1); lst.make_detached(e + 1); SCAN

    CHK(lst.empty());

    #if !BIDIR

    lst.push(e + 2); SCAN
    lst.insert(e + 2, e + 4); SCAN
    lst.insert(e + 2, e + 0, reverse); SCAN
    lst.insert(e + 2, e + 3); SCAN
    lst.insert(e + 2, e + 1, reverse); SCAN

    lst.remove_forward(e + 0); lst.make_detached(e + 1); SCAN
    lst.remove_forward(e + 0); lst.make_detached(e + 2); SCAN
    lst.remove_forward(e + 0); lst.make_detached(e + 3); SCAN
    lst.remove_forward(e + 0); lst.make_detached(e + 4); SCAN
    lst.pop(); lst.make_detached(e + 0); SCAN

    CHK(lst.empty());

    #endif

    lst.push(e + 2); SCAN
    lst.pop(); lst.make_detached(e + 2); SCAN
    lst.push(e + 2, reverse); SCAN
    lst.pop(reverse); lst.make_detached(e + 2); SCAN
    lst.push(e + 2); SCAN
    lst.push(e + 1); SCAN
    lst.push(e + 3, reverse); SCAN
    lst.pop(reverse); lst.make_detached(e + 3); SCAN
    lst.pop(); lst.make_detached(e + 1); SCAN

    lst.purge();
    CHK(lst.empty());

    return(0);
  }
