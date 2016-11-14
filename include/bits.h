/******************************************************************************\
* Copyright (c) 2016, Robert van Engelen, Genivia Inc. All rights reserved.    *
*                                                                              *
* Redistribution and use in source and binary forms, with or without           *
* modification, are permitted provided that the following conditions are met:  *
*                                                                              *
*   (1) Redistributions of source code must retain the above copyright notice, *
*       this list of conditions and the following disclaimer.                  *
*                                                                              *
*   (2) Redistributions in binary form must reproduce the above copyright      *
*       notice, this list of conditions and the following disclaimer in the    *
*       documentation and/or other materials provided with the distribution.   *
*                                                                              *
*   (3) The name of the author may not be used to endorse or promote products  *
*       derived from this software without specific prior written permission.  *
*                                                                              *
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED *
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF         *
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO   *
* EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,       *
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, *
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;  *
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,     *
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR      *
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF       *
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                                   *
\******************************************************************************/

/**
@file      bits.h
@brief     RE/flex operations on STL containers and sets
@author    Robert van Engelen - engelen@genivia.com
@copyright (c) 2015-2016, Robert van Engelen, Genivia Inc. All rights reserved.
@copyright (c) BSD-3 License - see LICENSE.txt
*/

#ifndef REFLEX_BITS_H
#define REFLEX_BITS_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace reflex {

