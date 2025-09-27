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

#if not defined(ABSTRACT_CONTAINER_PLW_SHARED_MUTEX_H_)
#define ABSTRACT_CONTAINER_PLW_SHARED_MUTEX_H_

#include <atomic>
#include <thread>

#if defined(L)
#undef L
#endif

#if defined(ABSTRACT_CONTAINER_PLW_SHARED_MUTEX_UNIT_TEST)

namespace abstract_container
{
void xloc(const char *loc);
}

#define L(NAME) xloc(#NAME)

#else

#include <condition_variable>

#define L(NAME)

#endif

// TEMP add assert() invocations

namespace abstract_container
{

class plw_shared_mutex
  {
    using lock_control_t = unsigned;

    static const lock_control_t
      lc_uniq_wait_msk{lock_control_t{1}},
      lc_uniq_lock_msk{lock_control_t{1} << 1},
      //lc_shared_wait_msk{lock_control_t{1} << 2},
      lc_shared_cnt_lsb_msk{lock_control_t{1} << 2};

    std::atomic<lock_control_t> lock_ctl{0};

#if 0
    struct wait_data_t
      {
        condition_variable cond;
        std::mutex mtx;
        std::atomic<bool> i_am_notifier;
      };
    wait_data_t wait_shared_data;

    struct wait_uniq_data_t : public wait_data_t
      {
        unsigned count{0};
      };
    wait_uniq_data_t wait_uniq_data;
#endif

    void blocking_lock();
    //void notifying_unlock(lock_control_t lc);
    void blocking_lock_shared();

  public:
    plw_shared_mutex() = default;

    bool try_lock()
      {
        auto lc{lock_ctl.load(std::memory_order_relaxed)};
        if (not lc)
          {
            L(TU0);
            return false;
          }
        return lock_ctl.compare_exchange_strong(lc, lc_uniq_lock_msk, std::memory_order_acquire, std::memory_order_relaxed);
      }

    void lock()
      {
        auto lc{lock_ctl.load(std::memory_order_relaxed)};
        L(TU0);
        for (;;)
          {
            if (not (lc bitand compl lc_uniq_lock_msk))
              {
                if (not (lc bitand lc_uniq_wait_msk))
                    lock_ctl.fetch_or(lc_uniq_wait_msk, std::memory_order_relaxed);
                std::this_thread::yield();
                lc = lock_ctl.load(std::memory_order_relaxed);
              }
            else if (lock_ctl.compare_exchange_strong(
                       lc, lc_uniq_lock_msk, std::memory_order_acquire, std::memory_order_relaxed))
              break;
          }
      }

    void unlock()
      {
        L(UU0);
        lock_ctl.fetch_sub(lc_uniq_lock_msk, std::memory_order_release);
      }

    bool try_lock_shared()
      {
        auto lc{lock_ctl.load(std::memory_order_relaxed)};
        L(TS0);
        for (;;)
          {
            if (lc bitand (lc_uniq_lock_msk bitor lc_uniq_wait_msk))
              {
                return false;
              }
            if (lock_ctl.compare_exchange_weak(
                  lc, lc + lc_shared_cnt_lsb_msk, std::memory_order_acquire, std::memory_order_relaxed))
              {
                return true;
              }
          }
      }

    void lock_shared()
      {
        if (not try_lock_shared())
          {
            L(LS0);
            std::this_thread::yield();
          }
      }

    void unlock_shared()
      {
        L(US0);
        lock_ctl.fetch_sub(lc_shared_cnt_lsb_msk, std::memory_order_release);
      }

    plw_shared_mutex(const plw_shared_mutex &) = delete;
    plw_shared_mutex & operator = (const plw_shared_mutex &) = delete;

  }; // end class plw_shared_mutex

} // end namespace abstract_container

#if not defined(ABSTRACT_CONTAINER_PLW_SHARED_MUTEX_UNIT_TEST)
#undef L
#endif

#endif // not defined(ABSTRACT_CONTAINER_PLW_SHARED_MUTEX_H_)
