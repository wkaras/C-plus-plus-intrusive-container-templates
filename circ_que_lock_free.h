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
#ifndef ABSTRACT_CONTAINER_CIRC_QUE_LOCK_FREE_H_
#define ABSTRACT_CONTAINER_CIRC_QUE_LOCK_FREE_H_

#include <atomic>

#include "circ_que.h"

namespace abstract_container
{

// If v is only ever written in one thread, it can be safely loaded in
// that thread as a non-atomic.
//
template <typename T>
inline T load_non_atomic(const std::atomic<T> &v)
  {
    // The idea of this is to allow the optimizer to avoid doing a lock
    // or to use the register-cached value from a preceeding load or store.
    // However, it may be that optimization is allowed to do this even when
    // using the "load" member function with relaxed order.  But a strict
    // reading of the Standard indicates that, due to using a pointer of a
    // different type, the compiler may treat it as a different variable,
    // which could cause an error.
    //
    // See:  http://stackoverflow.com/questions/42211688
    //
    // if (sizeof(std::atomic<T>) == sizeof(T))
    //   return(* reinterpret_cast<const T *>(&v));
    // else

    return(v.load(std::memory_order_relaxed));
  }

// Abstractor for instantiation of circ_que for use by two threads, one
// that does (all) pushes and one that does (all) pops.  Neither
// thread is blocked.  index_t is an unsigned integral type large enough
// to hold values up to max_num_elems.  atomic<index_t> must exist, and
// should be lock-free.
//
template <
  typename elem_t_, unsigned max_num_elems_, typename index_t = unsigned>
class circ_que_abs_lock_free
  {
  protected:

    typedef elem_t_ elem_t;

    static const unsigned max_num_elems = max_num_elems_;

    std::atomic<index_t> front, next_in;

    circ_que_abs_lock_free() : front(0), next_in(0) { }

    unsigned produce_front() const
      { return(front.load(std::memory_order_acquire)); }
    unsigned consume_front() const
      { return(load_non_atomic<index_t>(front)); }

    void consume_front(unsigned f)
      { front.store(f, std::memory_order_release); }

    unsigned produce_next_in() const
      { return(load_non_atomic<index_t>(next_in)); }
    unsigned consume_next_in() const
      { return(next_in.load(std::memory_order_acquire)); }

    void produce_next_in(unsigned ni)
      { next_in.store(ni, std::memory_order_release); }

    void end_full_wait() { }
    void end_empty_wait() { }
  };

template <typename elem_t, unsigned max_num_elems, typename index_t = unsigned>
using circ_que_lock_free =
  circ_que<circ_que_abs_lock_free<elem_t, max_num_elems, index_t> >;

} // end namespace abstract_container

#endif /* Include once */
