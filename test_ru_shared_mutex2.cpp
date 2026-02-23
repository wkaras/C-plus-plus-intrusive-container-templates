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

// Additional unit test for ru_shared_mutex.  Exercise creation and destruction of threads doing shared access.
// Which exercises the insertion and removal of per-thread mutex data into the linked list.

// Include twice to test guarding against redefinition.
//
//#include "ru_shared_mutex.h"
//#include "ru_shared_mutex.h"

#include <thread>
#include <atomic>
#include <iostream>
#include <cstdlib>

inline void fatal_error(char const *, unsigned)
  {
    std::cout << "fatal error\n";
    std::exit(1);
  }

#include "Bravo.h"

namespace
{

[[noreturn]] void failure()
  {
    std::cout << "Test failed\n";
    std::exit(1);
  }

//abstract_container::ru_shared_mutex::id id;
//auto &sh_mtx{abstract_container::ru_shared_mutex::c<id>::inst()};
ts::bravo::shared_mutex sh_mtx;

enum class cmd_t
  {
    none,
    lock,
    try_lock,
    unlock,
    exit
  };

struct thr_data_t
  {
    // True if thread is running.
    //
    bool active{false};

    std::thread obj;

    // When this is not 'none', the thread function executes the command, then sets this back to 'none'.
    //
    std::atomic<unsigned> cmd;

    bool try_result;
  };

void thr_func(thr_data_t * const thr)
  {
    thr->cmd = unsigned(cmd_t::none);

    ts::bravo::Token tok{0};

    for ( ; ; )
      {
        while (unsigned(cmd_t::none) == thr->cmd.load())
          std::this_thread::yield();

        switch (cmd_t(thr->cmd.load()))
          {
          case cmd_t::lock:
            tok = 0;
            sh_mtx.lock_shared(tok);
            break;
          case cmd_t::try_lock:
            tok = 0;
            thr->try_result = sh_mtx.try_lock_shared(tok);
            break;
          case cmd_t::unlock:
            sh_mtx.unlock_shared(tok);
            break;
          case cmd_t::exit:
            return;
          default:
            failure();
          }

        thr->cmd = unsigned(cmd_t::none);
      }
  }

const unsigned max_shared{10};

thr_data_t thr[max_shared];

void wait_cmd(thr_data_t &t)
  {
    while (t.cmd != unsigned(cmd_t::none))
      std::this_thread::yield();
  }

void test_run()
  {
    {
      std::lock_guard lg{sh_mtx};

      for (auto &t : thr)
        if (t.active)
          {
            t.cmd = unsigned(cmd_t::try_lock);
            wait_cmd(t);
            if (t.try_result)
              failure();
          }
    }
    for (auto &t : thr)
      if (t.active)
        {
          t.cmd = unsigned(cmd_t::lock);
          wait_cmd(t);
          if (sh_mtx.try_lock())
            failure();
          t.cmd = unsigned(cmd_t::unlock);
          wait_cmd(t);
          if (not sh_mtx.try_lock())
            failure();
          sh_mtx.unlock();
        }

    for (auto &t : thr)
      if (t.active)
        {
          t.cmd = unsigned(cmd_t::lock);
          wait_cmd(t);
        }
    if (sh_mtx.try_lock())
      failure();
    for (auto &t : thr)
      if (t.active)
        {
          t.cmd = unsigned(cmd_t::unlock);
          wait_cmd(t);
        }
    if (not sh_mtx.try_lock())
      failure();
    sh_mtx.unlock();
  }

} // end anonymous namespace

int main()
  {
    thr[0].obj = std::thread{thr_func, thr};
    thr[0].active = true;

    if (not sh_mtx.try_lock())
      failure();
    sh_mtx.unlock();

    test_run();

    thr[1].obj = std::thread{thr_func, thr + 1};
    thr[1].active = true;

    test_run();

    for (unsigned i{2}; i < 5; ++i)
      {
        thr[i].obj = std::thread{thr_func, thr + i};
        thr[i].active = true;
      }

    test_run();

    for (unsigned i{5}; i < max_shared; ++i)
      {
        thr[i].obj = std::thread{thr_func, thr + i};
        thr[i].active = true;
      }

    test_run();

    for (unsigned i{0}; i < max_shared; i += 2)
      thr[i].cmd = unsigned(cmd_t::exit);
    for (unsigned i{0}; i < max_shared; i += 2)
      thr[i].obj.join();
    for (unsigned i{0}; i < max_shared; i += 2)
      {
        thr[i].obj = std::thread{};
        thr[i].active = false;
      }

    test_run();

    for (unsigned i{0}; i < max_shared; i += 2)
      {
        thr[i].obj = std::thread{thr_func, thr + i};
        thr[i].active = true;
      }

    test_run();

    for (unsigned i{1}; i < max_shared; ++i)
      thr[i].cmd = unsigned(cmd_t::exit);
    for (unsigned i{1}; i < max_shared; ++i)
      {
        thr[i].obj.join();
        thr[i].active = false;
      }

    test_run();

    thr[0].cmd = unsigned(cmd_t::exit);
    thr[0].obj.join();

    if (not sh_mtx.try_lock())
      failure();
    sh_mtx.unlock();

    std::cout << "Success\n";
    return(0);
  }