/**
RE/flex Bits class.

Dynamic bit vectors are stored in Bits objects, which can be manipulated
with the usual bit-operations (`|` (bitor), `&` (bitand), `^` (bitxor)).

Example:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
    reflex::Bits digit('0', '9'); // bits '0' (48th bit) to '9' (57th bit)
    reflex::Bits upper('A', 'Z'); // bits 'A' (65th bit) to 'Z' (92th bit)
    reflex::Bits lower('a', 'z'); // bits 'a' (97th bit) to 'z' (122th bit)
    if (upper.intersects(lower) == false)
      std::cout << "upper and lower are disjoint\n";
    reflex::Bits alnum = digit | upper | lower;
    if (alnum.contains(digit) == true)
      std::cout << "digit is a subset of alnum\n";
    if (alnum['_'] == false)
      std::cout << "_ is not in alnum\n";
    alnum['_'] = true;
    if (alnum['_'] == true)
      std::cout << "_ is in updated alnum\n";
    std::cout << alnum.count() << " bits in alnum\n";
    for (size_t i = alnum.find_first(); i != reflex::Bits::npos; i = alnum.find_next(i))
      std::cout << (char)i;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Output:

    upper and lower are disjoint
    digit is a subset of alnum
    _ is not in alnum
    _ is in updated alnum
    63 bits in alnum
    0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz

*/
class Bits {
 public:
  static const size_t npos = static_cast<size_t>(-1); ///< npos returned by find_first() and find_next()
  /// References a single bit, returned by operator[].
  struct Bitref {
    Bitref(
        size_t    n, ///< n'th bit
        uint64_t *p) ///< in this word
      : 
        m(1UL << n),
        p(p)
    { }
    uint64_t  m; ///< mask m = 2^n
    uint64_t *p; ///< in this word
    /// Returns bit value.
    /// @returns bit value true or false.
    operator bool() const
    {
      return *p & m;
    }
    /// Assign bit value.
    /// @returns result value true or false.
    bool operator=(bool b) const ///< bit
    {
      if (b)
        *p |= m;
      else
        *p &= ~m;
      return b;
    }
    /// Bit-or bit value.
    /// @returns result value true or false.
    bool operator|=(bool b) const ///< bit-or with this bit
    {
      if (b)
        *p |= m;
      return *p & m;
    }
    /// Bit-and bit value.
    /// @returns result value true or false.
    bool operator&=(bool b) const ///< bit-and with this bit
    {
      if (!b)
        *p &= ~m;
      return *p & m;
    }
    /// Bit-xor bit value.
    /// @returns result value true or false.
    bool operator^=(bool b) const ///< bit-xor with this bit
    {
      if (b)
        *p ^= m;
      return *p & m;
    }
  };
  /// Construct an empty bit vector.
  Bits()
    :
      len_(0),
      vec_(NULL)
  { }
  /// Copy constructor
  Bits(const Bits& bits) ///< bits to copy
  {
    operator=(bits);
  }
  /// Construct a bit vector and set n'th bit.
  Bits(size_t n) ///< n'th bit to set
    :
      len_(0),
      vec_(NULL)
  {
    insert(n);
  }
  /// Construct a bit vector and set a range of bits n1'th to n2'th.
  Bits(
      size_t n1, ///< first bit to set
      size_t n2) ///< last bit to set
    :
      len_(0),
      vec_(NULL)
  {
    insert(n1, n2);
  }
  /// Destroy bits.
  ~Bits()
  {
    if (vec_)
      delete vec_;
  }
  /// Assign bits.
  /// @returns reference to this object.
  Bits& operator=(const Bits& bits) ///< bits to copy
  {
    len_ = bits.len_;
    if (len_)
      memcpy(vec_ = new uint64_t[len_], bits.vec_, len_ << 3);
    else
      vec_ = NULL;
    return *this;
  }
  /// Reference n'th bit in the bit vector to assign a value to that bit.
  /// @returns bit reference to assign.
  Bitref operator[](size_t n) ///< n'th bit
  {
    alloc((n >> 6) + 1);
    return Bitref(n & 0x3F, &vec_[n >> 6]);
  }
  /// Returns n'th bit.
  /// @returns true if n'th bit is set, false otherwise.
  bool operator[](size_t n) const ///< n'th bit
  {
    return n >> 6 < len_ ? vec_[n >> 6] & 1UL << (n & 0x3F) : false;
  }
  /// Insert and set a bit in the bit vector.
  /// @returns reference to this object.
  Bits& insert(size_t n) ///< n'th bit to set
  {
    alloc((n >> 6) + 1);
    vec_[n >> 6] |= 1UL << (n & 0x3F);
    return *this;
  }
  /// Erase a bit in the bit vector.
  /// @returns reference to this object.
  Bits& erase(size_t n) ///< n'th bit to erase
  {
    if (n >> 6 < len_)
      vec_[n >> 6] &= ~(1UL << (n & 0x3F));
    return *this;
  }
  /// Flips a bit in the bit vector.
  /// @returns reference to this object.
  Bits& flip(size_t n) ///< n'th bit to flip
  {
    alloc((n >> 6) + 1);
    vec_[n >> 6] ^= 1UL << (n & 0x3F);
    return *this;
  }
  /// Insert and set a range of bits in the bit vector.
  /// @returns reference to this object.
  Bits& insert(
      size_t n1, ///< first bit to set
      size_t n2) ///< last bit to set
  {
    alloc((n2 >> 6) + 1);
    for (size_t i = n1; i <= n2; ++i)
      vec_[i >> 6] |= 1UL << (i & 0x3F);
    return *this;
  }
  /// Erase a range of bits in the bit vector.
  /// @returns reference to this object.
  Bits& erase(
      size_t n1, ///< first bit to erase
      size_t n2) ///< last bit to erase
  {
    if (n1 >> 6 < len_)
    {
      if (n2 >> 6 >= len_)
	n2 = (len_ - 1) << 6;
      for (size_t i = n1; i <= n2; ++i)
	vec_[i >> 6] &= ~(1UL << (i & 0x3F));
    }
    return *this;
  }
  /// Flip a range of bits in the bit vector.
  /// @returns reference to this object.
  Bits& flip(
      size_t n1, ///< first bit to flip
      size_t n2) ///< last bit to flip
  {
    alloc((n2 >> 6) + 1);
    for (size_t i = n1; i <= n2; ++i)
      vec_[i >> 6] ^= 1UL << (i & 0x3F);
    return *this;
  }
  /// Bit-or (set union) the bit vector with the given bits.
  /// @returns reference to this object.
  Bits& operator|=(const Bits& bits) ///< bits
  {
    alloc(bits.len_);
    for (size_t i = 0; i < bits.len_; ++i)
      vec_[i] |= bits.vec_[i];
    return *this;
  } 
  /// Bit-and (set intersection) the bit vector with the given bits.
  /// @returns reference to this object.
  Bits& operator&=(const Bits& bits) ///< bits
  {
    alloc(bits.len_);
    for (size_t i = 0; i < bits.len_; ++i)
      vec_[i] &= bits.vec_[i];
    for (size_t i = bits.len_; i < len_; ++i)
      vec_[i] = 0;
    return *this;
  } 
  /// Bit-xor the bit vector with the given bits.
  /// @returns reference to this object.
  Bits& operator^=(const Bits& bits) ///< bits
  {
    alloc(bits.len_);
    for (size_t i = 0; i < bits.len_; ++i)
      vec_[i] ^= bits.vec_[i];
    return *this;
  } 
  /// Bit-delete (set minus) the bit vector with the given bits.
  /// @returns reference to this object.
  Bits& operator-=(const Bits& bits) ///< bits
  {
    size_t k = len_;
    if (bits.len_ < k)
      k = bits.len_;
    for (size_t i = 0; i < k; ++i)
      vec_[i] = (vec_[i] | bits.vec_[i]) - bits.vec_[i];
    return *this;
  } 
  /// Bit-or (set union) of two bit vectors.
  /// @returns bit vector of the result.
  Bits operator|(const Bits& bits) const ///< bits
  {
    return Bits(*this) |= bits;
  } 
  /// Bit-and (set intersection) of two bit vectors.
  /// @returns bit vector of the result.
  Bits operator&(const Bits& bits) const ///< bits
  {
    return Bits(*this) &= bits;
  } 
  /// Bit-xor of two bit vectors.
  /// @returns bit vector of the result.
  Bits operator^(const Bits& bits) const ///< bits
  {
    return Bits(*this) ^= bits;
  } 
  /// Bit-delete (set minus) of two bit vectors.
  /// @returns bit vector of the result.
  Bits operator-(const Bits& bits) const ///< bits
  {
    return Bits(*this) -= bits;
  } 
  /// Complement of the bit vector with all bits flipped.
  /// @returns bit vector of the result.
  Bits operator~() const
  {
    return Bits(*this).flip();
  } 
  /// Returns true if bit vectors are equal.
  /// @returns true (equal) or false (unequal).
  bool operator==(const Bits& bits) const ///< rhs bits
  {
    size_t k = len_;
    if (bits.len_ < k)
      k = bits.len_;
    for (size_t i = 0; i < k; ++i)
      if (vec_[i] != bits.vec_[i])
        return false;
    for (size_t i = bits.len_; i < len_; ++i)
      if (vec_[i] != 0)
        return false;
    for (size_t i = len_; i < bits.len_; ++i)
      if (bits.vec_[i] != 0)
        return false;
    return true;
  }
  /// Returns true if bit vectors are unequal.
  /// @returns true (unequal) or false (equal).
  bool operator!=(const Bits& bits) const ///< rhs bits
  {
    return !operator==(bits);
  }
  /// Returns true if the bit vector is lexicographically less than the given right-hand side bits.
  /// @returns true (less) or false (greater-or-equal).
  bool operator<(const Bits& bits) const ///< rhs bits
  {
    size_t k = len_;
    if (bits.len_ < k)
      k = bits.len_;
    for (size_t i = 0; i < k; ++i)
    {
      if (vec_[i] < bits.vec_[i])
        return true;
      if (vec_[i] > bits.vec_[i])
        return false;
    }
    for (size_t i = bits.len_; i < len_; ++i)
      if (vec_[i] != 0)
        return false;
    for (size_t i = len_; i < bits.len_; ++i)
      if (bits.vec_[i] != 0)
        return true;
    return false;
  }
  /// Returns true if the bit vector is lexicographically greater than the given right-hand side bits.
  /// @returns true (greater) or false (less-or-equal).
  bool operator>(const Bits& bits) const ///< rhs bits
  {
    return bits.operator<(*this);
  }
  /// Returns true if the bit vector is lexicographically less-or-equal to the given right-hand side bits.
  /// @returns true (less-or-equal) or false (greater).
  bool operator<=(const Bits& bits) const ///< rhs bits
  {
    return !operator>(bits);
  }
  /// Returns true if the bit vector is lexicographically greater-or-equal to the given right-hand side bits.
  /// @returns true (greater-or-equal) or false (less).
  bool operator>=(const Bits& bits) const ///< rhs bits
  {
    return !operator<(bits);
  }
  /// Returns true if all bits are set.
  /// @returns true if all bits set, false otherwise.
  bool all() const
  {
    for (size_t i = 0; i < len_; ++i)
      if (vec_[i] + 1 != 0)
        return false;
    return true;
  }
  /// Returns true if any bit is set.
  /// @returns true if any bit set, false if none.
  bool any() const
  {
    for (size_t i = 0; i < len_; ++i)
      if (vec_[i] != 0)
        return true;
    return false;
  }
  /// Erase all bits.
  /// @returns reference to this object.
  Bits& clear()
  {
    if (vec_)
      memset(vec_, 0, len_ << 3);
    return *this;
  }
  /// Flip all bits.
  /// @returns reference to this object.
  Bits& flip()
  {
    for (size_t i = 0; i < len_; ++i)
      vec_[i] = ~vec_[i];
    return *this;
  }
  /// Reserves space in the bit vector for len bits without changing its current content.
  /// @returns reference to this object.
  Bits& reserve(size_t len) ///< number of bits to reserve
  {
    if (len)
      alloc(((len - 1) >> 6) + 1);
    return *this;
  }
  /// Returns the current length of the bit vector.
  /// @returns number of bits.
  size_t size() const
  {
    return len_ << 6;
  }
  /// Returns the number of bits set.
  /// @returns number of 1 bits.
  size_t count() const
  {
    size_t n = 0, k = 0;
    while ((n = find_first(n)) != npos)
      ++n, ++k;
    return k;
  }
  /// Returns true if the bit vector intersects with the given bits, false if the bit vectors are disjoint.
  /// @returns true if bits intersect or false if disjoint.
  bool intersects(const Bits& bits) const ///< bits
  {
    size_t k = len_;
    if (bits.len_ < k)
      k = bits.len_;
    for (size_t i = 0; i < k; ++i)
      if (vec_[i] & bits.vec_[i])
        return true;
    return false;
  }
  /// Returns true if the given bits are a subset of the bit vector, i.e. for each bit in bits, the corresponding bit in the bit vector is set.
  /// @returns true if bits is a subset.
  bool contains(const Bits& bits) const ///< bits
  {
    size_t k = len_;
    if (bits.len_ < k)
      k = bits.len_;
    for (size_t i = 0; i < k; ++i)
      if (vec_[i] != (vec_[i] | bits.vec_[i]))
        return false;
    for (size_t i = len_; i < bits.len_; ++i)
      if (bits.vec_[i] != 0)
        return false;
    return true;
  }
  /// Returns the position of the first bit set in the bit vector, or Bits::npos if none.
  /// @returns first position or Bits::npos.
  size_t find_first(size_t n = 0) const ///< internal parameter (do not use)
  {
    size_t i = n >> 6;
    if (i < len_ && vec_[i])
      for (size_t j = n & 0x3F; j < 64; ++j)
        if (vec_[i] & 1UL << j)
          return (i << 6) + j;
    for (i = i + 1; i < len_; ++i)
      if (vec_[i])
        for (size_t j = 0; j < 64; ++j)
          if (vec_[i] & 1UL << j)
            return (i << 6) + j;
    return npos;
  }
  /// Returns the next position of a bit set in the bit vector, or Bits::npos if none.
  /// @returns next position or Bits::npos.
  size_t find_next(size_t n) const ///< the current position to search from
  {
    return find_first(n + 1);
  }
  /// Swap bit vectors.
  void swap(Bits& bits) ///< bits
  {
    uint64_t k = len_;
    uint64_t *p = vec_;
    len_ = bits.len_;
    vec_ = bits.vec_;
    bits.len_ = k;
    bits.vec_ = p;
  }
 private:
  /// On-demand allocator.
  void alloc(size_t len) ///< number of words required
  {
    if (len > len_)
    {
      size_t k = 1;
      while (k < len)
        k <<= 1;
      uint64_t *p = new uint64_t[k]();
      if (vec_)
      {
        memcpy(p, vec_, len_ << 3);
        delete[] vec_;
      }
      len_ = k;
      vec_ = p;
    }
  }
  size_t    len_; ///< number of words
  uint64_t *vec_; ///< array of words
};

} // namespace reflex

#endif