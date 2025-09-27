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

#include <thread>

#include "lw_shared_mutex.h"

#if not defined(ABSTRACT_CONTAINER_LW_SHARED_MUTEX_UNIT_TEST)

#define L(NAME)

#endif

namespace abstract_container
{

void lw_shared_mutex::blocking_lock()
  {
    const auto not_wait_msk{compl (lc_shared_wait_msk bitor lc_uniq_wait_msk)};
    std::unique_lock lk{wait_uniq_data.mtx};
    ++wait_uniq_data.count;
    auto lc{lock_ctl.load(std::memory_order_relaxed)};
    for (;;)
      {
        if (not (lc bitand not_wait_msk))
          {
            auto goal_lc{
                   (lc bitand (lc_shared_wait_msk bitor (1 == wait_uniq_data.count ? lock_control_t{0} : lc_uniq_wait_msk))) bitor
                   lc_uniq_lock_msk};
            if (lock_ctl.compare_exchange_weak(lc, goal_lc, std::memory_order_relaxed, std::memory_order_relaxed))
              {
                break;
              }
          }
        if (lc bitand not_wait_msk)
          {
            if ((lc bitand lc_uniq_wait_msk) or
                lock_ctl.compare_exchange_weak(
                  lc, lc bitor lc_uniq_wait_msk, std::memory_order_relaxed, std::memory_order_relaxed))
              {
                wait_uniq_data.cond.wait(lk);
                lc = lock_ctl.load(std::memory_order_relaxed);
              }
          }
      }
    --wait_uniq_data.count;
  }

void lw_shared_mutex::blocking_lock_shared()
  {
    const auto lock_or_wait_msk{lc_uniq_lock_msk bitor lc_uniq_wait_msk};
    std::unique_lock lk{wait_shared_data.mtx};
    auto lc{lock_ctl.load(std::memory_order_relaxed)};
    for (;;)
      {
        if (not (lc bitand lock_or_wait_msk))
          {
            if (lock_ctl.compare_exchange_weak(
                  lc, (lc + lc_shared_cnt_lsb_msk) bitand compl lc_shared_wait_msk, std::memory_order_relaxed,
                  std::memory_order_relaxed))
              {
                break;
              }
          }
        if (lc bitand lock_or_wait_msk)
          {
            if ((lc bitand lc_shared_wait_msk) or
                lock_ctl.compare_exchange_weak(
                  lc, lc bitor lc_shared_wait_msk, std::memory_order_relaxed, std::memory_order_relaxed))
              {
                wait_shared_data.cond.wait(lk);
                lc = lock_ctl.load(std::memory_order_relaxed);
              }
          }
      }
  }

void lw_shared_mutex::notifying_unlock(lock_control_t lc)
  {
    const auto wait_msk{lc_shared_wait_msk bitor lc_uniq_wait_msk};
    L(NU0);
    do
      {
        if (lc bitand lc_uniq_wait_msk)
          {
            bool f{false};
            if (wait_uniq_data.i_am_notifier.compare_exchange_strong(
                  f, true, std::memory_order_relaxed, std::memory_order_relaxed))
              {
                wait_uniq_data.cond.notify_one();
                wait_uniq_data.i_am_notifier.store(false, std::memory_order_relaxed);
              }
          }
        else if (lc bitand lc_shared_wait_msk)
          {
            bool f{false};
            if (wait_shared_data.i_am_notifier.compare_exchange_strong(
                  f, true, std::memory_order_relaxed, std::memory_order_relaxed))
              {
                wait_shared_data.cond.notify_all();
                wait_shared_data.i_am_notifier.store(false, std::memory_order_relaxed);
              }
          }

        lc = lock_ctl.load(std::memory_order_relaxed);
        if (not ((lc bitand wait_msk) and not (lc bitand lc_uniq_lock_msk)))
          {
            break;
          }
        std::this_thread::yield();
        lc = lock_ctl.load(std::memory_order_relaxed);
      }
    while ((lc bitand wait_msk) and not (lc bitand lc_uniq_lock_msk)); 
  }

} // end namespace abstract_container
