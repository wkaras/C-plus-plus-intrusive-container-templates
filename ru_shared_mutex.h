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

#if not defined(ABSTRACT_CONTAINER_RU_SHARED_MUTEX_H_)
#define ABSTRACT_CONTAINER_RU_SHARED_MUTEX_H_

#include <atomic>
#include <mutex>
#include <shared_mutex>

#if defined(L)
#undef L
#endif

#if defined(ABSTRACT_CONTAINER_RU_SHARED_MUTEX_UNIT_TEST)

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

class ru_shared_mutex
  {
  #if defined(ABSTRACT_CONTAINER_RU_SHARED_MUTEX_UNIT_TEST)

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

    struct data;

    struct per_thread_data
      {
        per_thread_data *link;
        data *mutex_data{nullptr};
        std::atomic<bool> sharing{false};

        per_thread_data() {} // Need this for clang/LLVM bug work-around.
        ~per_thread_data();
      };

    enum class unique_status { no, want, yes};


    struct data
      {
        std::atomic<unique_status> uniq{unique_status::no};
        std::atomic<bool> notify_uniq_cond{false};
        std::mutex uniq_mtx;
        std::shared_mutex thread_data_list_mtx;
        condition_variable wait_shared_cond, wait_uniq_cond;
        per_thread_data *thread_data_list_head{nullptr};

        ~data();
      };

    struct impl
      {
        // Pseudo-namespace, no instances.
        //
        impl() = delete;

        static void lock(data &d);
        static bool try_lock(data &d);
        static void unlock(data &d);
        static bool add_to_ptd_linked_list(data &d, per_thread_data &ptd, bool blocking = true);
        static void blocking_lock_shared(per_thread_data &ptd);
        static void blocking_unlock_shared(data &ptd);
        static void notify_uniq_cond(data &d);
        static bool all_sharing_flags_false(data &d, bool blocking = true);
      };

  public:
    // Pseudo-namespace, no instances.
    //
    ru_shared_mutex() = delete;

    class id {};

    template <id &id_>
    class c
      {
      public:
        static c & inst()
          {
            static c i;

            return i;
          }

        void lock() { impl::lock(d); }

        bool try_lock() { return impl::try_lock(d); }

        void unlock() { return impl::unlock(d); }

        void lock_shared()
          {
            if (not ptd.mutex_data)
              static_cast<void>(impl::add_to_ptd_linked_list(d, ptd));

            ptd.sharing = true;
            L(LS0);
            if (unique_status::no == d.uniq)
              {
                L(LS1);
                return;
              }

            impl::blocking_lock_shared(ptd);
          }

        bool try_lock_shared()
          {
            if (not ptd.mutex_data)
              if (not impl::add_to_ptd_linked_list(d, ptd, false))
                return false;

            ptd.sharing = true;
            L(TS0);
            if (unique_status::no == d.uniq)
              return true;
            L(TS1);
            unlock_shared();
            return false;
          }

        void unlock_shared()
          {
            L(US0);
            ptd.sharing = false;
            L(US1);
            if (d.uniq != unique_status::want)
              {
                L(US2);
                return;
              }

            return impl::blocking_unlock_shared(d);
          }

        c(const c &) = delete;
        c & operator = (const c &) = delete;

      private:
        c() {}

        data d;

        inline static thread_local per_thread_data ptd;

      }; // end class template c

  }; // end class ru_shared_mutex

} // end namespace abstract_container

#if not defined(ABSTRACT_CONTAINER_RU_SHARED_MUTEX_UNIT_TEST)
#undef L
#endif

#endif // not defined(ABSTRACT_CONTAINER_RU_SHARED_MUTEX_H_)
