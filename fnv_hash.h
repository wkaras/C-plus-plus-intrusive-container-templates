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

// Include once.
#ifndef ABSTRACT_CONTAINER_FNV_HASH_H_
#define ABSTRACT_CONTAINER_FNV_HASH_H_

// Utilities.

#include <stdint.h>

namespace abstract_container
{

// FNV-1a hash

const uint32_t fnv_hash_init = 0x811c9dc5;

uint32_t fnv_hash(
  const void *buf, unsigned size, uint32_t hash = fnv_hash_init);

inline uint32_t fnv_hash_next(uint8_t next, uint32_t hash = fnv_hash_init)
  {
    const uint32_t Fnv_prime = (uint32_t(1) << 24) bitor (1 << 8) bitor 0x93;

    hash ^= next;
    hash *= Fnv_prime;

    return(hash);
  }

} // end namespace abstract_container

#endif /* Include once */
