#include <iostream>
#include <thread>
#include <atomic>
#include <shared_mutex>
#include <mutex>
#include <cstdint>
#include <random>
#include <bitset>

#include "ru_shared_mutex.h"

/*
Speed test oriented towards the scenario where two shared mutexes protect two redundant copies of
a data structure, where one copy is coherent and thread-safe readable at all times.
*/

auto const hw_destructive_interference_size =
#ifdef __cpp_lib_hardware_interference_size
std::hardware_destructive_interference_size;
#else
64;
#endif

namespace
{

unsigned const n_threads{64};
unsigned const n_ops_per_cycle{100000};
unsigned const seed{0};

std::bitset<n_ops_per_cycle> write_op[n_threads];

union counter_t
  {
    struct v_s
      {
        std::uint64_t n_writes;
        std::uint64_t n_reads;
      }
    v;
    char space[hw_destructive_interference_size > sizeof(v_s) ? hw_destructive_interference_size : sizeof(v_s)];
  };

alignas(hw_destructive_interference_size) volatile counter_t counter[n_threads];

struct accum_t
  {
    std::uint64_t total{0}, max{0}, min{~std::uint64_t(0)};

    accum_t(std::uint64_t counter_t::v_s::*fp)
      {
        for (unsigned idx{0}; idx < n_threads; ++idx)
          {
            auto v = counter[idx].v.*fp;
            total += v;
            if (v > max)
              max = v;
            if (v < min)
              min = v;
          }
      }
  };

std::mutex wr_mtx;
std::atomic<unsigned> copy_select{0};

std::atomic<bool> go, stop;
std::atomic<unsigned> running_thread_count;

template <class mutex0_t, class mutex1_t>
class test_t
  {
  private:
    mutex0_t &mtx0;
    mutex1_t &mtx1;

    static void thr_func(unsigned th_idx, mutex0_t *mtxp0, mutex1_t *mtxp1)
      {
        {
          // The first shared lock has overhead for ru_mutex_shared.
          //
          std::shared_lock{*mtxp0};
          std::shared_lock{*mtxp1};
        }
        ++running_thread_count;

        while (!go)
          std::this_thread::yield();

        unsigned cycle_idx{0};
        while (!stop)
          {
            if (write_op[th_idx][cycle_idx])
              {
                std::unique_lock wl{wr_mtx};
                unsigned wr_select{!copy_select};
                for (unsigned i{0}; i < 2; ++i)
                  {
                    if (wr_select)
                      {
                        std::unique_lock ul{*mtxp1};
                        counter[th_idx].v.n_writes = counter[th_idx].v.n_writes + 1;
                      }
                    else
                      {
                        std::unique_lock ul{*mtxp0};
                        counter[th_idx].v.n_writes = counter[th_idx].v.n_writes + 1;
                      }
                    copy_select = wr_select;
                    wr_select = !wr_select;
                  }
              }
            else
              {
                if (copy_select)
                  {
                    std::shared_lock sl{*mtxp1};
                    counter[th_idx].v.n_reads = counter[th_idx].v.n_reads + 1;
                  }
                else
                  {
                    std::shared_lock sl{*mtxp0};
                    counter[th_idx].v.n_reads = counter[th_idx].v.n_reads + 1;
                  }
              }

            if (++cycle_idx == n_ops_per_cycle)
              cycle_idx = 0;
          }
      }

  public:
    test_t(mutex0_t &mtx0_, mutex1_t &mtx1_, unsigned n_unique_ops_per_cycle) : mtx0{mtx0_}, mtx1{mtx1_}
      {
        for (unsigned i{0}; i < n_threads; ++i)
          write_op[i].reset();

        if (n_unique_ops_per_cycle)
          {
            std::mt19937 gen{seed};
            std::uniform_int_distribution<unsigned> distrib{0, (n_threads * n_ops_per_cycle) - 1};

            for (unsigned i{0}; i < (n_threads * n_unique_ops_per_cycle); ++i)
              {
                unsigned j{distrib(gen)};
                while (write_op[j / n_ops_per_cycle][j % n_ops_per_cycle])
                  j = distrib(gen);
                write_op[j / n_ops_per_cycle][j % n_ops_per_cycle] = true;
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
              {
                counter[idx].v.n_writes = 0;
                counter[idx].v.n_reads = 0;
              }

            for (unsigned idx{0}; idx < n_threads; ++idx)
              thr[idx] = std::thread{thr_func, idx, &mtx0, &mtx1};

            while (running_thread_count < n_threads)
              std::this_thread::yield();

            go = true;

            std::this_thread::sleep_for(3s);

            stop = true;

            for (unsigned idx{0}; idx < n_threads; ++idx)
              {
                thr[idx].join();
              }

            accum_t rd{&counter_t::v_s::n_reads}, wr{&counter_t::v_s::n_writes};

            wr.total /= 2;
            wr.max /= 2;
            wr.min /= 2;

            std::cout << "pass=" << pass << ": total=" << rd.total << '/' << wr.total << ", max=" <<  rd.max << '/' << wr.max
                      << ", min=" <<  rd.min << '/' << wr.min << '\n';
          }
      }
  };

abstract_container::ru_shared_mutex::id id0;
using rusm0_t = abstract_container::ru_shared_mutex::c<id0>;

abstract_container::ru_shared_mutex::id id1;
using rusm1_t = abstract_container::ru_shared_mutex::c<id1>;

std::shared_mutex sh_mtx[2];

void pair(unsigned n_unique_ops_per_cycle)
  {
    std::cout << "\n\nru_shared_mutex: " << n_unique_ops_per_cycle << " per " << n_ops_per_cycle << '\n';
    test_t{rusm0_t::inst(), rusm1_t::inst(), n_unique_ops_per_cycle};
    std::cout << "\nstd::shared_mutex: " << n_unique_ops_per_cycle << " per " << n_ops_per_cycle << '\n';
    test_t{sh_mtx[0], sh_mtx[1], n_unique_ops_per_cycle};
  }

} // end anonymous namespace

int main()
  {
    std::cout << "counts: reads/writes\n";
    pair(0);
    pair(1);
    pair(5);
    pair(10);
    pair(100);
    pair(50000);

    return 0;
  }
