/*
Copyright (c) 2025 Walter William Karas

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

// Unit testing for ru_shared_mutex.

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
#include <atomic>
#include <utility>

#include "testloop.h"

using abstract_container::ru_shared_mutex;

namespace
{

ru_shared_mutex::id id;
auto &sh_mtx{ru_shared_mutex::c<id>::inst()};

struct thr_data_t
  {
    std::thread obj;
    bool per_thread_data_destroy{false};
    std::atomic<const char *> block_loc{""};
    std::atomic<const char *> last_loc{""};

    thr_data_t() = default;

    thr_data_t(thr_data_t &&src)
      {
        obj = std::move(src.obj);
        per_thread_data_destroy = src.per_thread_data_destroy;
        block_loc = src.block_loc.load();
        last_loc = src.last_loc.load();
      }
  };

std::vector<thr_data_t> thr;

thread_local std::size_t thr_num;

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

void check_last_loc(std::size_t thr_num_, const char *loc)
  {
    const unsigned limit_reps{1000};
    static unsigned max_reps_seen{2};

    thr_data_t &t{thr[thr_num_]};

    for (unsigned rep{1}; rep < limit_reps; ++rep)
      {
        if (strcmp(t.last_loc, loc) == 0)
          {
            if (rep > max_reps_seen)
              {
                std::cout << "max_reps_seen = " << rep << '\n';
                max_reps_seen = rep;
              }
            return;
          }
        std::this_thread::yield();
      }
    std::cout << "thread " << thr_num_ << " failed to reach location " << loc << '\n';
    std::cout << "thread " << thr_num_ << " at " << t.last_loc << '\n';
    failure();
  }

void thr_cleanup()
  {
    for (unsigned i{0}; i < thr.size(); ++i)
      {
        thr[i].obj.join();
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

    thr[thr_idx].obj = std::thread{thread_func, thr_idx};

    if (not last_loc)
      last_loc = block_loc;

    if (last_loc)
      check_last_loc(thr_idx, last_loc);
  }

void start_thr_s(
  void (*thread_func)(std::size_t thr_idx), std::size_t thr_idx, const char *block_loc = nullptr, const char *last_loc = nullptr)
  {
    thr[thr_idx].per_thread_data_destroy = true;
    start_thr(thread_func, thr_idx, block_loc, last_loc);
  }

void next_loc(std::size_t thr_idx, const char *block_loc, const char *last_loc = nullptr)
  {
    thr[thr_idx].block_loc = block_loc;

    check_last_loc(thr_idx, last_loc ? last_loc : block_loc);
  }

START_TEST

END_TEST

START_TEST

enum
  {
    u_idx,
    s_idx,
    s2_idx,
    num_t
  };

thr.resize(num_t);

start_thr_s(tf_lock_shared, s_idx, "US0");

start_thr_s(tf_lock_shared, s2_idx, "US0");

start_thr(tf_lock_unique, u_idx, "LU4", "CVW0");

// Cause spurious return from wait() function.
//
sh_mtx.d.wait_uniq_cond.notify_one(u_idx);

check_last_loc(u_idx, "LU4");
next_loc(u_idx, "CVW1", "CVW0");

next_loc(s2_idx, "US4");
thr[s2_idx].block_loc = "";

next_loc(s_idx, "US5");
next_loc(s_idx, "US6");
check_last_loc(u_idx, "CVW1");
next_loc(s_idx, "US5");
next_loc(s_idx, "US6");

next_loc(u_idx, "", "UU3");

// Return from unlock_shared() because unique lock gotten and released (no longer wanted).
//
next_loc(s_idx, "US7");

thr[s_idx].block_loc = "";

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

next_loc(ts_idx, "US6");

check_last_loc(u_idx, "UU3");

start_thr_s(tf_try_lock_shared, ts2_idx, "TS0");

start_thr(tf_lock_unique, u2_idx, "", "CVW0");

// Return from unlock_shared() with unique lock wanted, but thread ts2 will notify it.
//
next_loc(ts_idx, "US7");

thr[ts_idx].block_loc = "";
thr[ts2_idx].block_loc = "";

check_last_loc(u2_idx, "UU3");

END_TEST

START_TEST

// Test try_lock() unique functionality - this was previously unused
// This test exercises the try_lock() code path (TU0-TU3 labels) which
// was not covered by any existing tests
enum
  {
    tu_idx,
    num_t
  };

thr.resize(num_t);

// Simple test: just call try_lock_unique in a thread to exercise the code path
start_thr(tf_try_lock_unique, tu_idx);

END_TEST

START_TEST

// Additional try_lock_shared test for more coverage
// This provides additional exercise of the TS0 path and non-blocking behavior
// Test that try_lock_shared succeeds when no unique lock is held
start_thr(tf_try_lock_shared, 0);

END_TEST

START_TEST

// Test try_lock_shared with per-thread data destruction
// This ensures the per-thread data cleanup path (DPTD) is exercised
// with try_lock_shared operations
enum
  {
    ts_idx,
    num_t
  };

thr.resize(num_t);

start_thr_s(tf_try_lock_shared, ts_idx, "TS0");
thr[ts_idx].block_loc = "";

END_TEST

START_TEST

// Additional try_lock_unique test for robustness
// Provides more coverage of the try_lock() path with different timing
enum
  {
    tu_idx,
    num_t
  };

thr.resize(num_t);

// Simple try_lock_unique test
start_thr(tf_try_lock_unique, tu_idx);

END_TEST

} // end anonymous namespace

namespace abstract_container
{

void xloc(const char *loc)
  {
    auto &t{thr[thr_num]};

    t.last_loc = loc;

    while (std::strcmp(t.block_loc, loc) == 0)
      std::this_thread::yield();
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

    for (unsigned notify_mask{1}; notify_mask <= waiting_mask; notify_mask <<= 1)
      if (notify_mask bitand waiting_mask)
        {
          notified_mask.fetch_or(notify_mask);
          break;
        }

    --notifying_count;
  }

void ru_shared_mutex::condition_variable::wait(std::unique_lock<std::mutex> &mtx)
  {
    mtx.unlock();

    notified_mask.fetch_and(compl (1 << thr_num));

    waiting_mask.fetch_or(1 << thr_num);

    L(CVW0);

    do
      std::this_thread::yield();
    while (not (notified_mask bitand (1 << thr_num)));

    waiting_mask.fetch_and(compl (1 << thr_num));

    notified_mask.fetch_and(compl (1 << thr_num));

    L(CVW1);

    mtx.lock();
  }

} // end namespace abstract_container

#include "ru_shared_mutex.cpp"
