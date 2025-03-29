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

// Unit testing for ru_shared_mutex.  These tests seek to exercise all code paths.

#define ABSTRACT_CONTAINER_RU_SHARED_MUTEX_UNIT_TEST

// Include twice to test guarding against redefiniton.
//
#include "ru_shared_mutex.h"
#undef private
#include "ru_shared_mutex.h"
#undef private

#include <vector>
#include <thread>
#include <cstring>
#include <string>
#include <atomic>
#include <utility>
#include <chrono>

// Defines test_base class.
//
#include "testloop.h"

using namespace std::chrono_literals;

using namespace abstract_container;

namespace
{

ru_shared_mutex::id id;
auto &sh_mtx{ru_shared_mutex::c<id>::inst()};

// "Locations" are invocations of the L() macro withing functions.  For example L(XY) would define a location
// identified by the null-terminated string "XY".

// Data for each test thread (other than the main thread).
//
struct thr_data_t
  {
    std::thread inst;

    // If this is true, after the main thread joins with this test thread, it will test the last location thread execution
    // passed through was DPTD in the per_thread_data_ destructor.
    //
    bool per_thread_data_destroy{false};

    // If this is not "", stall this thread at the location identified by the pointed-to string.
    //
    std::atomic<const char *> block_loc{""};

    // Gives the identifying string for the last location thread execution passed through.
    //
    std::atomic<const char *> last_loc{""};

    // If thread execution passes through any of the locations whose identifiers are in this vector, the test fails.
    //
    std::vector<std::string> prohibited_locs;

    thr_data_t() = default;

