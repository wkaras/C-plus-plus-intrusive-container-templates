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

// Unit testing for circ_que_lock_free.h.

#include "circ_que_lock_free.h"
#include "circ_que_lock_free.h"

// Put a breakpoint on this function to break after a check fails.
void bp() { }

#include <cstdlib>
#include <iostream>
#include <thread>
#include <chrono>

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

const unsigned Max_elems = 5;

typedef circ_que_lock_free<unsigned, Max_elems> cq_t;

cq_t cq;

bool full_seen, empty_seen;

void producer()
  {
    circ_que_back<cq_t> cqb(cq);

    for (unsigned i = 0; i < 1000; ++i)
      {
        while (cqb.is_full())
          full_seen = true;;

        cqb.push(i);

        if ((i % 10) == 0)
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
  }

int main()
  {
    std::thread t(producer);

    circ_que_front<cq_t> cqf(cq);

    unsigned size;

    for (unsigned i = 0; i < 1000; ++i)
      {
        while (cqf.is_empty())
          empty_seen = true;;

        CHK(cqf() == i);
        size = cqf.size();
        CHK(cqf(size - 1) == (i + size - 1));

        cqf.pop();

        if ((i % 5) == 0)
          std::this_thread::sleep_for(std::chrono::milliseconds(20));
      }

    CHK(cqf.is_empty());

    t.join();

    CHK(empty_seen);
    CHK(full_seen);

    return(0);
  }
