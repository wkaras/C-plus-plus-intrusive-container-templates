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

#if not defined(ABSTRACT_CONTAINER_LW_SHARED_MUTEX_H_)
#define ABSTRACT_CONTAINER_LW_SHARED_MUTEX_H_

#include <atomic>
#include <mutex>

#if defined(L)
#undef L
#endif

#if defined(ABSTRACT_CONTAINER_LW_SHARED_MUTEX_UNIT_TEST)

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

class lw_shared_mutex
  {
  #if defined(ABSTRACT_CONTAINER_LW_SHARED_MUTEX_UNIT_TEST)

    class condition_variable
      {
      public:
        condition_variable() = default;
        condition_variable(const condition_variable &) = delete;
        condition_variable & operator = (const condition_variable &) = delete;

        void notify_all();
        void notify_one(int thr_num = -1);
        void wait(std::unique_lock<std::mutex> &mtx);

      private:
        std::atomic<unsigned> notified_mask{0}, waiting_mask{0};
        std::atomic<unsigned> notifying_count{0};
      };

      #define private public

  #else

  private:

    using condition_variable = std::condition_variable;

  #endif

    using lock_control_t = unsigned;

    static const lock_control_t
      lc_uniq_wait_msk{lock_control_t{1}},
      lc_uniq_lock_msk{lock_control_t{1} << 1},
      lc_shared_wait_msk{lock_control_t{1} << 2},
      lc_shared_cnt_lsb_msk{lock_control_t{1} << 3};

    std::atomic<lock_control_t> lock_ctl{0};

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

    void blocking_lock();
    void blocking_lock_shared();
    void notifying_unlock(lock_control_t lc);

  public:
    lw_shared_mutex() = default;

    bool try_lock()
      {
        auto lc{lock_ctl.load(std::memory_order_relaxed)};
        if (lc bitand compl lc_shared_wait_msk)
          {
            L(TU0);
            return false;
          }
        return lock_ctl.compare_exchange_strong(
                 lc, lc bitor lc_uniq_lock_msk, std::memory_order_acquire, std::memory_order_relaxed);
      }

    void lock()
      {
        auto lc{lock_ctl.load(std::memory_order_relaxed)};
        L(TU0);
        if ((lc bitand compl lc_shared_wait_msk) or
            not lock_ctl.compare_exchange_strong(
                  lc, lc bitor lc_uniq_lock_msk, std::memory_order_acquire, std::memory_order_relaxed))
          {
            blocking_lock();
          }
      }

    void unlock()
      {
        L(UU0);
        auto lc{lock_ctl.fetch_sub(lc_uniq_lock_msk, std::memory_order_release) - lc_uniq_lock_msk};
        if (lc bitand (lc_uniq_wait_msk bitor lc_shared_wait_msk))
          {
            notifying_unlock(lc);
          }
      }

    bool try_lock_shared()
      {
        auto lc{lock_ctl.load(std::memory_order_relaxed)};
        L(TS0);
        for (;;)
          {
            if (lc bitand (lc_uniq_wait_msk bitor lc_uniq_lock_msk))
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
            blocking_lock_shared();
          }
      }

    void unlock_shared()
      {
        L(US0);
        auto lc{lock_ctl.fetch_sub(lc_shared_cnt_lsb_msk, std::memory_order_release) - lc_shared_cnt_lsb_msk};
        if (lc bitand (lc_uniq_wait_msk bitor lc_shared_wait_msk))
          {
            notifying_unlock(lc);
          }
      }

    lw_shared_mutex(const lw_shared_mutex &) = delete;
    lw_shared_mutex & operator = (const lw_shared_mutex &) = delete;

  }; // end class lw_shared_mutex

} // end namespace abstract_container

#if not defined(ABSTRACT_CONTAINER_LW_SHARED_MUTEX_UNIT_TEST)
#undef L
#endif

#endif // not defined(ABSTRACT_CONTAINER_LW_SHARED_MUTEX_H_)
