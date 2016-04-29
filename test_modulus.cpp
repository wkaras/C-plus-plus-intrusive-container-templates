/*
Copyright (c) 2016 Walter William Karas

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

// Unit testing for modulus_hash.h .

#include "modulus_hash.h"
#include "modulus_hash.h"

#include <cstdlib>
#include <iostream>

void second_test();

void third_test();

#include <stdint.h>

using std::cout;
using std::exit;
using abstract_container::modulus_hash;
using abstract_container::modulus_hash_fast;

struct Tr
  {
    typedef uint8_t modulus_t;
    typedef uint32_t product_t;

    static const unsigned key_segment_bits = 16;
    static const unsigned num_key_segments = 5;
    static const modulus_t modulus = 19;

    typedef const uint16_t *key;

    template<unsigned Key_segment> static uint16_t get_segment(key k)
      { return(k[Key_segment]); }
  };

const unsigned Num_keys = 1000;

const unsigned Min_segs = 3;

uint16_t data[Tr::num_key_segments];

unsigned hist[Tr::modulus];

Tr::modulus_t hash(Tr::key k)
  {
    Tr::modulus_t h = modulus_hash<Tr>(k);
    if (modulus_hash_fast<Tr>(k) != h)
      {
        cout << "ERROR LINE " << __LINE__ << '\n';
        exit(1);
      }

    return(h);
  }

Tr::modulus_t hash(Tr::key k, unsigned n_seg)
  {
    Tr::modulus_t h = modulus_hash<Tr>(k, n_seg);
    if (modulus_hash_fast<Tr>(k, n_seg) != h)
      {
        cout << "ERROR LINE " << __LINE__ << '\n';
        exit(1);
      }

    return(h);
  }

#define LAST_ONLY 1

int main()
  {
    const uint16_t Gen = 9 * 9 * 9 * 9;
    uint16_t prod = Gen;

    for (unsigned i = 0; i < Num_keys; ++i)
      {
        for (unsigned j = 0; j < Tr::num_key_segments; ++j)
          {
            if ((j == (Tr::num_key_segments - 1)) or !LAST_ONLY)
              data[j] = prod;
            else
              data[j] = 0;
            prod *= Gen;
          }

        ++hist[hash(data)];
      }

    unsigned ttl = 0;
    for (unsigned i = 0; i < Tr::modulus; ++i)
      {
        cout << hist[i] << '\n';
        ttl += hist[i];
      }
    cout << ttl << '\n';

    cout << "===================" << '\n';

    prod = Gen;

    for (unsigned i = 0; i < Tr::modulus; ++i)
      hist[i] = 0;

    unsigned seg_count = Min_segs;

    for (unsigned i = 0; i < Num_keys; ++i)
      {
        for (unsigned j = 0; j < seg_count; ++j)
          {
            if ((j == (seg_count - 1)) or !LAST_ONLY)
              data[j] = prod;
            else
              data[j] = 0;
            prod *= Gen;
          }

        ++hist[hash(data, seg_count)];

        if (seg_count == Tr::num_key_segments)
          seg_count = Min_segs;
        else
          ++seg_count;
      }

    ttl = 0;
    for (unsigned i = 0; i < Tr::modulus; ++i)
      {
        cout << hist[i] << '\n';
        ttl += hist[i];
      }
    cout << ttl << '\n';

    second_test();

    third_test();

    return(0);
  }

struct Tr2
  {
    typedef uint8_t modulus_t;
    typedef uint16_t product_t;

    static const unsigned key_segment_bits = 8;
    static const unsigned num_key_segments = 8;
    static const modulus_t modulus = 13 * 13;

    typedef const uint64_t key;

    template<unsigned Key_segment> static uint8_t get_segment(key k)
      { return(k >> (Key_segment * key_segment_bits)); }
  };

bool big_endian()
 { uint16_t v = 0x102; return(* reinterpret_cast<uint8_t *>(&v) == 1); }

void second_test()
  {
    const uint64_t k =
      big_endian() ?
      ((uint64_t(0x8888777766665555) << 32) + 0x4444333322221111) :
      ((uint64_t(0x1111222233334444) << 32) + 0x5555666677778888);

    const unsigned m1 = unsigned(k % Tr2::modulus);

    // Zero high 8 bits.
    const unsigned m2 = unsigned(((k << 8) >> 8) % Tr2::modulus);

    const unsigned h1 = modulus_hash<Tr2>(k);

    const unsigned h2 = modulus_hash<Tr2>(k, 7);

    if (m1 != h1) cout << "FAIL 1\n";
    if (m2 != h2) cout << "FAIL 2\n";
  }

#define REVERSE 0

struct Tr3
  {
    typedef uint8_t modulus_t;
    typedef uint16_t product_t;

    static const unsigned key_segment_bits = 8;
    static const unsigned num_key_segments = 4;
    static const modulus_t modulus = 19;

    typedef const uint8_t *key;

    template<unsigned Key_segment> static uint8_t get_segment(key k)
      { return(k[big_endian() ? (3 - Key_segment) : Key_segment]); }
  };

const unsigned Per_bin = 100000;

const unsigned Num_keys3 = Tr3::modulus * Per_bin;

unsigned hist3[Tr3::modulus];

Tr3::modulus_t hash3(Tr3::key k)
  {
    Tr3::modulus_t h = modulus_hash<Tr3>(k);
    if (modulus_hash_fast<Tr3>(k) != h)
      {
        cout << "ERROR LINE " << __LINE__ << '\n';
        exit(1);
      }

    return(h);
  }

void third_test()
  {
    for (uint32_t i = 0; i < Num_keys3; ++i)
      ++hist3[hash3(reinterpret_cast<uint8_t *>(&i))];

    for (unsigned i = 0; i < Tr::modulus; ++i)
      if (hist3[i] != Per_bin)
        cout << "3rd TEST ERROR bin " << i << '\n';
  }
