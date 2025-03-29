/*
Copyright (c) 2026 Walter William Karas

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

#if defined(TESTLOOP_H_20250721)

#error only include once

#else

#define TESTLOOP_H_20250721

#endif

#include <cstdlib>
#include <iostream>
#include <vector>

namespace
{

class test_base;

std::vector<test_base *> test_list;

// Function to set breakpoints on for debugging
void pre_break() { }

class test_base
  {
  public:

    test_base() { test_list.push_back(this); }

    void operator () ()
      {
        pre_break();
        test();
      }

  private:

    virtual void test() = 0;
  };

void one_test(unsigned tno)
  {
    std::cout << "Test " << tno << '\n';
    (*(test_list[tno]))();
  }

[[noreturn]] void failure()
  {
    std::cout << "Test failed\n";
    std::exit(1);
  }

} // end anonymous namespace

int main(int n_arg, const char * const * arg)
  {
    if (n_arg < 3)
      {
        if (n_arg == 2)
          {
            // Only parameter is the number of the test to run.

            unsigned tno = static_cast<unsigned>(atoi(arg[1]));

            if (tno < test_list.size())
              one_test(static_cast<unsigned>(tno));
            else
              std::cout << "test number must be less than " << test_list.size()
                   << '\n';
          }
        else
          // Run all tests.
          //
          for (unsigned tno = 0; tno < test_list.size(); ++tno)
            one_test(tno);
      }

    return(0);
  }
