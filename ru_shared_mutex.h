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

/*

Rarely Unique Shared Mutexes

An ru_shared_mutex may provide better performance than an instance of std::shared_mutex.  The performance advantage is
proportional to the following:
o The ratio of shared locks to unique locks.
o The number of CPU processor cores.
o The threads that take the lock are static versus dynamic.

A big drawback is that each rarely unique shared mutex must be the singleton of a distinct class, specifically, an
instantiation of the ru_shared_mutex::c class template.  Each instantiation has the same interface as std::shared_mutex
for shared and exclusive locking.

Waiting unique locks always take priority over waiting shared locks.  Multiple threads waiting for a unique lock may
not get the lock in the order that they called 'lock()'.

Requires C++17 or later.

*/

#if not defined(ABSTRACT_CONTAINER_RU_SHARED_MUTEX_H_)
#define ABSTRACT_CONTAINER_RU_SHARED_MUTEX_H_

#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <cassert>

#if defined(L)
#warning undefining preprocessor symbol L
#undef L
#endif

// See test_ru_shared_mutex.cpp for a description of code enabled by ABSTRACT_CONTAINER_RU_SHARED_MUTEX_UNIT_TEST.
//
#if defined(ABSTRACT_CONTAINER_RU_SHARED_MUTEX_UNIT_TEST)

namespace abstract_container
{
void xloc(const char *loc);
}

#define L(NAME) xloc(#NAME)

#define private public

#else

#include <condition_variable>

#define L(NAME)

#endif

namespace abstract_container
{

class ru_shared_mutex
  {
  private:

    // Pre-declare type of per-mutex data.
    //
    struct data;

    // Thread local data for a shared mutex.  All instances of this for a particular mutex are kept in a singly-linked
    // list.
    //
    struct per_thread_data_
      {
        per_thread_data_ *link;
        data *mutex_data{nullptr};

        // If this is true, the thread that owns this instance has or is seeking a shared lock of the mutex.
        //
        std::atomic<bool> sharing{false};

        #if not defined(NDEBUG)
        // Value of 0 means unlocked, 1 means unique lock, -1 means shared lock.
        //
        int status_count{0};
        #endif

        per_thread_data_() {} // Need this for clang/LLVM bug work-around.
        ~per_thread_data_();
      };

  public:

    // This class exists to allow control of the alignment of the per-thread data for the mutex, without unnecessary
    // implementation exposure.
    //
    class per_thread_data : public per_thread_data_ {};

  #if defined(ABSTRACT_CONTAINER_RU_SHARED_MUTEX_UNIT_TEST)

  public:

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

  #else

  private:

    using condition_variable = std::condition_variable;

  #endif

    enum class unique_status { no, want, yes};

    // Per-mutex data.
    //
    struct data
      {
        // 'yes' if a thread is holding a unique lock, 'want' if a thread is seeking a unique lock, 'no' otherwise.
        // A thread can only change the value of 'uniq' if it has the 'uniq_mtx' mutex locked.
        //
        std::atomic<unique_status> uniq{unique_status::no};

        // This flag is used to prevent reentrant calls from different threads to 'wait_uniq_cond.notify_one()'.
        //
        std::atomic<bool> notify_uniq_cond{false};

        // Threads lock this before changing 'uniq', or before waiting on either condition variable.  When a thread
        // changes 'uniq' to 'want', the thread cannot release this mutex until after changing 'uniq' back to 'no'.
        //
        std::mutex uniq_mtx;

        // Wait for shared, unique lock, respectively.
        //
        condition_variable wait_shared_cond, wait_uniq_cond;

        // Protect access to the linked list of per-thread data instances.
        //
        std::shared_mutex thread_data_list_mtx;

        // Head pointer of linked list of per-thread data instances.
        //
        per_thread_data_ *thread_data_list_head{nullptr};

        ~data();
      };

    struct impl
      {
        // Pseudo-namespace, no instances.
        //
        impl() = delete;

        // These functions are defined in ru_shared_mutex.cpp.

