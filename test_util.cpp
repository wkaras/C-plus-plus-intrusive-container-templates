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

// Unit testing for util.h .

#include "util.h"
#include "util.h"

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

struct A { int i, j; };

struct B { int m, n; A a; };

int main()
  {
    B b;

    CHK(&b == ABSTRACT_CONTAINER_MBR_TO_CLS_PTR(B, m, &b.m));
    CHK(&b == ABSTRACT_CONTAINER_MBR_TO_CLS_PTR(B, n, &b.n));
    CHK(&b == ABSTRACT_CONTAINER_MBR_TO_CLS_PTR(B, a.i, &b.a.i));
    CHK(&b == ABSTRACT_CONTAINER_MBR_TO_CLS_PTR(B, a.j, &b.a.j));
    CHK(&b.a == ABSTRACT_CONTAINER_MBR_TO_CLS_PTR(A, j, &b.a.j));

    return(0);
  }
