/*
Copyright (c) 2016 Walter William Karas
Copyright (c) 2019 K. Miller

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
Test the speed and collision rate of various hash functions.

The timing methods have limited repeatability and portability.
*/

#include <iostream>
#include <exception>
#include <cmath>
#include <cstdlib>

#include "fnv_hash.h"
#include "crc32.h"
#include "modulus_hash.h"

#include <stdint.h>

using std::cout;

void bail(const char *msg)
  {
    cout << msg << '\n';
    std::terminate();
  }

#define USE_GNU_TIMES 1

#if USE_GNU_TIMES

#include <sys/times.h>

// tm is end time on intput, time different from start to end on output.
inline void since(const clock_t start, clock_t &tm) { tm -= start; }

void out_tm(clock_t tm) { cout << tm << " tics"; }

class Exe_tm
  {
  public:

    Exe_tm()
      {
        struct tms usage;

        if (times(&usage) == clock_t(-1))
          bail("FAIL:  getrusage() call failed");

        user_tm = usage.tms_utime;
        sys_tm = usage.tms_stime;
      }

    void done()
      {
        struct tms usage;

        if (times(&usage) == clock_t(-1))
          bail("FAIL:  getrusage() call failed");

        since(user_tm, usage.tms_utime);
        user_tm = usage.tms_utime;

        since(sys_tm, usage.tms_stime);
        sys_tm = usage.tms_stime;
      }

    void dump()
      {
        cout << "User time:  ";
        out_tm(user_tm);

        cout << "\nSystem time:  ";
        out_tm(sys_tm);
        cout << '\n';
      }

  private:
    clock_t user_tm, sys_tm;
  };

#else

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

typedef struct timeval Tm_val;

// tm is end time on intput, time different from start to end on output.
void since(const Tm_val start, Tm_val &tm)
  {
    if (tm.tv_sec == start.tv_sec)
      {
        tm.tv_sec = 0;
        tm.tv_usec -= start.tv_usec;
      }
    else
      {
        tm.tv_usec += 1000000 - start.tv_usec;
        tm.tv_sec -= start.tv_sec + 1;
        if (tm.tv_usec >= 1000000)
          {
            tm.tv_usec -= 1000000;
            ++tm.tv_sec;
          }
      }
  }

void out_tm(const Tm_val &tm)
  {
    cout << "seconds: " << tm.tv_sec;
    cout << "  useconds: " << tm.tv_usec;
  }

class Exe_tm
  {
  public:

    Exe_tm()
      {
        struct rusage usage;

        if (getrusage(RUSAGE_SELF, &usage))
          bail("FAIL:  getrusage() call failed");

        user_tm = usage.ru_utime;
        sys_tm = usage.ru_stime;
      }

    void done()
      {
        struct rusage usage;

        if (getrusage(RUSAGE_SELF, &usage))
          bail("FAIL:  getrusage() call failed");

        since(user_tm, usage.ru_utime);
        user_tm = usage.ru_utime;

        since(sys_tm, usage.ru_stime);
        sys_tm = usage.ru_stime;
      }

    void dump()
      {
        cout << "User time:  ";
        out_tm(user_tm);

        cout << "\nSystem time:  ";
        out_tm(sys_tm);
        cout << '\n';
      }

  private:
    Tm_val user_tm, sys_tm;
  };

#endif

template <class Hash>
void test_hash(unsigned num_keys, uint8_t seed)
  {
    uint8_t key[Hash::Num_key_bytes];

    unsigned hist[Hash::Num_values];

    for (unsigned i = 0; i < Hash::Num_values; ++i)
      hist[i] = 0;

    Hash h;

    std::srand(seed);

    Exe_tm exe_tm;

    for (unsigned i = 0; i < num_keys; ++i)
      {
        for (unsigned j = 0; j < Hash::Num_key_bytes; ++j)
          key[j] = (uint64_t(std::rand()) * 256) / RAND_MAX;

        ++hist[h(key)];
      }

    exe_tm.done();

    double average = double(num_keys) / Hash::Num_values;

    double accum = 0.0;

    for (unsigned i = 0; i < Hash::Num_values; ++i)
      {
        double diff = hist[i] - average;
        accum += diff * diff;
      }

    cout << '\n' << h.name() << '\n';
    exe_tm.dump();
    cout << "Bin size standard deviation / average:  ";
    cout << (sqrt(accum / Hash::Num_values) / average) << '\n';
  }

template <unsigned Num_key_bytes_>
struct Dummy
  {
    static const unsigned Num_key_bytes = Num_key_bytes_;

    static const unsigned Num_values = 1;

    static const char * name()
      { return("Dummy (no actual hashing, shows test overhead)"); }

    unsigned operator () (const uint8_t *)
      {
        return(0);
      }
  };

template <unsigned Num_key_bytes_>
struct Crc
  {
    static const unsigned Num_key_bytes = Num_key_bytes_;

    static const unsigned Num_values = 1 << 10;

    static const char * name() { return("32-bit CRC"); }

    unsigned operator () (const uint8_t *key)
      {
        return(
          abstract_container::crc32(key, Num_key_bytes) & (Num_values - 1));
      }
  };