        static void lock(data &d);
        static bool try_lock(data &d);
        static void unlock(data &d);
        static bool add_to_ptd_linked_list(data &d, per_thread_data_ &ptd, bool blocking = true);
        static void blocking_lock_shared(per_thread_data_ &ptd);
        static void blocking_unlock_shared(data &ptd);
        static void notify_uniq_cond(data &d);
        static bool all_sharing_flags_false(data &d, bool blocking = true);
      };

    #if defined(__cpp_lib_hardware_interference_size)

    static const auto hardware_destructive_interference_size{std::hardware_destructive_interference_size};

    #elif defined(DESTRUCTIVE_INTERFERENCE_SIZE) // Allow for definition on command line.

    static const std::size_t hardware_destructive_interference_size{DESTRUCTIVE_INTERFERENCE_SIZE};

    #else

    // Guess, too big is better than too small.
    //
    static const std::size_t hardware_destructive_interference_size{128};

    #endif

    struct alignas(hardware_destructive_interference_size) no_thrash_ptd : public per_thread_data {};

  public:
    // Pseudo-namespace, no instances.
    //
    ru_shared_mutex() = delete;

    // Create one instance of this class per rarely unique shared mutex.
    //
    class id {};

    template <id &id_>
    static per_thread_data & default_ptd_func()
      {
        thread_local per_thread_data ptd;
        return(ptd);
      }

    template <id &id_>
    static per_thread_data & fast_ptd_func()
      {
        thread_local no_thrash_ptd ptd;
        return(ptd);
      }

    // ptd_func() must return, for each unique pair of c instantiation and thread, a unique instance of per_thread_data.
    //
    template <id &id_, per_thread_data & (*ptd_func)() = default_ptd_func<id_> >
    class c
      {
      public:

        // Returns a reference to the singleton rarely unique shared mutex for the instantiation of c.
        //
        static c & inst()
          {
            static c i;

            return(i);
          }

        void lock()
          {
            assert(ptd().status_count++ == 0);

            impl::lock(d);
          }

        bool try_lock()
          {
            assert(ptd().status_count == 0);

            bool got_it{impl::try_lock(d)};

            assert(got_it ? ++ptd().status_count : 1);

            return(got_it);
          }

        void unlock()
          {
            assert(--ptd().status_count == 0);

            impl::unlock(d);
          }

        void lock_shared()
          {
            assert(ptd().status_count-- == 0);

            if (not ptd().mutex_data)
              static_cast<void>(impl::add_to_ptd_linked_list(d, ptd()));

            ptd().sharing = true;
            L(LS0);
            if (unique_status::no == d.uniq)
              {
                L(LS1);
                return;
              }

            impl::blocking_lock_shared(ptd());
          }

        bool try_lock_shared()
          {
            assert(ptd().status_count == 0);

            if (not ptd().mutex_data)
              if (not impl::add_to_ptd_linked_list(d, ptd(), false))
                return(false);

            assert(--ptd().status_count);

            ptd().sharing = true;
            L(TS0);
            if (unique_status::no == d.uniq)
              return(true);
            L(TS1);
            unlock_shared();
            return(false);
          }

        void unlock_shared()
          {
            assert(++ptd().status_count == 0);

            L(US0);
            ptd().sharing = false;
            L(US1);
            if (d.uniq != unique_status::want)
              {
                L(US2);
                return;
              }

            return(impl::blocking_unlock_shared(d));
          }

        c(const c &) = delete;
        c & operator = (const c &) = delete;

      private:
        c() = default;

        data d;

        // Return reference to per-thread data for this c instantiation, for the thread calling this function.
        //
        static per_thread_data_ & ptd() { return(static_cast<per_thread_data_ &>(ptd_func())); }

      }; // end class template c

  }; // end class ru_shared_mutex

} // end namespace abstract_container

#if not defined(ABSTRACT_CONTAINER_RU_SHARED_MUTEX_UNIT_TEST)
#undef L
#endif

#endif // not defined(ABSTRACT_CONTAINER_RU_SHARED_MUTEX_H_)
