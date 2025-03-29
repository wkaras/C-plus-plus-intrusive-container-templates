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

#include "ru_shared_mutex.h"

#if not defined(ABSTRACT_CONTAINER_RU_SHARED_MUTEX_UNIT_TEST)

#define L(NAME)

#endif

namespace abstract_container
{

ru_shared_mutex::per_thread_data::~per_thread_data()
  {
    if (not mutex_data)
      // Either this thread never locked this mutex shared, or program shutdown is in progress, and this
      // mutex has already been destroyed.
      //
      return;

    std::unique_lock lk{mutex_data->thread_data_list_mtx};

    // Remove this instance from linked list.

    per_thread_data **ptr{&mutex_data->thread_data_list_head};

    while (*ptr != this)
      ptr = &((*ptr)->link);

    *ptr = (*ptr)->link;

    L(DPTD);
  }

ru_shared_mutex::data::~data()
  {
    // Handle the C++ static destruction fiasco.  I assume static destruction is single threaded.
    //
    for (per_thread_data *ptr{thread_data_list_head}; ptr; ptr = ptr->link)
      ptr->mutex_data = nullptr;
  }

void ru_shared_mutex::impl::lock(ru_shared_mutex::data &d)
  {
    L(LU0);
    std::unique_lock lk{d.uniq_mtx};
    for (;;)
      {
        L(LU1);
        d.uniq = unique_status::want;
        L(LU2);
        if (all_sharing_flags_false(d))
          {
            L(LU3);
            d.uniq = unique_status::yes;
            break;
          }

        // This unlocks uniq_mtx, potentially allowing another thread to get a unique lock first.  Thus,
        // d.uniq could transition to yes, then to no, before uniq_mtx is re-locked.
        //
        d.wait_uniq_cond.wait(lk);
        L(LU4);
      }
    L(LU5);
    lk.release();
  }

bool ru_shared_mutex::impl::try_lock(ru_shared_mutex::data &d)
  {
    if (not d.uniq_mtx.try_lock())
      {
        L(TU0);
        return(false);
      }
    d.uniq = unique_status::want;
    L(TU1);

    if (not all_sharing_flags_false(d, false))
      {
        d.uniq = unique_status::no;
        d.uniq_mtx.unlock();
        L(TU2);
        return(false);
      }
    L(TU3);
    d.uniq = unique_status::yes;
    return(true);
  }

void ru_shared_mutex::impl::unlock(ru_shared_mutex::data &d)
  {
    L(UU0);
    d.uniq = unique_status::no;
    L(UU1);
    d.wait_shared_cond.notify_all();
    d.uniq_mtx.unlock();
    L(UU2);

    notify_uniq_cond(d);
    L(UU3);
  }

bool ru_shared_mutex::impl::add_to_ptd_linked_list(ru_shared_mutex::data &d, ru_shared_mutex::per_thread_data &ptd, bool blocking)
  {
    std::unique_lock lk{d.thread_data_list_mtx, std::defer_lock};

    if (blocking)
      lk.lock();
    else if (not lk.try_lock())
      return(false);

    if (not ptd.mutex_data)
      {
        ptd.link = d.thread_data_list_head;
        d.thread_data_list_head = &ptd;

        ptd.mutex_data = &d;
      }

    return(true);
  }

void ru_shared_mutex::impl::blocking_lock_shared(ru_shared_mutex::per_thread_data &ptd)
  {
    data &d{*ptd.mutex_data};
    L(LS2);
    std::unique_lock lk{d.uniq_mtx};
    L(LS3);
    while (d.uniq != unique_status::no)
      {
        ptd.sharing = false;
        L(LS4);

        if (unique_status::want == d.uniq)
          {
            L(LS5);
            notify_uniq_cond(d);
            L(LS6);
          }

        L(LS7);
        d.wait_shared_cond.wait(lk);
        L(LS8);
        ptd.sharing = true;
        L(LS9);
      }
    L(LS10);
  }

void ru_shared_mutex::impl::blocking_unlock_shared(ru_shared_mutex::data &d)
  {
    L(US3);
    if (not all_sharing_flags_false(d))
      {
        L(US4);
        return;
      }

    for (;;)
      {
        L(US5);
        notify_uniq_cond(d);
        L(US6);

        if ((d.uniq != unique_status::want) or not all_sharing_flags_false(d))
          {
            L(US7);
            return;
          }

        std::this_thread::yield();
      }
  }

void ru_shared_mutex::impl::notify_uniq_cond(ru_shared_mutex::data &d)
  {
    // condition_variable::notify_one() is non-const, so presumably not thread-safe, so protect it.
    //
    bool f{false};
    if (d.notify_uniq_cond.compare_exchange_strong(f, true))
      {
        d.wait_uniq_cond.notify_one();
        d.notify_uniq_cond = false;
      }
  }

bool ru_shared_mutex::impl::all_sharing_flags_false(ru_shared_mutex::data &d, bool blocking)
  {
    std::shared_lock lk{d.thread_data_list_mtx, std::defer_lock};

    if (blocking)
      lk.lock();
    else if (not lk.try_lock())
      return(false);

    per_thread_data *ptr{d.thread_data_list_head};
    while (ptr)
      {
        if (ptr->sharing)
          return(false);
        ptr = ptr->link;
      }
    return(true);
  }

} // end namespace abstract_container
