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
#ifndef ABSTRACT_CONTAINER_MODULUS_HASH_H_
#define ABSTRACT_CONTAINER_MODULUS_HASH_H_

/*
Modulus hash of long-length keys.

Implementation Strategy

The key to hash must consist of segments of a fixed number of bits.  The
key can be thought of as a high-precision number given by this sum:

S(0) + S(1) * (2 ** Sbits) + ... + S(n) * (2 ** (Sbits * n))

S(i) means segment number i.  Sbits is the bit width of each segment.  The
modulus hash MH is given by the recusive formula:

MH(n) = ((S(n) * ((2 ** (Sbits * n)) mod M)) + MH(n - 1)) mod M

with MH(0) = S(0) mod M .  M is the modulus.

A faster but less general approach is to not reduce each intermediate sum
to its modulus:

SUM(n) = (S(n) * ((2 ** (Sbits * n)) mod M)) + SUM(n - 1)

with SUM(0) = S(0) and MH(n) = SUM(n) mod M.  The drawback is that some
SUM(i) may overflow the precision of the accumulator.
*/

namespace abstract_container
{

// Calculate a segment coefficient for key modulus hash calculation.
//
// Required public members (or equivalents) of traits class parameter:
//
// Types:
// modulus_t -- an integral type with enough precision to hold the modulus
//   value.
// product_t -- and integral type.  The bit width must be at least 1 more than
//   the sum of the bit width needed to hold the modulus value, plus
//   the bit width of one key segment.
//
// Constants:
// static const unsigned key_segment_bits -- number of bits in each key
//   segment
// static const modulus_t modulus -- the modulus for the hash.
//
// The template parameter key_segment gives the 0-base number of the
// key segment to provide the coefficient for.
//
template <class traits, unsigned key_segment>
class modulus_hash_coeff
  {
  public:

    typedef typename traits::modulus_t modulus_t;
    typedef typename traits::product_t product_t;

  private:

    static const product_t coeff1 =
      (product_t(1) << traits::key_segment_bits) % traits::modulus;

  public:

    static const modulus_t val =
      (coeff1 * modulus_hash_coeff<traits, key_segment - 1>::val) %
      traits::modulus;
  };

template <class traits>
class modulus_hash_coeff<traits, 0>
  {
  public:

    typedef typename traits::modulus_t modulus_t;
    typedef typename traits::product_t product_t;

    static const modulus_t val = 1;
  };

namespace impl
{

// Helper template for calculating modulus hash.
//
// The traits class template parameter must have the public members it
// needs for modulus_hash_coeff, plus the following:
//
// Type:
// key -- the type of the key to take the modulus hash of.
//
// Constant:
// static const unsigned num_key_segments -- the maximum number of segments
//   in a key.
//
// Template:
// template <unsigned key_segment> static it get_segment(key k) -- returns
//   the key segment whose 0-base number is key_segment from the key k.
//   'it' means some integral type large enough to hold the segment.
//
// reverse_key_segment should be num_key_segments minus one, in the first
// call to a member function of this templeted class, which starts the
// template recursion.
//
template <class traits, unsigned reverse_key_segment>
class modulus_hash
  {
  public:

    typedef typename traits::modulus_t modulus_t;
    typedef typename traits::product_t product_t;

    static const unsigned key_segment =
      traits::num_key_segments - 1 - reverse_key_segment;

    // Call this function when the number of segments to hash is
    // equal to num_key_segments.
    //
    static modulus_t val(typename traits::key k)
      {
        return(
          ((product_t(traits::template get_segment<key_segment>(k)) *
            modulus_hash_coeff<traits, key_segment>::val) +
           modulus_hash<traits, reverse_key_segment - 1>::val(k)) %
          traits::modulus);
      }

