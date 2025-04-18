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

// Include once.
#ifndef ABSTRACT_CONTAINER_LIST_H_
#define ABSTRACT_CONTAINER_LIST_H_

#include <utility>

#if (__cplusplus < 201100) && !defined(nullptr)
#define nullptr 0
#endif

namespace abstract_container
{

#ifndef ABSTRACT_CONTAINER_DIRECTIONS_DEFIFINED_
#define ABSTRACT_CONTAINER_DIRECTIONS_DEFIFINED_

const bool forward = true;
const bool reverse = false;

#endif

namespace impl
{

template<int Line>
void static_error_at_line();

}

/*
Singly-linked list intrusive container class template

The 'abstractor' template parameter class must have the following public
or protected members, or behave as though it does:

type handle -- must be copyable.  Each element to be contained in a list
  must have a unique value of this type associated with it.

static const bool store_tail -- if true, the handle of the tail of the
  list is stored, making pushes in the reverse direction have
  constant rather that linear time.

Member functions:

handle null() -- must always return the same value, which is a handle value
  that is never associated with any element.  The returned value is called
  the null value.  Must be a static member.

void link(handle h, handle link_h) -- causes the handle value link_h to be
  stored withing the element associated with the handle value h.

handle link(handle h) -- must return the stored handle value that was most
  recently stored in the element associated with handle h by the other the
  link() member function.
*/
template <class abstractor>
class list : public abstractor
  {
  public:

    typedef typename abstractor::handle handle;

    static const bool store_tail = abstractor::store_tail;

    // Note:  all member functions have constant time complexity unless
    // noted as linear.

    static handle null() { return(abstractor::null()); }

    #if __cplusplus >= 201100
    template<typename ... args_t>
    list(args_t && ... args)
      : abstractor(std::forward<args_t>(args)...)
    #else
    list()
    #endif
      {
        head() = null();

        if (store_tail)
          tail() = null();
      }

    #if __cplusplus >= 201100

    list(const list &) = delete;

    list & operator = (const list &) = delete;

    #endif

    // Linear if direction is reverse.
    //
    handle link(handle h, bool is_forward = true)
      {
        if (is_forward)
          return(abstractor::link(h));

        handle result = null();
        for (handle h2 = head(); h2 != h; result = h2, h2 = link(h2))
          ;

        return(result);
      }

    // Put the specied element (which must not be part of any list) into
    // a state that it can only be in when not in any list.
    //
    void make_detached(handle h) { link(h, h); }

    // Returns true if make_detach() was called for the specified element,
    // and it has not since been put in any list.
    //
    bool is_detached(handle h) { return(link(h) == h); }

    // FUTURE
    // list(
    //   list &to_split, handle first_in_list, handle last_in_list)

    // Returns the handle of first element in the list in the given direction.
    // Returns the null value if the list is empty.  Linear if direction
    // is reverse and store_tail is false.
    //
    handle start(bool is_forward = true)
      {
        if (is_forward)
          return(head());

        if (store_tail)
          return(tail());

        handle result = null();
        for (handle h = head(); h != null(); result = h, h = link(h))
          ;

        return(result);
      }

    // For the element in_list (already in the list), inserts the element
    // to_insert after it in the given direction.  Linear if direction is
    // reverse.
    //
    void insert(handle in_list, handle to_insert, bool is_forward = true)
      {
        handle ilf = link(in_list, is_forward);

        if (!is_forward)
          {
            if (ilf == null())
              {
                push(to_insert);
                return;
              }

            handle tmp = in_list;
            in_list = ilf;
            ilf = tmp;
          }

        link(to_insert, ilf);
        link(in_list, to_insert);

        if ((ilf == null()) and store_tail)
          // New head in reverse direction.
          tail() = to_insert;
      }

    #if 0
    // FUTURE
    void insert(handle in_list, list &to_insert, bool is_forward = true)
      {
        if (to_insert.head[forward] == null())
          return;
        ...
      }
    #endif

    // Remorves the next elemment forward from specified element from the list.
    //
    void remove_forward(handle in_list)
      {
        handle f = link(in_list);
        handle ff = link(f);

        link(in_list, ff);

        if (store_tail and (ff == null()))
          tail() = in_list;
      }

    // Remorves the specified element (initially in the list) from the list.
    // Linear.
    //
    void remove(handle in_list)
      {
        handle f = link(in_list, forward);
        handle r = link(in_list, reverse);

        if (r == null())
          head() = f;
        else
          link(r, f);

        if (store_tail and (f == null()))
          tail() = r;
      }

    // FUTURE
    // void remove(handle first_in_list, handle last_in_list))

    // Make the specified element (not initially in the list) the new first
    // element in the list, in the specified direction.  Linear if direction
    // is reverse and tail is not stored.
    //
    void push(handle to_push, bool is_forward = true)
      {
        if (head() == null())
          {
            link(to_push, null());
            head() = to_push;
            if (store_tail)
              tail() = to_push;
          }
        else if (is_forward)
          {
            link(to_push, head());
            head() = to_push;
          }
        else if (store_tail)
          {
            link(to_push, null());
            link(tail(), to_push);
            tail() = to_push;
          }
        else
          {
            link(to_push, null());
            link(start(reverse), to_push);
          }
      }

    // FUTURE
    // void push(list &to_push, bool is_forward = true)

    // Removes and returns the first element (in the given direction) in the
    // list.  Linear if direction is reverse.
    //
    handle pop(bool is_forward = true)
      {
        handle p = head();
        handle f = link(p);
 
        if (is_forward or (f == null()))
          {
            head() = f;

            if (store_tail and (f == null()))
              tail() = null();
          }
        else
          {
            handle h;
            do
              {
                h = p;
                p = f;
                f = link(p);
              }
            while (f != null());

            link(h, null());
            if (store_tail)
              tail() = h;
          }

        return(p);
      }

    // Returns true if the list is empty.
    //
    bool empty() { return(head() == null()); }

    // Initialized the list to the empty state.
    //
    void purge()
      {
        head() = null();

        if (store_tail)
          tail() = null();
      }

  private:

    handle head_[1 + !!store_tail];

    handle & head() { return(head_[0]); }

    handle & tail()
      {
        if (!store_tail)
          // Cause link failure.  This detects an internal error (use
          // of tail whan it's not stored) in the list implementation,
          // not in the calling code.
          impl::static_error_at_line<__LINE__>();

        return(head_[1]);
      }

    void link(handle h, handle link_h) { abstractor::link(h, link_h); }

  }; // end list

namespace impl
{

template <bool>
struct p_list_abs;

template <bool store_tail>
class p_list_elem
  {
  public:

    const p_list_elem * link() const
      { return(link_); }

  private:

    p_list_elem *link_;

    friend struct impl::p_list_abs<store_tail>;
  };

template <bool store_tail_>
struct p_list_abs
  {
    typedef p_list_elem<store_tail_> *handle;

    static const bool store_tail = store_tail_;

    static handle null() { return(0); }

    static handle link(handle h) { return(h->link_); }

    static void link(handle h, handle link_h) { h->link_ = link_h; }
  };

} // end namespace impl

template <bool store_tail = true>
class p_list : public list<impl::p_list_abs<store_tail> >
  {
  public:

    typedef impl::p_list_elem<store_tail> elem;
  };

} // end namespace abstract_container

#endif /* Include once */
