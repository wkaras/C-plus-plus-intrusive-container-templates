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

// Unit testing for hash_table.h .

#include "hash_table.h"
#include "hash_table.h"

#include "list.h"

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

const unsigned Num_elem = 30;

const unsigned Num_buckets = 10;

struct Elem
  {
    int key;
    Elem *link;

    static const unsigned Bad_key = Num_buckets * 10;

    void make_detached() { key = Bad_key; link = this; }

    bool is_detached() const
      { return((key == Bad_key) and (link == this)); }
  };

Elem e[30];

class Abs
  {
  private:

    struct List_abs
      {
        static const bool store_tail = false;
        typedef Elem *handle;
        static handle null() { return(nullptr); }
        static handle link(handle h) { return(h->link); }
        static void link(handle h, handle link_h) { h->link = link_h; }
      };

  protected:

    typedef abstract_container::list<List_abs> list;
    typedef unsigned index;

    static const index num_hash_values = Num_buckets;

    typedef int key;

    bool is_key(key k, Elem *h) { return(h->key == k); }

    index hash_key(key k) { return(k / 10); }

    index hash_elem(Elem *h) { return(h->key / 10); }
  };

struct Ht : public hash_table<Abs>
  {
    typedef list p_list;

    p_list & p_bucket(index hash_value) { return(bucket(hash_value)); }
  };

Ht ht;

// Mark all elements as detached.
//
void detach_all()
  {
    ht.purge();

    for (unsigned i = 0; i < Num_elem; ++i)
      e[i].make_detached();
  }

// Check if hash table is sane.
//
void scan()
  {
    unsigned cnt = 0;

    for (unsigned i = 0; i < Num_elem; ++i)
      if (!e[i].is_detached())
        {
          ++cnt;

          CHK(e[i].key < int(10 * Num_buckets));

          Ht::p_list & b = ht.p_bucket(e[i].key / 10);

          Elem *ep = b.start();

          for ( ; ; )
            {
              CHK(ep != Ht::null());

              if (ep == (e + i))
                break;

              ep = b.link(ep);
            }

          CHK(ht.search(e[i].key) == (e + i));
        }

    unsigned icnt = 0;

    for (Ht::iter it(ht); it; ++it)
      {
        ++icnt;

        CHK(!(*it)->is_detached());
      }

    CHK(cnt == icnt);

  } // end scan()

#define SCAN { std::cout << "SCAN line " << __LINE__ << std::endl; scan(); }

int main()
  {
    detach_all(); SCAN

    e[0].key = 1;
    ht.insert(e + 0);
    SCAN
    e[1].key = 20;
    ht.insert(e + 1);
    SCAN
    e[2].key = 30;
    ht.insert(e + 2);
    SCAN
    e[3].key = 50;
    ht.insert(e + 3);
    SCAN
    e[4].key = 70;
    ht.insert(e + 4);
    SCAN
    ht.remove(e + 3);
    e[3].make_detached();
    SCAN
    e[3].key = 50;
    ht.insert(e + 3);
    SCAN
    e[5].key = 51;
    ht.insert(e + 5);
    SCAN
    e[6].key = 52;
    ht.insert(e + 6);
    SCAN
    ht.remove_key(51);
    e[5].make_detached();
    SCAN
    ht.remove_key(50);
    e[3].make_detached();
    SCAN
    ht.remove_key(52);
    e[6].make_detached();
    SCAN

    ht.purge();
    detach_all();
    SCAN

    unsigned idx = 0;

    #define I(K) e[idx].key = (K); ht.insert(e + (idx++)); SCAN

    I(11)
    I(12)
    I(31)
    I(32)
    I(33)
    I(81)
    I(82)
    I(1)
    I(2)
    I(91)
    I(92)

    return(0);
  }