    // Call this function when the number of segments to hash is
    // less than num_key_segments.  The number of segments is given
    // by key_seg_count.  The hash is preformed on the lowest-numbered
    // segments.
    //
    static modulus_t val(typename traits::key k, unsigned key_seg_count)
      {
        product_t prod =
          product_t(traits::template get_segment<key_segment>(k)) *
          modulus_hash_coeff<traits, key_segment>::val;

        if (key_seg_count == 1)
          return(prod % traits::modulus);

        return(
          (prod +
           modulus_hash<traits, reverse_key_segment - 1>::val(
             k, key_seg_count - 1)) % traits::modulus);
      }

    // Call this function when the number of segments to hash is
    // equal to num_key_segments.
    //
    static product_t fast_val(typename traits::key k)
      {
        return(
          (product_t(traits::template get_segment<key_segment>(k)) *
            modulus_hash_coeff<traits, key_segment>::val) +
          modulus_hash<traits, reverse_key_segment - 1>::fast_val(k));
      }

    // Call this function when the number of segments to hash is
    // less than num_key_segments.  The number of segments is given
    // by key_seg_count.  The hash is preformed on the lowest-numbered
    // segments.
    //
    static product_t fast_val(typename traits::key k, unsigned key_seg_count)
      {
        product_t prod =
          product_t(traits::template get_segment<key_segment>(k)) *
          modulus_hash_coeff<traits, key_segment>::val;

        if (key_seg_count == 1)
          return(prod);

        return(
          prod +
          modulus_hash<traits, reverse_key_segment - 1>::fast_val(
            k, key_seg_count - 1));
      }
  };

template <class traits>
class modulus_hash<traits, 0>
  {
  public:

    typedef typename traits::modulus_t modulus_t;
    typedef typename traits::product_t product_t;

    static const unsigned key_segment = traits::num_key_segments - 1;

    static modulus_t val(typename traits::key k, unsigned = 0)
      {
        return(
          (product_t(traits::template get_segment<key_segment>(k)) *
            modulus_hash_coeff<traits, key_segment>::val) %
          traits::modulus);
      }

    static product_t fast_val(typename traits::key k, unsigned = 0)
      {
        return(
          product_t(traits::template get_segment<key_segment>(k)) *
           modulus_hash_coeff<traits, key_segment>::val);
      }
  };

} // end namespace impl

// Hash all key segments.  'traits' has the same requirements as for
// the impl::modulus_hash template.
//
template<class traits>
inline typename traits::modulus_t modulus_hash(typename traits::key k)
  {
    return(impl::modulus_hash<traits, traits::num_key_segments - 1>::val(k));
  }

// Hash some of the key segments, starting with segment 0, with the number
// of segments hash given by the second parameter.  'traits' has the same
// requirements as for the impl::modulus_hash template.
//
template<class traits>
inline typename traits::modulus_t modulus_hash(
  typename traits::key k, unsigned key_segments_count)
  {
    return(
      impl::modulus_hash<traits, traits::num_key_segments - 1>::val(
        k, key_segments_count));
  }

// Hash all key segments.  'traits' has the same requirements as for
// the impl::modulus_hash template.  As explained above, the final
// and intermediate sums must fit within the precision of
// traits::product_t.
//
template<class traits>
inline typename traits::modulus_t modulus_hash_fast(typename traits::key k)
  {
    return(
      impl::modulus_hash<traits, traits::num_key_segments - 1>::fast_val(k) %
      traits::modulus);
  }

// Hash some of the key segments, starting with segment 0, with the number
// of segments hash given by the second parameter.  'traits' has the same
// requirements as for the impl::modulus_hash template.  As explained above,
// the final and intermediate sums must fit within the precision of
// traits::product_t.
//
template<class traits>
inline typename traits::modulus_t modulus_hash_fast(
  typename traits::key k, unsigned key_segments_count)
  {
    return(
      (impl::modulus_hash<traits, traits::num_key_segments - 1>::fast_val(
         k, key_segments_count)) % traits::modulus);
  }

} // end namespace abstract_container

#endif /* Include once */