    thr_data_t(thr_data_t &&src)
      {
        inst = std::move(src.inst);
        per_thread_data_destroy = src.per_thread_data_destroy;
        block_loc = src.block_loc.load();
        last_loc = src.last_loc.load();
      }
  };

std::vector<thr_data_t> thr;

thread_local std::size_t thr_num;

// Functions whose name start with tf_ are thread functions of test threads.

void tf_lock_shared(std::size_t thr_num_)
  {
    thr_num = thr_num_;

    sh_mtx.lock_shared();
    sh_mtx.unlock_shared();
  }

void tf_try_lock_shared(std::size_t thr_num_)
  {
    thr_num = thr_num_;

    if (sh_mtx.try_lock_shared())
      sh_mtx.unlock_shared();
  }

void tf_lock_unique(std::size_t thr_num_)
  {
    thr_num = thr_num_;

    sh_mtx.lock();
    sh_mtx.unlock();
  }

void tf_try_lock_unique(std::size_t thr_num_)
  {
    thr_num = thr_num_;

    if (sh_mtx.try_lock())
      sh_mtx.unlock();
  }

// The main (controller) thread call this to wait for the test thread with the given number to pass through the given
// location.
//
void check_last_loc(std::size_t thr_num_, const char *loc)
  {
    const unsigned limit_reps{100};
    static unsigned max_reps_seen{2};

    thr_data_t &t{thr[thr_num_]};

    for (unsigned rep{1}; rep < limit_reps; ++rep)
      {
        if (std::strcmp(t.last_loc, loc) == 0)
          {
            if (rep > max_reps_seen)
              {
                std::cout << "max_reps_seen = " << rep << '\n';
                max_reps_seen = rep;
              }
            return;
          }

        // I originally used std::this_thread::yield() here, but it did not seem to cause other threads to run with
        // full reliability.
        //
        std::this_thread::sleep_for(1us);
      }
    std::cout << "thread " << thr_num_ << " failed to reach location " << loc << '\n';
    std::cout << "thread " << thr_num_ << " at " << t.last_loc << '\n';
    failure();
  }

// Clean up all threads at the end of one test case.  Thread data is restored to initial state for the next test case.
//
void thr_cleanup()
  {
    for (unsigned i{0}; i < thr.size(); ++i)
      {
        thr[i].block_loc = "";
        thr[i].inst.join();
        if (thr[i].per_thread_data_destroy)
          check_last_loc(i, "DPTD");
      }
    thr.clear();
  }

#define EC0(A, B) A##B
#define EC1(A, B) EC0(A, B)

// Expand A and B fully and concatenate.
#define EC(A, B) EC1(A, B)

#define TEST_NS EC(test_ns, __LINE__)

#define START_TEST \
namespace TEST_NS { \
struct test_t : public test_base { \
void test() override {

#define END_TEST thr_cleanup(); } }; test_t t; }

void start_thr(
  void (*thread_func)(std::size_t thr_idx), std::size_t thr_idx, const char *block_loc = nullptr, const char *last_loc = nullptr)
  {
    if (block_loc)
      thr[thr_idx].block_loc = block_loc;

    thr[thr_idx].inst = std::thread{thread_func, thr_idx};

    if (not last_loc)
      last_loc = block_loc;

    if (last_loc)
      check_last_loc(thr_idx, last_loc);
  }

// Start a thread with with either 'tf_lock_shared()' or 'tf_try_lock_shared()' as 'thread_func'.
//
void start_thr_s(
  void (*thread_func)(std::size_t thr_idx), std::size_t thr_idx, const char *block_loc = nullptr, const char *last_loc = nullptr)
  {
    thr[thr_idx].per_thread_data_destroy = true;
    start_thr(thread_func, thr_idx, block_loc, last_loc);
  }

void next_loc(std::size_t thr_idx, const char *block_loc, const char *last_loc = nullptr)
  {
    if (std::strcmp(block_loc, thr[thr_idx].last_loc) == 0)
      // Iterate once through a loop to the same location.
      //
      thr[thr_idx].last_loc = "";
    else
      thr[thr_idx].block_loc = block_loc;

    check_last_loc(thr_idx, last_loc ? last_loc : block_loc);
  }

// Begin tests focusing on lock().

START_TEST

enum
  {
    u_idx,
    num_t
  };

thr.resize(num_t);

start_thr(tf_lock_unique, u_idx, "LU2");

thr[u_idx].prohibited_locs.emplace_back("LU4");

next_loc(u_idx, "LU5");

END_TEST

START_TEST

enum
  {
    u_idx,
    u2_idx,
    tu_idx,
    ts_idx,
    num_t
  };

thr.resize(num_t);

start_thr(tf_try_lock_unique, tu_idx, "TU1");

start_thr(tf_lock_unique, u_idx, "LU1", "LU0");

thr[tu_idx].block_loc = "";
check_last_loc(u_idx, "LU1");

start_thr_s(tf_try_lock_shared, ts_idx, "TS0");

next_loc(u_idx, "LU4", "CVW0");

// Cause spurious return from wait() function.
//
sh_mtx.d.wait_uniq_cond.notify_one(u_idx);

check_last_loc(u_idx, "LU4");
next_loc(u_idx, "CVW1", "CVW0");
thr[ts_idx].block_loc = "US6";
check_last_loc(u_idx, "CVW1");

start_thr(tf_lock_unique, u2_idx, "", "UU2");
check_last_loc(ts_idx, "US6");

if (sh_mtx.d.uniq not_eq ru_shared_mutex::unique_status::no)
  {
    std::cout << "d.uniq not equal to no\n";
    failure();
  }

next_loc(u_idx, "LU2");
thr[u_idx].block_loc = "";
check_last_loc(u_idx, "UU2");

END_TEST

START_TEST

enum
  {
    u_idx,
    u2_idx,
    u3_idx,
    ts_idx,
    ts2_idx,
    ts3_idx,
    num_t
  };

thr.resize(num_t);

start_thr(tf_lock_unique, u_idx, "LU1");
start_thr(tf_lock_unique, u2_idx, "LU1", "LU0");
start_thr(tf_lock_unique, u3_idx, "LU1", "LU0");

start_thr_s(tf_try_lock_shared, ts_idx, "TS0");
start_thr_s(tf_try_lock_shared, ts2_idx, "TS0");
start_thr_s(tf_try_lock_shared, ts3_idx, "TS0");

next_loc(u_idx, "LU4", "CVW0");
thr[u2_idx].block_loc = "LU4";
thr[u3_idx].block_loc = "LU4";
check_last_loc(u2_idx, "CVW0");
check_last_loc(u3_idx, "CVW0");

// Cause spurious return from wait() function.
//
sh_mtx.d.wait_uniq_cond.notify_one(u_idx);

check_last_loc(u_idx, "LU4");
next_loc(ts2_idx, "US1");
next_loc(ts3_idx, "US1");

// Cause spurious return from wait() function.
//
sh_mtx.d.wait_uniq_cond.notify_one(u2_idx);

check_last_loc(u2_idx, "CVW1");
next_loc(u_idx, "LU4", "CVW0");
check_last_loc(u2_idx, "LU4");

// Cause spurious return from wait() function.
//
sh_mtx.d.wait_uniq_cond.notify_one(u3_idx);

check_last_loc(u3_idx, "CVW1");
next_loc(u2_idx, "LU4", "CVW0");
check_last_loc(u3_idx, "LU4");
next_loc(u3_idx, "LU4", "CVW0");

// With the unit test version of condition variables, notify_one() with no thread index specified, will cause wait() to return
// for the lowest-index waiting thread.

next_loc(ts_idx, "US5");
check_last_loc(u_idx, "LU4");
next_loc(u_idx, "", "UU2");
check_last_loc(u2_idx, "LU4");
next_loc(u2_idx, "", "UU2");
check_last_loc(u3_idx, "LU4");
next_loc(u3_idx, "", "UU2");

END_TEST

// Begin tests focusing on try_lock().

START_TEST

enum
  {
    tu_idx,
    tu2_idx,
    tu3_idx,
    ts_idx,
    num_t
  };

thr.resize(num_t);

thr[tu2_idx].prohibited_locs.emplace_back("UU0");
start_thr(tf_try_lock_unique, tu2_idx, "TU1");

thr[tu_idx].prohibited_locs.emplace_back("UU0");
start_thr(tf_try_lock_unique, tu_idx, "", "TU0");

start_thr_s(tf_try_lock_shared, ts_idx, "TS0");
next_loc(tu2_idx, "", "TU2");
next_loc(ts_idx, "", "DPTD");

start_thr(tf_try_lock_unique, tu3_idx, "TU3");
next_loc(tu3_idx, "", "UU2");

END_TEST

// No tests focusing on unlock().

// Begin tests focusing on lock_shared().

START_TEST

enum
  {
    u_idx,
    s_idx,
    s2_idx,
    num_t
  };

thr.resize(num_t);

start_thr_s(tf_lock_shared, s_idx, "LS0");
start_thr_s(tf_lock_shared, s2_idx, "LS1");
next_loc(s2_idx, "US2");
thr[s2_idx].block_loc = "";

start_thr(tf_lock_unique, u_idx, "CVW1", "CVW0");

next_loc(s_idx, "CVW1", "CVW0");

check_last_loc(u_idx, "CVW1");
check_last_loc(s_idx, "CVW0");

// Cause spurious return from wait() function.
//
sh_mtx.d.wait_shared_cond.notify_one(s_idx);

check_last_loc(s_idx, "CVW1");
next_loc(s_idx, "US6", "CVW0");

thr[u_idx].block_loc = "";

END_TEST

START_TEST

enum
  {
    u_idx,
    s_idx,
    num_t
  };

thr.resize(num_t);

start_thr(tf_lock_unique, u_idx, "LU3");

thr[s_idx].prohibited_locs.emplace_back("LS3");
start_thr_s(tf_lock_shared, s_idx, "LS4", "LS2");

next_loc(u_idx, "UU1");
check_last_loc(s_idx, "LS4");

END_TEST

// Begin tests focusing on try_lock_shared().

START_TEST

enum
  {
    tu_idx,
    ts_idx,
    ts2_idx,
    ts3_idx,
    num_t
  };

thr.resize(num_t);

thr[ts_idx].prohibited_locs.emplace_back("TS1");
start_thr_s(tf_try_lock_shared, ts_idx, "US0");
thr[ts2_idx].prohibited_locs.emplace_back("TS1");
start_thr_s(tf_try_lock_shared, ts2_idx, "US2");
next_loc(ts_idx, "US2");

start_thr(tf_try_lock_unique, tu_idx, "TU1");

start_thr_s(tf_try_lock_shared, ts3_idx, "US0");
next_loc(tu_idx, "TU2");
thr[ts3_idx].prohibited_locs.emplace_back("US0");
next_loc(ts3_idx, "US2");

END_TEST

// Begin tests focusing on unlock_shared().

START_TEST

enum
  {
    u_idx,
    s_idx,
    s2_idx,
    s3_idx,
    num_t
  };

thr.resize(num_t);

start_thr_s(tf_lock_shared, s_idx, "US0");

start_thr_s(tf_lock_shared, s2_idx, "US0");

start_thr_s(tf_lock_shared, s3_idx, "US2");

start_thr(tf_lock_unique, u_idx, "CVW1", "CVW0");

next_loc(s2_idx, "US3");
thr[s2_idx].block_loc = "";

next_loc(s_idx, "US4");
next_loc(s_idx, "US5");
check_last_loc(u_idx, "CVW1");
next_loc(s_idx, "US5");

next_loc(u_idx, "", "UU2");

// Return from unlock_shared() because unique lock gotten and released (no longer wanted).
//
next_loc(s_idx, "US6");

END_TEST

START_TEST

enum
  {
    u_idx,
    u2_idx,
    ts_idx,
    ts2_idx,
    num_t
  };

thr.resize(num_t);

start_thr_s(tf_try_lock_shared, ts_idx, "US0");

start_thr(tf_lock_unique, u_idx, "LU4", "CVW0");

// Cause spurious return from wait() function.
//
sh_mtx.d.wait_uniq_cond.notify_one(u_idx);

check_last_loc(u_idx, "LU4");
next_loc(u_idx, "", "CVW0");

next_loc(ts_idx, "US5");

check_last_loc(u_idx, "UU2");

start_thr_s(tf_try_lock_shared, ts2_idx, "TS0");

start_thr(tf_lock_unique, u2_idx, "", "CVW0");

// Return from unlock_shared() with unique lock wanted, but thread ts2 will notify it.
//
next_loc(ts_idx, "US6");

thr[ts_idx].block_loc = "";
thr[ts2_idx].block_loc = "";

check_last_loc(u2_idx, "UU2");

END_TEST

} // end anonymous namespace

namespace abstract_container
{

void xloc(const char *loc)
  {
    auto &t{thr[thr_num]};

    t.last_loc = loc;

    for (const auto &ploc : t.prohibited_locs)
      if (ploc == loc)
        {
          std::cout << "prohibited loc " << loc << " reached in thread " << thr_num << '\n';
          failure();
        }

    while (std::strcmp(t.block_loc, t.last_loc) == 0)
      // I originally used std::this_thread::yield() here, but it did not seem to cause other threads to run with
      // full reliability.
      //
      std::this_thread::sleep_for(1us);
  }

void ru_shared_mutex::condition_variable::notify_all()
  {
    if (++notifying_count > 1)
      {
        std::cout << "notify_all() called while already notifying, in thread " << thr_num << '\n';
        failure();
      }

    notified_mask.fetch_or(compl 0);

    --notifying_count;
  }

void ru_shared_mutex::condition_variable::notify_one(int thr_num)
  {
    if (++notifying_count > 1)
      {
        std::cout << "notify_one() called while already notifying, in thread " << thr_num << '\n';
        failure();
      }

    if (thr_num < 0)
      {
        for (unsigned notify_mask{1}; notify_mask <= waiting_mask; notify_mask <<= 1)
          if (notify_mask bitand waiting_mask)
            {
              notified_mask.fetch_or(notify_mask);
              break;
            }
      }
    else
      {
        unsigned notify_mask{unsigned(1) << thr_num};
        if (notify_mask bitand waiting_mask)
          notified_mask.fetch_or(notify_mask);
      }

    --notifying_count;
  }

void ru_shared_mutex::condition_variable::wait(std::unique_lock<std::mutex> &mtx)
  {
    notified_mask.fetch_and(compl (1 << thr_num));

    waiting_mask.fetch_or(1 << thr_num);

    mtx.unlock();

    L(CVW0);

    do
      // I originally used std::this_thread::yield() here, but it did not seem to cause other threads to run with
      // full reliability.
      //
      std::this_thread::sleep_for(1us);
    while (not (notified_mask bitand (1 << thr_num)));

    L(CVW1);

    mtx.lock();

    waiting_mask.fetch_and(compl (1 << thr_num));

    notified_mask.fetch_and(compl (1 << thr_num));
  }

} // end namespace abstract_container

#include "ru_shared_mutex.cpp"
