/*
Copyright (c) 2016, 2025 Walter William Karas

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

// Circular queue containers, usable in single-thread and multithread
// code.

// Include once.
#ifndef ABSTRACT_CONTAINER_CIRC_QUE_H_
#define ABSTRACT_CONTAINER_CIRC_QUE_H_

#include <new>
#include <utility>

#if (__cplusplus < 201100) && !defined(max_align_t)

#define max_align_t int

#endif

namespace abstract_container
{

#ifndef ABSTRACT_CONTAINER_RAW_DEFINED_
#define ABSTRACT_CONTAINER_RAW_DEFINED_

class raw_t { };
const raw_t raw;

#endif

template <class cq_t>
class circ_que_front;

template <class cq_t>
class circ_que_back;

/*
Circular queue container class template

The 'abstractor' template parameter class must have the following public
or protected members, or behave as though it does:

Types:
elem_t -- type of the elements of the circular queue.

Constants:
unsigned max_num_elems -- Maximum number of elements that can be queued at the
  same time.

Member Functions:
void consume_front(unsigned) -- change the index (initialy zero) of the front
  element in the queue's underlying array.  This is only called from the
  popping/consuming code.
void produce_next_in(unsigned) -- change the index (initialy zero) of the next
  element to push in the queue's underlying array.  This is only called from
  the pushing/producing code.
unsigned consume_front() -- returns the index of the front element in the
  queue's underlying array.  This is only called from the popping/consuming
  code.
unsigned consume_next_in() -- returns the index of the next element in the
  queue's underlying array to push.  This is only called from the
  popping/consuming code.
unsigned produce_front() -- returns the index of the front element in the
  queue's underlying array.  This is only called from the pushing/producing
  code.
unsigned produce_next_in() -- returns the index of the next element in the
  queue's underlying array to push.  This is only called from the
  pushing/producing code.
void end_full_wait() -- in blocking, mutithreading specializations, will cause
  wait_while_full() call in the pushing thread to exit.  Should only be called
  in popping thread.  Should return immediately in non-blocking
  specializations.
void end_empty_wait() -- in blocking, mutithreading specializations, will cause
  wait_while_empty() call in the popping thread to exit.  Should only be called
  in pushing thread.  Should return immediately in non-blocking
  specializations.
void wait_while_empty() -- block in popping thread, waiting for
  end_empty_wait() to be called in the pushing thread.  Can be omitted in
  non-blocking specializations.
void wait_while_full() -- block in pushing thread, waiting for
  end_full_wait() to be called in the popping thread.  Can be omitted in
  non-blocking specializations.
*/
template <class abstractor>
class circ_que : public abstractor
  {
  public:

    static const unsigned max_size = abstractor::max_num_elems;

    typedef typename abstractor::elem_t elem_t;

  protected:

    static const unsigned dimension = max_size + 1;

    typedef circ_que cq_t;

    #if __cplusplus >= 201100

    alignas(elem_t) char data[dimension][sizeof(elem_t)];

    #else

    max_align_t
      data[dimension][(sizeof(elem_t) + sizeof(max_align_t) - 1) /
                      sizeof(max_align_t)];

    #endif

    elem_t & operator [] (unsigned idx)
      { return(reinterpret_cast<elem_t &>(data[idx])); }

    // Returns number of elements currently in queue, given index of front
    // element, and index of next element to push.
    static unsigned size(unsigned f, unsigned ni)
      { return(ni >= f ? (ni - f) : (ni + dimension - f)); }

    friend class circ_que_front<cq_t>;
    friend class circ_que_back<cq_t>;

  #if __cplusplus >= 201100
  public:

    template<typename ... args_t>
    circ_que(args_t && ... args)
      : abstractor(std::forward<args_t>(args)...) { }
  #endif

  }; // end circ_que

// Access queue from popping/consuming code.  The parameter is presumed
// to be a specialization of circ_que.
//
template <class cq_t>
class circ_que_front
  {
  public:

    typedef typename cq_t::elem_t elem_t;

    circ_que_front(cq_t &cq_) : cq(cq_) { }

    // Returns number of elements currently in queue.
    //
    unsigned size() const
      {
        unsigned ni = cq.consume_next_in();
        unsigned f = cq.consume_front();
        return(cq_t::size(f, ni));
      }

    bool is_empty() const
      {
        return(
          cq.consume_next_in() == cq.consume_front());
      }

    void wait_while_empty()
      {
        while (is_empty())
          cq.wait_while_empty();
      }

    // Returns element currently at front of queue.
    //
    const elem_t & operator () () const
      { return(cq[cq.consume_front()]); }

    // Look ahead from front to successive elements in queue.  index
    // must be less than size().
    //
    const elem_t & operator () (unsigned index) const
      { return(cq[(cq.consume_front() + index) % cq_t::dimension]); }

    // Warning:  doing a pop when the queue is empty will corrupt its state.

    // Pop (discard) front element, without calling ~elem_t().
    //
    void pop(raw_t) { pop_(cq.consume_front()); }

    // Pop (discard) front element.
    //
    void pop()
      {
        unsigned f = cq.consume_front();

        cq[f].~elem_t();

        pop_(f);
      }

  private:

    void pop_(unsigned f)
      {
        ++f;

        cq.consume_front(f == cq_t::dimension ? 0 : f);

        cq.end_full_wait();
      }

    cq_t &cq;

  }; // end circ_que_front

