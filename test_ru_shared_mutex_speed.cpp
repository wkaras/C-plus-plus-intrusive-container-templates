#include <iostream>
#include <thread>
#include <atomic>
#include <shared_mutex>
#include <cstdint>
#include <random>
#include <bitset>

#include "ru_shared_mutex.h"

auto const hw_destructive_interference_size =
#ifdef __cpp_lib_hardware_interference_size
std::hardware_destructive_interference_size;
#else
64;
#endif

namespace
{

unsigned const n_threads{64};
unsigned const n_locks_per_cycle{100000};
unsigned const seed{0};

std::bitset<n_locks_per_cycle> use_unique_lock[n_threads];

union counter_t
  {
    std::uint64_t v;
    char space[hw_destructive_interference_size];
  };

alignas(hw_destructive_interference_size) volatile counter_t counter[n_threads];

std::atomic<bool> go, stop;
std::atomic<unsigned> running_thread_count;

template <class mutex_t>
class test_t
  {
  private:
    mutex_t &mtx;

    static void thr_func(unsigned th_idx, mutex_t *mtxp)
      {
        {
          // The first shared lock has overhead for ru_mutex_shared.
          //
          std::shared_lock ul{*mtxp};
        }
        ++running_thread_count;

        while (!go)
          std::this_thread::yield();

        unsigned cycle_idx{0};
        while (!stop)
          {
            if (use_unique_lock[th_idx][cycle_idx])
              {
                std::unique_lock ul{*mtxp};
                counter[th_idx].v = counter[th_idx].v + 1;
              }
            else
              {
                std::shared_lock sl{*mtxp};
                counter[th_idx].v = counter[th_idx].v + 1;
              }

            if (++cycle_idx == n_locks_per_cycle)
              cycle_idx = 0;
          }
      }

  public:
    test_t(mutex_t &mtx_, unsigned n_unique_locks_per_cycle) : mtx{mtx_}
      {
        for (unsigned i{0}; i < n_threads; ++i)
          use_unique_lock[i].reset();

        if (n_unique_locks_per_cycle)
          {
            std::mt19937 gen{seed};
            std::uniform_int_distribution<unsigned> distrib{0, (n_threads * n_locks_per_cycle) - 1};

            for (unsigned i{0}; i < (n_threads * n_unique_locks_per_cycle); ++i)
              {
                unsigned j{distrib(gen)};
                while (use_unique_lock[j / n_locks_per_cycle][j % n_locks_per_cycle])
                  j = distrib(gen);
                use_unique_lock[j / n_locks_per_cycle][j % n_locks_per_cycle] = true;
              }
          }

        using namespace std::chrono_literals;

        // Run twice to make visible any effects from caching/pagination.
        //
        for (unsigned pass{1}; pass <= 2; ++pass)
          {
            std::thread thr[n_threads];

            go = false;
            stop = false;

            for (unsigned idx{0}; idx < n_threads; ++idx)
              counter[idx].v = 0;

            for (unsigned idx{0}; idx < n_threads; ++idx)
              thr[idx] = std::thread(thr_func, idx, &mtx);

            while (running_thread_count < n_threads)
              std::this_thread::yield();

            go = true;

            std::this_thread::sleep_for(3s);

            stop = true;

            for (unsigned idx{0}; idx < n_threads; ++idx)
              {
                thr[idx].join();
              }

            std::uint64_t total{0}, max{0}, min{~unsigned(0)};
            for (unsigned idx{0}; idx < n_threads; ++idx)
              {
                total += counter[idx].v;
                if (counter[idx].v > max)
                  max = counter[idx].v;
                if (counter[idx].v < min)
                  min = counter[idx].v;
              }

            std::cout << "pass=" << pass << ": total=" << total << ", max=" << max << ", min=" << min << '\n';
          }
      }
  };

abstract_container::ru_shared_mutex::id id;
using rusm_t = abstract_container::ru_shared_mutex::c<id>;

std::shared_mutex sh_mtx;

void pair(unsigned n_unique_locks_per_cycle)
  {
    std::cout << "\n\nru_shared_mutex: " << n_unique_locks_per_cycle << " per " << n_locks_per_cycle << '\n';
    test_t{rusm_t::inst(), n_unique_locks_per_cycle};
    std::cout << "\nstd::shared_mutex: " << n_unique_locks_per_cycle << " per " << n_locks_per_cycle << '\n';
    test_t{sh_mtx, n_unique_locks_per_cycle};
  }

} // end anonymous namespace

int main()
  {
    pair(0);
    pair(1);
    pair(10);
    pair(50);
    pair(5000);

    return 0;
  }
