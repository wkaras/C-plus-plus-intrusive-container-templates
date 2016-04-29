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

// Unit testing for circ_que.h (single thread only).

#include "circ_que.h"
#include "circ_que.h"

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

const unsigned Max_elems = 5;

typedef basic_circ_que<int, Max_elems> bcq_t;

struct One_test
  {
    bcq_t cq;

    circ_que_front<bcq_t> cqf;
    circ_que_back<bcq_t> cqb;

    int in_val, out_val;

    unsigned size;

    void sane()
      {
        CHK(cqf.size() == size);
        CHK(cqf.is_empty() == (size == 0));

        CHK(cqb.size() == size);
        CHK(cqb.is_full() == (size == bcq_t::max_size));

        if (size)
          {
            CHK(cqf() == out_val);
            CHK(cqf(size - 1) == in_val);
          }
      }

    One_test(unsigned push, unsigned pop, unsigned push2) : cqf(cq), cqb(cq)
      {
        in_val = 10;
        out_val = 10;
        size = 0;

        sane();

        for (unsigned i = 0; i < push; ++i)
          {
            switch (size % 3)
              {
              case 0:
                cqb() = in_val;
                cqb.push(raw);
                break;

              case 1:
                cqb.init(in_val);
                cqb.push(raw);
                break;

              case 2:
                cqb.push(in_val);
                break;
              }
            ++size;

            sane();

            in_val += 10;
          }

        in_val -= 10;

        for (unsigned i = 0; i < pop; ++i)
          {
            if (size & 1)
              cqf.pop(raw);
            else
              cqf.pop();
            --size;
            out_val += 10;

            sane();
          }

        in_val += 10;

        for (unsigned i = 0; i < push2; ++i)
          {
            cqb() = in_val;
            cqb.push(raw);
            ++size;

            sane();

            in_val += 10;
          }

        in_val -= 10;

        pop = push - pop + push2;

        for (unsigned i = 0; i < pop; ++i)
          {
            cqf.pop(raw);
            --size;
            out_val += 10;

            sane();
          }

        CHK(size == 0);
      }
  };

struct Purge_test
  {
    bcq_t cq;

    circ_que_back<bcq_t> cqb;

    Purge_test() : cqb(cq)
      {
        cqb.push(1);
        cqb.push(2);
        cqb.push(3);

        CHK(cqb.size() == 3);

        cq.purge();

        CHK(cqb.size() == 0);
      }
  };

int main()
  {
    for (unsigned push = 0; push <= Max_elems; ++push)
      for (unsigned pop = 0; pop <= push; ++pop)
        for (unsigned push2 = 0; push2 <= (Max_elems - push + pop); ++push2)
          {
            One_test(push, pop, push2);
          }

    Purge_test();

    return(0);
  }
