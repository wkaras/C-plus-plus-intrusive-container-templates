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
#ifndef ABSTRACT_CONTAINER_UTIL_H_
#define ABSTRACT_CONTAINER_UTIL_H_

// Utilities.

namespace abstract_container
{

namespace impl
{

const unsigned dummy_address = 0x100;

}

} // end namespace abstract_container

#define ABSTRACT_CONTAINER_MBR_OFFSET_IN_CLS(CLS_NAME, FLD_SPEC) \
  (reinterpret_cast<char *>( \
    &(reinterpret_cast<CLS_NAME *>( \
        abstract_container::impl::dummy_address)->FLD_SPEC)) - \
   reinterpret_cast<char *>(abstract_container::impl::dummy_address))

// If p_list::elem or p_bidir_list::elem are a data member rather than
// a base class, this macro converts a pointer to the data member into a
// pointer to the containing class.
//
#define ABSTRACT_CONTAINER_MBR_TO_CLS_PTR(CLS_NAME, FLD_SPEC, FLD_PTR) \
  reinterpret_cast<CLS_NAME *>( \
    reinterpret_cast<char *>(FLD_PTR) - \
    ABSTRACT_CONTAINER_MBR_OFFSET_IN_CLS(CLS_NAME, FLD_SPEC))

#endif /* Include once */
