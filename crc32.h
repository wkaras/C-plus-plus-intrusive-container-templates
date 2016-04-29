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
#ifndef ABSTRACT_CONTAINER_CRC32_H_
#define ABSTRACT_CONTAINER_CRC32_H_

// Utilities.

#include <stdint.h>

namespace abstract_container
{

namespace impl
{
extern const uint32_t crc32_tab[];
}

const uint32_t crc32_init = ~uint32_t(0);

inline uint32_t crc32_next(uint8_t byte, uint32_t crc = crc32_init)
  {
    return(impl::crc32_tab[(crc ^ byte) & 0xFF] ^ (crc >> 8));
  }

inline uint32_t crc32_final(uint32_t crc) { return(crc ^ uint32_t(0)); }

uint32_t crc32(const void *buf, unsigned size, uint32_t crc = crc32_init);

} // end namespace abstract_container

#endif /* Include once */