template <unsigned Num_key_bytes_>
struct Inline_crc
  {
    static const unsigned Num_key_bytes = Num_key_bytes_;

    static const unsigned Num_values = 1 << 10;

    static const char * name() { return("Inline 32-bit CRC"); }

    unsigned operator () (const uint8_t *key)
      {
        const uint8_t *p = static_cast<const uint8_t *>(key);
        const uint8_t *end = p + Num_key_bytes;

        uint32_t hash = abstract_container::crc32_init;

        for ( ; p < end; ++p)
          hash = abstract_container::crc32_next(*p, hash);

        return(abstract_container::crc32_final(hash) & (Num_values - 1));
      }
  };

template <unsigned Num_key_bytes_>
struct Fnv
  {
    static const unsigned Num_key_bytes = Num_key_bytes_;

    static const unsigned Num_values = 1 << 10;

    static const char * name() { return("FNV"); }

    unsigned operator () (const uint8_t *key)
      {
        return(
          abstract_container::fnv_hash(key, Num_key_bytes) & (Num_values - 1));
      }
  };

template <unsigned Num_key_bytes_>
struct Inline_fnv
  {
    static const unsigned Num_key_bytes = Num_key_bytes_;

    static const unsigned Num_values = 1 << 10;

    static const char * name() { return("Inline FNV"); }

    unsigned operator () (const uint8_t *key)
      {
        const uint8_t *p = static_cast<const uint8_t *>(key);
        const uint8_t *end = p + Num_key_bytes;

        uint32_t hash = abstract_container::fnv_hash_init;

        for ( ; p < end; ++p)
          hash = abstract_container::fnv_hash_next(*p, hash);

        return(hash & (Num_values - 1));
      }
  };

template <unsigned Num_key_bytes_>
struct Mod_traits
  {
    typedef const uint8_t *key;

    typedef uint32_t modulus_t;
    typedef uint64_t product_t;

    static const unsigned key_segment_bits = 32;
    static const unsigned modulus = 31 * 31;
    static const unsigned num_key_segments =
      ((8 * Num_key_bytes_) + key_segment_bits - 1) / key_segment_bits;

    template <unsigned key_segment>
    static uint32_t get_segment(key k)
      {
        k += (4 * key_segment);
        uint32_t res = *k;

        if (((4 * key_segment) + 1) == Num_key_bytes_)
          return(res);

        res = (res << 8) | *(k + 1);

        if (((4 * key_segment) + 2) == Num_key_bytes_)
          return(res);

        res = (res << 8) | *(k + 2);

        if (((4 * key_segment) + 3) == Num_key_bytes_)
          return(res);

        return((res << 8) | *(k + 3));
      }
  };

template <unsigned Num_key_bytes_>
struct Modulus
  {
    typedef Mod_traits<Num_key_bytes_> Traits;

    static const unsigned Num_key_bytes = Num_key_bytes_;

    static const unsigned Num_values = Traits::modulus;

    static const char * name() { return("Modulus Hash"); }

    unsigned operator () (const uint8_t *key)
      {
        return(abstract_container::modulus_hash<Traits>(key));
      }
  };

template <unsigned Num_key_bytes_>
struct Fast_modulus
  {
    typedef Mod_traits<Num_key_bytes_> Traits;

    static const unsigned Num_key_bytes = Num_key_bytes_;

    static const unsigned Num_values = Traits::modulus;

    static const char * name() { return("Fast Modulus Hash"); }

    unsigned operator () (const uint8_t *key)
      {
        return(abstract_container::modulus_hash_fast<Traits>(key));
      }
  };

const unsigned Num_key_bytes = 37;

int main(int n_arg, char **arg)
  {
    int num_keys = 0, seed = 0;

    if (n_arg > 2)
      num_keys = std::atoi(arg[2]);

    if (n_arg > 1)
      seed = std::atoi(arg[1]);

    if (num_keys <= 0)
      num_keys = 10000000;

    if (seed <= 0)
      seed = 0;

    cout << "\nSeed:  " << unsigned(seed) << '\n';
    cout << "Number of keys:  " << num_keys << '\n';

    test_hash<Dummy<Num_key_bytes> >(unsigned(num_keys), uint8_t(seed));

    test_hash<Crc<Num_key_bytes> >(unsigned(num_keys), uint8_t(seed));

    test_hash<Inline_crc<Num_key_bytes> >(unsigned(num_keys), uint8_t(seed));

    test_hash<Fnv<Num_key_bytes> >(unsigned(num_keys), uint8_t(seed));

    test_hash<Inline_fnv<Num_key_bytes> >(unsigned(num_keys), uint8_t(seed));

    test_hash<Modulus<Num_key_bytes> >(unsigned(num_keys), uint8_t(seed));

    test_hash<Fast_modulus<Num_key_bytes> >(unsigned(num_keys), uint8_t(seed));

    cout << '\n';

    return(0);
  }