// Access queue from pushing/producing code.  The parameter is presumed
// to be a specialization of circ_que.
// 
template <class cq_t>
class circ_que_back
  {
  public:

    typedef typename cq_t::elem_t elem_t;

    circ_que_back(cq_t &cq_) : cq(cq_) { }

    // Returns number of elements currently in queue.
    //
    unsigned size() const
      {
        unsigned ni = cq.produce_next_in();
        unsigned f = cq.produce_front();
        return(cq_t::size(f, ni));
      }

    bool is_full() const { return(size() == cq_t::max_size); }

    void wait_while_full()
      {
        while (is_full())
          cq.wait_while_full();
      }

    // Return reference to next element to push (which has not been
    // initialized).  Warning:  do not call this when the queue is
    // full.
    //
    elem_t & operator () () { return(cq[cq.produce_next_in()]); }

    // Construct the next element to push.  Warning:  do not call this
    // when the queue is full.
    //
    #if __cplusplus >= 201100
    template<typename ... args_t>
    void init(args_t && ... args)
      {
        unsigned ni = cq.produce_next_in();

        new(&cq[ni]) elem_t(std::forward<args_t>(args)...);
      }
    #else
    void init(const elem_t &e)
      {
        unsigned ni = cq.produce_next_in();

        new(&cq[ni]) elem_t(e);
      }
    #endif

    // Warning:  doing a push when the queue is full will corrupt its state.

    // Push next element, without initializing it.
    //
    void push(raw_t) { push_(cq.produce_next_in()); }

    // Construct the next element to push and then push it.
    //
    #if __cplusplus >= 201100
    template<typename ... args_t>
    void push(args_t && ... args)
      {
        unsigned ni = cq.produce_next_in();

        new(&cq[ni]) elem_t(std::forward<args_t>(args)...);

        push_(ni);
      }
    #else
    void push(const elem_t &e)
      {
        unsigned ni = cq.produce_next_in();

        new(&cq[ni]) elem_t(e);

        push_(ni);
      }
    #endif

  private:

    void push_(unsigned ni)
      {
        ++ni;

        cq.produce_next_in(ni == cq_t::dimension ? 0 : ni);

        cq.end_empty_wait();
      }

    cq_t &cq;

  }; // end circ_que_back

// Abstractor for single-thread instantiation of circ_que. index_t
// is an unsigned integral type large enough to hold values up to
// max_num_elems;
//
template <
  typename elem_t_, unsigned max_num_elems_, typename index_t = unsigned>
class circ_que_abs_basic
  {
  protected:

    typedef elem_t_ elem_t;

    static const unsigned max_num_elems = max_num_elems_;

    index_t front, next_in;

    circ_que_abs_basic() : front(0), next_in(0) { }

    unsigned produce_front() const { return(front); }
    unsigned consume_front() const { return(front); }

    void consume_front(unsigned f) { front = f; }

    unsigned produce_next_in() const { return(next_in); }
    unsigned consume_next_in() const { return(next_in); }

    void produce_next_in(unsigned ni) { next_in = ni; }

    void end_full_wait() { }
    void end_empty_wait() { }
  };

// Specialization for single-thread use.
//
template <typename elem_t, unsigned max_num_elems, typename index_t = unsigned>
class basic_circ_que
  : public circ_que<circ_que_abs_basic<elem_t, max_num_elems, index_t> >
  {
  public:

    void purge()
      {
        this->front = 0;
        this->next_in = 0;
      }

    friend class
      circ_que_front<basic_circ_que<elem_t, max_num_elems, index_t> >;
    friend class
      circ_que_back<basic_circ_que<elem_t, max_num_elems, index_t> >;
  };

// Note:  This is not an intrusive, non-copying container.  I just
// put it here rather than putting it in it's own small repo.

} // end namespace abstract_container

#endif /* Include once */
