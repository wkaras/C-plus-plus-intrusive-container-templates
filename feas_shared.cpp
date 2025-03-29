#include <iostream>
#include <thread>
#include <atomic>
#include <shared_mutex>
#include <cstdint>

#ifndef __cpp_lib_hardware_interference_size
#error
#endif

namespace
{

unsigned const N_threads{16};

union Counter_t
  {
    std::uint64_t v;
    char space[std::hardware_destructive_interference_size];
  };

alignas(std::hardware_destructive_interference_size) volatile Counter_t read_count[N_threads];

std::thread thr[N_threads];

std::atomic<bool> go, stop;

template <class Mutex_t>
class Test_t
  {
  private:
    inline static Mutex_t mtx;

    static void read_thr_func(unsigned th_idx)
      {
        while (!go.load(std::memory_order_seq_cst))
          ;

        while (!stop.load(std::memory_order_seq_cst))
          {
            std::shared_lock sl{mtx};
            read_count[th_idx].v = read_count[th_idx].v + 1;
          }
      }

  public:
    static void x()
      {
        using namespace std::chrono_literals;

        go.store(false, std::memory_order_seq_cst);
        stop.store(false, std::memory_order_seq_cst);

        for (unsigned idx{0}; idx < N_threads; ++idx)
          {
            thr[idx] = std::thread(read_thr_func, idx);
          }

        go.store(true, std::memory_order_seq_cst);

        std::this_thread::sleep_for(5s);

        stop.store(true, std::memory_order_seq_cst);

        for (unsigned idx{0}; idx < N_threads; ++idx)
          {
            thr[idx].join();
          }

        unsigned total{0}, max{0}, min{~unsigned(0)};
        for (unsigned idx{0}; idx < N_threads; ++idx)
          {
            total += read_count[idx].v;
            if (read_count[idx].v > max)
              max = read_count[idx].v;
            if (read_count[idx].v < min)
              min = read_count[idx].v;
          }

        std::cout << "total=" << total << ", max=" << max << ", min=" << min << '\n';
      }
  };

class Dummy_mtx_t
  {
  private:
    volatile std::atomic<unsigned> one;
    static thread_local volatile std::atomic<unsigned> many;

  public:
    void lock_shared()
      {
        many.store(true, std::memory_order_seq_cst);
        static_cast<void>(one.load(std::memory_order_seq_cst));
      }

    void unlock_shared()
      {
        many.store(true, std::memory_order_seq_cst);
        static_cast<void>(one.load(std::memory_order_seq_cst));
      }
  };

thread_local volatile std::atomic<unsigned> Dummy_mtx_t::many;

} // end anonymous namespace

int main()
  {
    Test_t<std::shared_mutex>::x();
    Test_t<std::shared_mutex>::x();

    Test_t<Dummy_mtx_t>::x();
    Test_t<Dummy_mtx_t>::x();

    return 0;
  }
