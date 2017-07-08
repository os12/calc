/* --------------------------------------------------------------
    Signed integers with unlimited range (version 2.0).
    Base functions in machine-independent form.

    http://www.imach.uran.ru/cbignum

    Copyright 1999-2010 by Raul N.Shakirov, IMach of RAS(UB).
    All Rights Reserved.

    Permission has been granted to copy, distribute and modify
    software in any context without fee, including a commercial
    application, provided that the aforesaid copyright statement
    is present here as well as exhaustive description of changes.

    THE SOFTWARE IS DISTRIBUTED "AS IS". NO WARRANTY OF ANY KIND
    IS EXPRESSED OR IMPLIED. YOU USE AT YOUR OWN RISK. THE AUTHOR
    WILL NOT BE LIABLE FOR DATA LOSS, DAMAGES, LOSS OF PROFITS OR
    ANY OTHER KIND OF LOSS WHILE USING OR MISUSING THIS SOFTWARE.
-------------------------------------------------------------- */
#ifndef _CBIGNUMF_INL
#define _CBIGNUMF_INL

#ifndef _CBIGNUMF_H
#include "Cbignumf.h"
#endif//_CBIGNUMF_H

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#define DISALLOW_EXARRAY_SHORTTYPE
#include "Cbignum.h"
#define BITS    (CHAR_BIT * sizeof (CBNL))

static int count_floor2nBITS()                  // Maximal power of 2
{                                               // not greater BITS-1,
  int n = 2; while (n <= (BITS-1)/2) n <<= 1;   // usually BITS/2.
  return n;
}
static int floor2nBITSm1 = count_floor2nBITS() - 1;

#ifdef  _MSC_VER
#pragma auto_inline (off)
#endif/*_MSC_VER*/

//================================================
//      Enabling of Cbignumf.cpp optimization.
//================================================
//      _CBIGNUM_OPTIMIZED_COPY     copying.

#ifdef  _CBIGNUM_ASM

#if 1
#define _CBIGNUM_OPTIMIZED_COPY
#endif

#endif//_CBIGNUM_ASM

//================================================
//      Service functions.
//================================================

inline size_t size_max (size_t n1, size_t n2)
{
  return ((n1 > n2)? n1 : n2);
}

inline size_t size_min (size_t n1, size_t n2)
{
  return ((n1 < n2)? n1 : n2);
}

//================================================
//      Use C naming conventions.
//================================================

#ifdef  NCHECKPTR
#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus
#endif//NCHECKPTR

//================================================
//      Implementation of cBigNumber operations.
//================================================
//      Unlimited number is stored in array of type CBNL.
//      Word 0 contains number of CBNL words, required for
//      storing of complementary code of number, the next
//      words contain complementary code in order from low
//      words to high words. Sign resides in the last word.
//
//      Operation are implemented as functions of language C.
//      Unlimited numbers are passed through macro EXPTR(CBNL):
//      this is CBNL* in release mode and exptr<CBNL> in
//      debug mode with check of bounds.
//      Input numbers are declared with modifier const.
//
//      For optimization purposes some functions return
//      number of words instead of storing it in word 0.
//      Names of these functions are started from underscore,
//      they are accompanied by inline function which
//      calls function and then fills in word 0.
//
//      Dynamic check if input data is controlled by the
//      following macro:
//      NDEBUG    - turn off check of bounds and assert
//                  checks of input parameters.
//      NCHECKPTR - turn off check of bounds.
//
//      Cycles in functions cBigNumber... are optimized
//      to reduce number of checks and branches when
//      processing one-word numbers. Sequence operations
//      is selected to meet the criteria and optimal
//      loading of Pentium conveyers.

//================================================
//      Diagnostic variables and functions.
//================================================

cTTY    cBigNumberMessages;             // Handler for messages.
cTTY    cBigNumberProgress;             // Handler for progress indicators.

//      Dump of internal representation is performed in order
//      from high words to low words. Each word is preceded by
//      space and outputs in hexadecimal format, in order from
//      high bits to low bits. Output is finished by new line.

void    cBigNumberDump (                        // Dump of code to messages.
                const   CBPTR(CBNL) p1          // Unlimited number.
        )
{
  size_t n1 = (size_t)(*CBPTRBASE(p1));         // Number of words.
  for (; n1; n1--) cBigNumberMessages <<" "<< (const char*) cHexDump (p1 [n1]);
  cBigNumberMessages << "\n";
}

//      Short dump of hexadecimal internal code contains
//      the high word followed by total number of bytes.

void    cBigNumberInfo (                        // Information (short dump).
                const   CBPTR(CBNL) p1          // Unlimited number.
        )
{
  size_t n1 = (size_t)(*CBPTRBASE(p1));         // Number of words.
  cBigNumberMessages << cHexDump (n1? p1 [n1]: 0);
  if (n1 > 1)
    cBigNumberMessages << "... (" << (CBNL)(n1 * sizeof (CBNL)) << " bytes)\n";
}

void    cBigNumberERange  (                     // Error message.
                const   CBPTR(CBNL) p1          // Unlimited number.
        )
{
  cBigNumberMessages << "cBigNumber value out of range: ";
  cBigNumberInfo (p1);
  abort();
}

//================================================
//      Information functions.
//================================================

//      Meaning words are most senior word, which does not contain
//      only sign and sign extension bits and all younger words.

size_t  cBigNumberWords (                       // Number of meaning words.
                const   CBPTR(CBNL) p1          // Unlimited number.
        )                                       // May be not normalized.
{
  size_t n1 = (size_t)(*CBPTRBASE(p1));         // Number of words.
  if (n1)
  {
    CBNL sign = p1 [n1] >> (BITS-1);            // Sign 0 or -1.
    do
    {
      if (p1 [n1] != sign) break;               // Are there meaning bits?
    }
    while (--n1 != 0);
  }
  return (n1);
}

//      In signed number meaning bits are most senior bit, which is
//      distinct from sign bit and all younger bits. For example,
//      0 and -1 contain no meaning bits, 1 and -2 contain 1 meaning bit,
//      127 and -128 contain 7 meaning bits. Absolute value of number
//      containing n meaning bits is not greater than 2 in power n.

int     cLongBits (CBNL num)                    // Number of meaning bits
{                                               // in the range 0..BITS-1.
  int n = 0;
  num ^= (num >> (BITS-1));                     // Negative number.
  int k = floor2nBITSm1;                        // Shift ..15,7,3,1.
  do
  {
    int k0 = (((num >> k) == 0) - 1) & (k + 1); num >>= k0; n += k0;
  }
  while ((k >>= 1) != 0);
  return (n += (num > 0));                      // Shift 0.
}

//      In unsigned number meaning bits are most senior bit which
//      is distinct from 0 and all younger bits.

int     cULongBits (unsigned CBNL num)          // Number of meaning bits
{                                               // in the range 0..BITS.
  int n = (((CBNL)num >= 0) - 1); num &= ~n;
      n &= BITS;                                // BITS meaning bits.
  int k = floor2nBITSm1;                        // Shift ..15,7,3,1.
  do
  {
    int k0 = (((num >> k) == 0) - 1) & (k + 1); num >>= k0; n += k0;
  }
  while ((k >>= 1) != 0);
  return (n += (num > 0));                      // Shift 0.
}

//      Count meaning bits in unlimited number.
//      If count > CBNL_MAX cBigNumberERange() is called.

CBNL    cBigNumberBits (                        // Number of meaning bits.
                const   CBPTR(CBNL) p1          // Unlimited number.
        )                                       // May be not normalized.

{
  size_t n = cBigNumberWords(p1);               // Number of meaning words.
  size_t n1 = (size_t)(*CBPTRBASE(p1));         // Number of words.
  if (n)
  {                                             // Check for overflow.
    if ((CBNL)n > (CBNL)(CBNL_MAX / BITS))
       cBigNumberERange (p1);
    return ((CBNL)(n - 1) * BITS + cULongBits ((unsigned CBNL)
                                    (p1 [n] ^ (p1 [n1] >> (BITS-1)))));
  }
  return (0);
}

//      Low 0-words are all junior meaning zero words.
//      If all words are 0, there are no low 0-words.

size_t  cBigNumberExWords (                     // Number of low 0-words.
                const   CBPTR(CBNL) p1          // Unlimited number.
)                                               // May be not normalized.
{
  size_t n1 = (size_t)(*CBPTRBASE(p1));         // Number of words.
  for (size_t n = 0; n < n1; n++)
  {
    if (p1 [n + 1] != 0) return (n);            // Number of 0-words.
  }
  return (0);
}

//      Low 0-bits are all junior meaning zero bits.
//      For example, 0 and odd numbers contain no low 0-bits,
//      2 and -2 contain 1 low 0-bit, 128 and -128 contain
//      7 low 0-bits. Absolute value of number containing
//      n low 0-bits is not less than 2 in power n.

int     cLongExBits (CBNL num)                  // Number of low 0-bits
{                                               // in the range 0..BITS-1.
  int n = 0;
  if (num) while ((num & (CBNL)1) == 0) { ++n; num >>= 1; }
  return (n);
}

//      Count low 0-bits in unlimited number.
//      If count > CBNL_MAX cBigNumberERange() is called.

CBNL    cBigNumberExBits (                      // Number of low 0-bits.
                const   CBPTR(CBNL) p1          // Unlimited number.
        )                                       // May be not normalized.

{
  size_t n = cBigNumberExWords(p1);             // Number of low 0-words.
  size_t n1 = (size_t)(*CBPTRBASE(p1));         // Number of words.
  if (n1)
  {                                             // Check for overflow.
    if ((CBNL)n > (CBNL)((CBNL_MAX - (BITS - 1)) / BITS))
      cBigNumberERange (p1);
    return ((CBNL)n * BITS + cLongExBits (p1 [n + 1]));
  }
  return (0);
}

//      Determine if number is positive and exactly 2 in power n.

int     cLongExactLog2 (CBNL num)               // Either exact log2 or -1.
{                                               // log2 in the range 0..BITS-1.
  int n = cLongExBits (num);
  if ((num >> n) != 1) n = -1;
  return n;
}

//================================================
//      Normalization and denormalization.
//================================================

//      The number is normalized if minimal number of CBNL words
//      are used for storing, but no less then one word:
//      - if length()>1, the high word differs from sign extension
//        of word preceding the high word;
//      - zero is represented by single 0-word.

size_t _CBNL_C  cBigNumberIsFit (               // Check of normalization.
                const   CBPTR(CBNL) p1          // Unlimited number.
        )                                       // If number is normalized,
{                                               // return 1 else 0.
  size_t n1 = (size_t)(*CBPTRBASE(p1));         // Number of words.

  if (n1 > 1) return ((p1 [n1] != (p1 [n1-1] >> (BITS-1))));

  return (n1);                                  // Number of words is 1 or 0.
}

//      Normalization to minimal number of words, not less then 1.

size_t _CBNL_C _cBigNumberFit (                 // Normalization.
                        EXPTR(CBNL) p1          // Buffer with number.
        )                                       // Size of buffer >= 2.
{
  register size_t n1 = (size_t)(*p1);           // Number of words.
  p1 += n1;                                     // Preparing of pointer.

  if (n1)                                       // If bits of the high word
  {                                             // and high bit of the word
    register CBNL pass, lp = *p1;               // before high are all 0 or 1
    do continue;                                // then delete the high word.
    while (--n1 != 0 &&
        (pass = lp, lp = p1 [-1], --p1, pass == (lp >> (BITS-1))));
  }                                             // If no words at all
  else { p1 [1] = 0; }                          // then add word 0.

  return (n1 + 1);                              // Number of words.
}
//#define cBigNumberFit(p1)     (size_t)(*(p1)=_cBigNumberFit(p1))

//      Denormalization with increasing of number of words up to
//      given number. If given number is not greater then initial
//      number of words function adds one more word.
//
//      Function returns NOT normalized result.

size_t _CBNL_C _cBigNumberFitTo (               // Denormalization.
                        EXPTR(CBNL) p1,         // Buffer with number.
                        size_t      n           // Number of words.
        )                                       // Size of buffer no less
{                                               // max (*p1 + 2, n + 1).
  register size_t n1 = (size_t)(*p1);           // Number of words.
  register CBNL pass = p1 [n1] >> (BITS-1);     // Carry.

  do
    p1 [++n1] = pass;                           // Expand the carry.
  while (n1 < n);

  return (n1);                                  // Number of words.
}
//#define cBigNumberFitTo(p1,n) (size_t)(*(p1)=_cBigNumberFitTo(p1,n))

//================================================
//      Functions of comparison.
//================================================

//      Sign bit of more short number expanded to make
//      number of words in both numbers the same.
//      Then the numbers compared word by word in order
//      from high words to low words until mismatch.
//      Comparison of the high words is signed,
//      comparison of lower words is unsigned.

//      For optimization purposes algorithmic equivalent
//      is applied instead of actual expanding of sign bit.

int    _CBNL_C  cBigNumberComp (                // Comparison of p1, p2.
                const   CBPTR(CBNL) p1,         // Number.
                const   CBPTR(CBNL) p2          // Number.
        )                                       // Returns -1 if p1 <  p2
                                                // Returns  0 if p1 == p2
                                                // Returns  1 if p1  > p2
{                                               // p1, p2 may overlap.
  size_t n1 = (size_t)(*CBPTRBASE(p1));         // Number of words in p1.
  size_t n2 = (size_t)(*CBPTRBASE(p2));         // Number of words in p2.
  register CBNL lp1 = 0;                        // Current word of p1.
  register CBNL lp2 = 0;                        // Current word of p2.

//      Comparison of sign words.

  if (n1) lp1 = p1 [n1--];                      // Sign word of p1.
  if (n2) lp2 = p2 [n2--];                      // Sign word of p2.

  if (n1 == n2)                                 // Lengths of p1, p2 the same.
  {
    if (lp1 != lp2) goto NotSameSignWord;       // Sign words not the same.
  }
  else if (n1 > n2)                             // If number p1 longer, then
  {                                             // compare it with expansion
    lp2 >>= (BITS-1); ++n2;                     // of sign of p2.
    if (lp1 != lp2) goto NotSameSignWord;       // Sign words not the same.
    if (n1 > n2)                                // Comparing non-sign words.
    {
      do                                        // Cycle of comparison.
      {
        lp1 = p1 [n1]; --n1;                    // Read word.
        if (lp1 != lp2) goto NotSame;           // Words not the same.
      }
      while (n1 > n2);                          // End of cycle.
    }
  }
  else // (n1 < n2)                             // If number p2 longer, then
  {                                             // compare it with expansion
    lp1 >>= (BITS-1); ++n1;                     // of sign of p2.
    if (lp1 != lp2) goto NotSameSignWord;       // Sign words not the same.
    if (n1 < n2)                                // Comparing non-sign words.
    {
      do                                        // Cycle of comparison.
      {
        lp2 = p2 [n2]; --n2;                    // Read of words.
        if (lp1 != lp2) goto NotSame;           // Words are not the same.
      }
      while (n1 < n2);                          // End of cycle.
    }
  }

//      Pair-wise comparison of non-sign words.

  if (n1)
  {
    do                                          // Cycle of comparison.
    {
      lp1 = p1 [n1]; lp2 = p2 [n1]; --n1;       // Read word.
      if (lp1 != lp2) goto NotSame;             // Words are not the same.
    }
    while (n1);                                 // End of cycle.
  }

  return (0);                                   // Numbers are equal.
NotSame:                                        // Numbers are not equal.
  return (((unsigned CBNL)lp1 >= (unsigned CBNL)lp2) * 2 - 1);
NotSameSignWord:                                // Sign words are not equal.
  return ((lp1 >= lp2) * 2 - 1);
}

//      Special comparison for division.
//      It is assumed:
//      - the numbers are normalized.
//      - size of first number not less then size of second number.
//      Before comparison second number is left shifted to make sizes the same.

int    _CBNL_C  cBigNumberCompHigh (            // Comparison from high words.
                const   CBPTR(CBNL) p1,         // Number.
                const   CBPTR(CBNL) p2          // Number.
        )                                       // Returns -1 if p1 <  p2
                                                // Returns  0 if p1 == p2
                                                // Returns  1 if p1  > p2
{                                               // p1, p2 may overlap.
  size_t n1 = (size_t)(*CBPTRBASE(p1));         // Number of words in p1.
  size_t n2 = (size_t)(*CBPTRBASE(p2));         // Number of words in p2.
  assert (n1 >= n2);                            // Check of size.
  assert (n2 > 0);                              // Check of size.

  register CBNL lp1 = p1 [n1--];                // Current word of p1.
  register CBNL lp2 = p2 [n2--];                // Current word of p2.
  assert (!n1 || lp1 != (p1 [n1] >> (BITS-1))); // Check of normalization.
  assert (!n2 || lp2 != (p2 [n2] >> (BITS-1))); // Check of normalization.

//      Comparison of sign words.

  if (lp1 != lp2) return ((lp1 > lp2) * 2 - 1); // Words are not the same.

//      Pair-wise comparison of non-sign words.

  if (n2)
  do                                            // Cycle of comparison.
  {
    lp1 = p1 [n1--]; lp2 = p2 [n2--];           // Read words.
    if (lp1 != lp2)                             // Words are not the same.
      return (((unsigned CBNL)lp1 > (unsigned CBNL)lp2) * 2 - 1);
  }
  while (n2);                                   // End of cycle.

//      Comparison over zone of shift.

  if (n1)
  do
  {
    if (p1 [n1--] != 0) return 1;               // Not the same.
  }
  while (n1);

  return (0);                                   // Numbers are equal.
}

//      Special fast comparison of number with 0.
//
//      Algorithm is optimized to exclude conditional branches.
//      It returns bit-wise sum of sign word and number of words
//      divided by 2 (sign word of positive number may be 0,
//      but in such case number of words > 1).
//
//      It is assumed that the number has no redundant high 0-words,
//      otherwise wrong negative result is possible.


inline  CBNL    cBigNumberNot0  (               // Comparison with 0:
                const CBPTR(CBNL) p1_debug      // returns 0 if p1 == 0
                )                               // else non 0.
{
  const CBNL* p1 = CBPTRBASE(p1_debug);         // Conventional pointer.
  size_t n1 = (size_t)(*p1);                    // Number of words.
  return (p1 [n1] | (*p1 >> 1));
}

//================================================
//      Copying and bit-wise inversion.
//================================================

#ifndef _CBIGNUM_OPTIMIZED_COPY                 // If not in Cbignumf.cpp.

//      Exact copying.
//
//      Function neither requires for normalization of operand
//      nor normalizes it, but returns normalized result if
//      operand is normalized.

void   _CBNL_C  cBigNumberCopy (                // Copying p = p1.
                const   CBPTR(CBNL) p1,         // Number to copy.
                        EXPTR(CBNL) p           // Buffer of size *p1 + 1.
        )                                       // p1, p may overlap.
{
  size_t n1 = (size_t)(*p++ = *CBPTRBASE(p1++));// Number of words.
  if (n1)
  {
    do                                          // Cycle of copying of words.
      { *p++ = *p1++; }                         // Copying of word.
    while (--n1 != 0);                          // End of cycle.
  }
}

#endif//_CBIGNUM_OPTIMIZED_COPY

//      Bit-wise inversion.
//
//      Function neither requires for normalization of operand
//      nor normalizes it, but returns normalized result if
//      operand is normalized.

void    cBigNumberCompl (                       // Inversion p = ~p1.
                const   CBPTR(CBNL) p1,         // Number to invert.
                        EXPTR(CBNL) p           // Buffer of size
                                                // max (*p1 + 1, 2).
        )                                       // p1, p may overlap.
{
  size_t n1 = (size_t)(*p++ = *CBPTRBASE(p1++));// Number of words.
  if (n1)
  {
    do                                          // Cycle of inversion of words.
      { *p++ = ~*p1++; }                        // Inversion of word.
    while (--n1 != 0);                          // End of cycle.
  }
  else
  {
    p [-1] = 1; p [0] = ~(CBNL)0;               // Number ~0.
  }
}

//================================================
//      Shift to given number of words.
//================================================

//      Left shift increases number of words, if number to shift is not 0,
//      Shift is equivalent to multiplication by 2 in power (BITS * k1).
//
//      Function neither requires for normalization of operand
//      nor normalizes it, but returns normalized result if
//      operand is normalized.

void    cBigNumberCopyShl (                     // Shift p = p1 << (BITS*k1).
                const   CBPTR(CBNL) p1,         // Number to shift.
                        size_t      k1,         // Degree of shift in words.
                        EXPTR(CBNL) p           // Buffer of size
                                                // max (*p1 + k1, 1) + 1.
        )                                       // p1, p may overlap.
{
  size_t n1 = (size_t)(*CBPTRBASE(p1));         // Number of words.
  if (n1 <= 1 && p1 [n1] == 0) k1 = 0;          // Number 0.
  p1 += n1; *p = (CBNL)(n1 + k1); p += (n1 + k1);
  if (n1)
  do                                            // Cycle of copying of words.
    { *p-- = *p1--; }                           // Copying of word.
  while (--n1 != 0);                            // End of cycle.
  if (k1)
  do                                            // Cycle of clearing of words.
    { *p-- = 0; }                               // Clearing of word.
  while (--k1 != 0);                            // End of cycle.
}

//      Right shift saves sign of number.
//      Shift is equivalent to division by 2 in power (BITS * k1).
//
//      Function neither requires for normalization of operand
//      nor normalizes it, but returns normalized result if
//      operand is normalized.

void    cBigNumberCopyShr (                     // Shift p = p1 >> (BITS*k1).
                const   CBPTR(CBNL) p1,         // Number to shift.
                        size_t      k1,         // Degree of shift in words.
                        EXPTR(CBNL) p           // Buffer of size
                                                // max (*p1 - k1, 1) + 1.
        )                                       // p1, p may overlap.
{
  size_t n1 = (size_t)(*CBPTRBASE(p1));         // Number of words.
  if (n1 > k1)
  {
    *p++ = (CBNL)(n1 -= k1);                    // Number of words.
    p1 += (k1 + 1);                             // Shift.
    do                                          // Cycle of copying of words.
      { *p++ = *p1++; }                         // Copying of word.
    while (--n1 != 0);                          // End of cycle.
  }
  else
  {
    *p = 1; p [1] = p1 [n1] >> (BITS-1);        // Number 0 or -1.
  }
}

//      Clipping of some number of words and adding of high 0 word.
//
//      Function returns NOT normalized result.

void    cBigNumberCutOut (                      // Clipping.
                const   CBPTR(CBNL) p1,         // Number to clip.
                        size_t      k1,         // Number of words to skip.
                        size_t      n,          // Number of words to copy.
                        EXPTR(CBNL) p           // Buffer of size n1 + 1.
        )                                       // p1, p may overlap.
{
  assert ((size_t)(*CBPTRBASE(p1)) >= k1 + n);  // Check of size.
  *p++ = (CBNL)(n + 1);                         // Number of words.
  if (n > 0)                                    // Copying is required.
  {
    p1 += (k1 + 1);                             // Start.
    do                                          // Cycle of copying of words.
      { *p++ = *p1++; }                         // Copying of word.
    while (--n != 0);                           // End of cycle.
  }
  *p = 0;                                       // The high word.
}

//================================================
//      Unary operations.
//================================================

//      Sign inversion is actually bit-wise inversion followed by increment.
//      1) Invert sign of words until first non-0 word.
//      2) Invert (bit-wise) the successive words.
//      3) If words are over, and the high is 100..(bit-wise),
//         then add word 0.
//         If two high words of result are 11.. and 100..(bit-wise),
//         then remove the high word.
//
//      Function neither requires for normalization of operand
//      nor normalizes it, but returns normalized result if
//      operand is normalized.

size_t _CBNL_C _cBigNumberNeg (                 // Sign inversion p = -p1.
                const   CBPTR(CBNL) p1,         // Initial number.
                        EXPTR(CBNL) p           // Buffer of size *p1 + 2.
        )                                       // p1, p may overlap.
{
  size_t n1 = (size_t)(*CBPTRBASE(p1++));       // Number of words.
  p++;
  size_t n = n1;                                // Counter of words.
  if (n)
  do                                            // Cycle for sign inversion.
  {
    CBNL lp;                                    // Optimization.
    if ((*p = lp = -*p1) != 0)                  // Sign inversion of word.
    {                                           // Non-0 word obtained.
      if (--n != 0)                             // Are there any words anymore?
      {                                         // If there are:
        do                                      // Cycle of bit-wise inversion.
          { ++p1; ++p; *p = lp = ~*p1; }        // bit-wise inversion of word.
        while (--n != 0);                       // End of cycle of inversion.
        return (n1 - (lp == (p [-1] >> (BITS-1))));
      }                                         // Remove redundant high word.
                                                // If no wore words then
      if (lp != CBNL_MIN) return (n1);          // check for overflow
      p [1] = 0; return (n1 + 1);               // and add high word.
    }
  }
  while ((++p1, ++p, --n) != 0);                // End of cycle.

  return (n1);                                  // All words are 0.
}
//#define cBigNumberNeg(p1,p)   (size_t)(*(p)=_cBigNumberNeg(p1,p))

//      Increment:
//      1) Add 1 to words until first non-null result.
//      2) If no more words and the high word is 100..(bit-wise),
//         the add word 0.
//         If two high words of result are 11.. and 100..(bit-wise),
//         then remove the high word.
//
//      Function neither requires for normalization of operand
//      nor normalizes it, but returns normalized result if
//      operand is normalized.

size_t  _cBigNumberMInc (                       // Addition p1 = p1 + 1.
                        EXPTR(CBNL) p1          // Buffer of size *p1 + 2.
        )
{
  size_t n1 = (size_t)(*p1++);                  // Number of words.
  size_t n = n1;                                // Counter of words.

  if (n)                                        // There is at least one word.
  {
    do                                          // Cycle of incrementing.
    {
      CBNL lp;                                  // Optimization.
      if ((*p1 = lp = *p1 + 1) != 0)            // Increment.
      {                                         // Non 0 word obtained.
        if (--n != 0)                           // Are there any words anymore?
        {                                       // If one word is remained and
          if (--n != 0) return (n1);            // it is redundant, delete it.
          return (n1 - (p1 [1] == (lp >> (BITS-1))));
        }                                       // If no wore words then
        if (lp != CBNL_MIN) return (n1);        // check for overflow
        p1 [1] = 0; return (n1 + 1);            // and add high word.
      }
    }
    while ((++p1, --n) != 0);                   // End of cycle.
  }
  else { *p1 = 1; n1 = 1; }                     // Normalization of empty num.

  return (n1);                                  // All words are 0.
}
//#define cBigNumberMInc(p1)    (size_t)(*(p1)=_cBigNumberMInc(p1))

//      Decrement:
//      1) Subtract 1 from words until first result, different from ~0.
//      2) If no more words and the high word is 011..(bit-wise),
//         the add word ~0.
//         If two high words of result are 00.. and 011..(bit-wise),
//         then remove the high word.
//
//      Function neither requires for normalization of operand
//      nor normalizes it, but returns normalized result if
//      operand is normalized.

size_t  _cBigNumberMDec (                       // Subtraction p1 = p1 - 1.
                        EXPTR(CBNL) p1          // Buffer of size *p1 + 2.
        )
{
  size_t n1 = (size_t)(*p1++);                  // Number of words.
  size_t n = n1;                                // Counter of words.

  if (n)                                        // There is at least one word.
  {
    do                                          // Cycle of subtracting 1.
    {
      CBNL lp;                                  // Optimization.
      if ((*p1 = lp = *p1 - 1) != ~(CBNL)0)     // Subtraction 1.
      {                                         // Non ~0 word obtained.
        if (--n != 0)                           // Are there any words anymore?
        {                                       // If one word is remained and
          if (--n != 0) return (n1);            // it is redundant, delete it.
          return (n1 - (p1 [1] == (lp >> (BITS-1))));
        }                                       // If no more words then
        if (lp != CBNL_MAX) return (n1);        // check for overflow
        p1 [1] = ~(CBNL)0; return (n1 + 1);     // and add high word.
      }
    }
    while ((++p1, --n) != 0);                   // End of cycle.
  }
  else { *p1 = -1; n1 = 1; }                    // Normalization of empty num.

  return (n1);                                  // All words are ~1.
}
//#define cBigNumberMDec(p1)    (size_t)(*(p1)=_cBigNumberMDec(p1))

//      Left shift is performed successively with carry of the high bit
//      to successive word and adding of word with extension of sign bit.
//      If two highest bits of the high word are not the same then number
//      of words is increased - that is, added word becomes sign word
//
//      Function neither requires for normalization of operand
//      nor normalizes it, but returns normalized result if
//      operand is normalized.

size_t  _cBigNumberMMul2 (                      // Shift p1 <<= 1.
                        EXPTR(CBNL) p1          // Buffer of size *p1 + 2.
        )
{
  size_t n1 = (size_t)(*p1++);                  // Number of words.
  size_t n = n1;                                // Counter of words.
  CBNL num = 0;                                 // Word before shift.
  if (n)
  do                                            // Cycle of shift.
  {
    CBNL pass = (unsigned CBNL)num >> (BITS-1); // Bit from the previous word.
    num = *p1;                                  // Current word before shift.
    *p1++ = (num << 1) | pass;                  // Current word after shift.
  }
  while (--n != 0);                             // End of cycle.

  num >>= (BITS-2);                             // Extension of sign
  n1 += (num != (*p1 = num >> 1));              // on overflow.

  return (n1);                                  // Number of words.
}
//#define cBigNumberMMul2(p1)   (size_t)(*(p1)=_cBigNumberMMul2(p1))

//      Left shift with copying.

size_t  _cBigNumberMul2 (                       // Shift p = p1 << 1.
                const   CBPTR(CBNL) p1,         // Operand.
                        EXPTR(CBNL) p           // Buffer of size *p1 + 2.
        )
{
  size_t n1 = (size_t)(*CBPTRBASE(p1++));       // Number of words.
  p++;
  CBNL num = 0;                                 // Word before shift.
  size_t n = n1;                                // Counter of words.

  if (n)
  do                                            // Cycle of shift.
  {
    CBNL pass = (unsigned CBNL)num >> (BITS-1); // Bit from the previous word.
    num = *p1++;                                // Current word before shift.
    *p++ = (num << 1) | pass;                   // Current word after shift.
  }
  while (--n != 0);                             // End of cycle.

  num >>= (BITS-2);                             // Extension of sign
  n1 += (num != (*p = num >> 1));               // on overflow.

  return (n1);                                  // Number of words.
}
//#define cBigNumberMul2(p1,p)  (size_t)(*(p)=_cBigNumberMul2(p1,p))

//      Right shift is performed successively with carry of the low bit
//      to preceding word and adding of word with extension of sign bit.
//      Shift of the high word is signed then unsigned.
//      If before shift all bits if the high word are the same it is deleted.
//
//      Function neither requires for normalization of operand
//      nor normalizes it, but returns normalized result if
//      operand is normalized.

size_t  _cBigNumberMDiv2 (                      // Shift p1 <<= 1.
                        EXPTR(CBNL) p1          // Buffer of size *p1 + 1.
        )
{
  size_t n1 = (size_t)(*p1);                    // Number of words.
  size_t n = n1;                                // Counter of words.
  p1 += n;                                      // The high word.

  if (n)                                        // Is there any words?
  {
    if (--n != 0)                               // More the one word.
    {
      CBNL num = *p1;                           // The high word before shift.
      n1 -= (num == (*p1-- = (num >> 1)));      // Shift of the high word and
                                                // it's deleting of 0 or ~0
      do                                        // Cycle of shift.
      {
        CBNL pass = num << (BITS-1);            // Bit from the next word.
        num = *p1;                              // Current word before shift.
        *p1-- = ((unsigned CBNL)num >> 1) | pass;
      }                                         // Current word after shift.
      while (--n != 0);                         // End of cycle.
    }
    else                                        // Only one word.
    {
      (*p1) >>= 1;                              // Shift.
    }
  }
  return (n1);                                  // Number of words.
}
//#define cBigNumberMDiv2(p1)   (size_t)(*(p1)=_cBigNumberMDiv2(p1))

//      Right shift with copying.

size_t  _cBigNumberDiv2 (                       // Shift p = p1 << 1.
                const   CBPTR(CBNL) p1,         // Operand.
                        EXPTR(CBNL) p           // Buffer of size *p1 + 1.
        )
{
  size_t n1 = (size_t)(*CBPTRBASE(p1));         // Number of words.
  size_t n = n1;                                // Counter of words.
  p1 += n;                                      // The high word.
  p  += n;                                      // The high word.

  if (n)                                        // Is there any words?
  {
    if (--n != 0)                               // More the one word.
    {
      CBNL num = *p1--;                         // The high word before shift.
      n1 -= (num == (*p-- = (num >> 1)));       // Shift of the high word and
                                                // it's deleting of 0 or ~0
      do                                        // Cycle of shift.
      {
        CBNL pass = num << (BITS-1);            // Bit from the next word.
        num = *p1--;                            // Current word before shift.
        *p-- = ((unsigned CBNL)num >> 1) | pass;// Current word after shift.
      }
      while (--n != 0);                         // End of cycle.
    }
    else                                        // Only one word.
    {
      *p = (*p1) >> 1;                          // Shift.
    }
  }
  return (n1);                                  // Number of words.
}
//#define cBigNumberDiv2(p1,p)  (size_t)(*(p)=_cBigNumberDiv2(p1,p))

//================================================
//      Preparing of table of shifts.
//================================================

//      Number is normalized, then in the same buffer after the number
//      table of shifts by 1..BITS-1 with step *p + 2 is prepared.
//      Function always returns normalized number and normalized shifts;
//      non-involved high words is filled by extensions of sign bits.
//      Number with table of shifts can take place in optimized
//      operations that use table of shifts.

void    cBigNumberTab (                         // Preparing table of shifts.
                        EXPTR(CBNL) p           // Number in buffer of size
        )                                       // (max(*p,1) + 2) * BITS + 1.
                                                // Buffer is filled by shifts
{                                               // p << 1..BITS-1 step *p + 2.
  size_t n = cBigNumberFit (p);                 // Number of normalized words.
  int ns = BITS - 1;                            // Counter of bit shifts.
  p [n + 1] = p [n] >> (BITS-1);                // Extension of sign.
  n += 2;
  EXPTR(CBNL) pp = p;                           // Work pointer.
  do                                            // Cycle of shifts.
  {
    pp += n;
    cBigNumberMul2 (p, pp);                     // Left bit shift of number.
    p = pp;
  }
  while (--ns > 0);                             // End of cycle.
}

//================================================
//      Logical functions.
//================================================

//      Sign bit of more short number expanded to make
//      number of words in both numbers the same.
//      Then the numbers are processed word by word.
//
//      Peculiarities:
//      - The second operand must not be longer then the first one.
//      - For optimization purposes algorithmic equivalent
//        is applied instead of actual expanding of sign bit.
//
//      Functions do not require for normalization of operands and
//      normalize the result, except for case when operand contains
//      0 words - in such case the result will also contain 0 words.

size_t  _cBigNumberXor  (                       // Addition mod2 p = p1 ^ p2.
                const   CBPTR(CBNL) p1,         // Operand.
                const   CBPTR(CBNL) p2,         // Operand (no longer p1).
                        EXPTR(CBNL) p           // Buffer of size *p1 + 1.
        )                                       // p1, p2, p may overlap.
{
  size_t n1 = (size_t)(*CBPTRBASE(p1++));       // Number of words.
  size_t n2 = (size_t)(*CBPTRBASE(p2++));       // Number of words.
  p++;
  assert (n1 >= n2);
  if (n1 == 0) return 0;                        // Return if 0 words.

  {
    register CBNL lp2 = 0;                      // Current word of p2.

//      Processing of words p1, p2.

    {
      size_t n;                                 // Counter of words.
      if ((n = n2) != 0)
      {
        do { *p++ = *p1++ ^ (lp2 = *p2++); }    // Cycle on p1, p2.
        while (--n != 0);                       // End of cycle on p1, p2.
      }
    }

//      Processing of remainder p1, if p1 is longer then p2.

    {
      lp2 >>= (BITS-1);                         // Sign bit of p2.
      size_t n;                                 // Counter of words.
      if ((n = n1 - n2) != 0)
      {
        do { *p++ = *p1++ ^ lp2; }              // Cycle on p1.
        while (--n != 0);                       // End of cycle on p1.
      }
    }
    --p;                                        // Preparing of pointer.
  }

//      Normalization - deleting of redundant high words of p consisting of
//      0 or 1 and coincident with expansion of sign bit of the previous word.

  {
    register CBNL pass;                         // The high word.
    register CBNL lp;                           // The previous word.
    register size_t n = n1;                     // Number of words.
    if (n)                                      // If bits of the high word
    {                                           // and high bit of the word
      lp = *p;                                  // before high are all 0 or 1
      do continue;                              // then delete the high word.
      while (--n != 0 &&
        (pass = lp, lp = p [-1], --p, pass == (lp >> (BITS-1))));
    }                                           // If no words at all
    else { p [1] = 0; }                         // then add word 0.

    return (n + 1);                             // Number of words p.
  }
}
//#define cBigNumberXor(p1,p2,p) (size_t)(*(p)=_cBigNumberXor(p1,p2,p))

size_t  _cBigNumberAnd  (                       // Conjunction p = p1 & p2.
                const   CBPTR(CBNL) p1,         // Operand.
                const   CBPTR(CBNL) p2,         // Operand (no longer p1).
                        EXPTR(CBNL) p           // Buffer of size *p1 + 1.
        )                                       // p1, p2, p may overlap.
{
  size_t n1 = (size_t)(*CBPTRBASE(p1++));       // Number of words.
  size_t n2 = (size_t)(*CBPTRBASE(p2++));       // Number of words.
  p++;
  assert (n1 >= n2);
  if (n1 == 0) return 0;                        // Return if 0 words.

  {
    register CBNL lp2 = 0;                      // Current word of p2.

//      Processing of words p1, p2.

    {
      size_t n;                                 // Counter of words.
      if ((n = n2) != 0)
      {
        do { *p++ = *p1++ & (lp2 = *p2++); }    // Cycle on p1, p2.
        while (--n != 0);                       // End of cycle on p1, p2.
      }
    }

//      Processing of remainder p1, if p1 is longer then p2.

    {
      lp2 >>= (BITS-1);                         // Sign bit of p2.
      size_t n;                                 // Counter of words.
      if ((n = n1 - n2) != 0)
      {
        do { *p++ = *p1++ & lp2; }              // Cycle on p1.
        while (--n != 0);                       // End of cycle on p1.
      }
    }
    --p;                                        // Preparing of pointer.
  }

//      Normalization - deleting of redundant high words of p consisting of
//      0 or 1 and coincident with expansion of sign bit of the previous word.

  {
    register CBNL pass;                         // The high word.
    register CBNL lp;                           // The previous word.
    register size_t n = n1;                     // Number of words.
    if (n)                                      // If bits of the high word
    {                                           // and high bit of the word
      lp = *p;                                  // before high are all 0 or 1
      do continue;                              // then delete the high word.
      while (--n != 0 &&
        (pass = lp, lp = p [-1], --p, pass == (lp >> (BITS-1))));
    }                                           // If no words at all
    else { p [1] = 0; }                         // then add word 0.

    return (n + 1);                             // Number of words p.
  }
}
//#define cBigNumberAnd(p1,p2,p) (size_t)(*(p)=_cBigNumberAnd(p1,p2,p))

size_t  _cBigNumberOr  (                        // Disjunction p = p1 & p2.
                const   CBPTR(CBNL) p1,         // Operand.
                const   CBPTR(CBNL) p2,         // Operand (no longer p1).
                        EXPTR(CBNL) p           // Buffer of size *p1 + 1.
        )                                       // p1, p2, p may overlap.
{
  size_t n1 = (size_t)(*CBPTRBASE(p1++));       // Number of words.
  size_t n2 = (size_t)(*CBPTRBASE(p2++));       // Number of words.
  p++;
  assert (n1 >= n2);
  if (n1 == 0) return 0;                        // Return if 0 words.

  {
    register CBNL lp2 = 0;                      // Current word of p2.

//      Processing of words p1, p2.

    {
      size_t n;                                 // Counter of words.
      if ((n = n2) != 0)
      {
        do { *p++ = *p1++ | (lp2 = *p2++); }    // Cycle on p1, p2.
        while (--n != 0);                       // End of cycle on p1, p2.
      }
    }

//      Processing of remainder p1, if p1 is longer then p2.

    {
      lp2 >>= (BITS-1);                         // Sign bit of p2.
      size_t n;                                 // Counter of words.
      if ((n = n1 - n2) != 0)
      {
        do { *p++ = *p1++ | lp2; }              // Cycle on p1.
        while (--n != 0);                       // End of cycle on p1.
      }
    }
    --p;                                        // Preparing of pointer.
  }

//      Normalization - deleting of redundant high words of p consisting of
//      0 or 1 and coincident with expansion of sign bit of the previous word.

  {
    register CBNL pass;                         // The high word.
    register CBNL lp;                           // The previous word.
    register size_t n = n1;                     // Number of words.
    if (n)                                      // If bits of the high word
    {                                           // and high bit of the word
      lp = *p;                                  // before high are all 0 or 1
      do continue;                              // then delete the high word.
      while (--n != 0 &&
        (pass = lp, lp = p [-1], --p, pass == (lp >> (BITS-1))));
    }                                           // If no words at all
    else { p [1] = 0; }                         // then add word 0.

    return (n + 1);                             // Number of words p.
  }
}
//#define cBigNumberOr(p1,p2,p)  (size_t)(*(p)=_cBigNumberOr(p1,p2,p))

//================================================
//      Functions of addition and subtraction.
//================================================

//      Sign bit of more short number expanded to make
//      number of words in both numbers the same.
//      Then the numbers are added (subtracting) word by word
//      with carrying (borrowing).
//
//      Peculiarities:
//      - The second operand must not be longer then the first one.
//      - Functions always returns normalized number.
//      - For optimization purposes algorithmic equivalent
//        is applied instead of actual expanding of sign bit.
//
//      Functions do not require for normalization of operands and
//      normalize the result, except for case when operand contains
//      0 words - in such case the result will also contain 0 words.

size_t  _cBigNumberAdd  (                       // Addition p = p1 + p2.
                const   CBPTR(CBNL) p1,         // Augend.
                const   CBPTR(CBNL) p2,         // Addend (no longer p1).
                        EXPTR(CBNL) p           // Buffer of size *p1 + 2.
        )                                       // p1, p2, p may overlap.
{
  size_t n1 = (size_t)(*CBPTRBASE(p1++));       // Number of words.
  size_t n2 = (size_t)(*CBPTRBASE(p2++));       // Number of words.
  p++;
  assert (n1 >= n2);
  if (n1 == 0) return 0;                        // Return if 0 words.

  register unsigned CBNL pass = 0;              // Carry to next word of p.
        // carry is contained in the high bit of pass
        // unsigned CBNL provides for unsigned shift of pass.
  {
    register CBNL lp1 = 0;                      // Current word of p1.
    register CBNL lp2 = 0;                      // Current word of p2.

//      Addition of words p1, p2.

    {
      size_t n;                                 // Counter of words.
      if ((n = n2) != 0)
      {
        do                                      // Cycle on p1, p2.
        {
          lp1 = *p1++; lp2 = *p2++;
          pass = (~(*p++ = (pass >> (BITS-1)) + lp1 + lp2) & (lp1 ^ lp2))
                                                           | (lp1 & lp2);
        }
        while (--n != 0);                       // End of cycle on p1, p2.
      }
    }

//      Either carry or borrow over rest of p1 if p1 is longer then p2.

    {
      size_t n;                                 // Counter of words.
      if ((n = n1 - n2) != 0)
      {
        register const CBNL *pp = p1;           // Optimization.
        if ((lp2 & CBNL_MIN) == 0)              // p2 is not negative.
        {
          do                                    // Cycle on p1.
          {
            lp1 = *pp++;
            pass = ~(*p++ = (pass >> (BITS-1)) + lp1) & lp1;
          }
          while (--n != 0);                     // End of cycle on p1.
        }
        else                                    // p2 is negative.
        {
          do                                    // Cycle on p1.
          {
            lp1 = *pp++;
            pass = ~(*p++ = (pass >> (BITS-1)) + lp1 - 1) | lp1;
          }
          while (--n != 0);                     // End of cycle on p1.
        }
      }
    }
    --p;                                        // Preparing of pointer.

//      Addition of sign extensions of p1, p2.
//      This produced the high word of p consists of solely 0 or 1
//      The high word p is stored in pass.

    pass = (pass >> (BITS-1)) + ((lp1) >> (BITS-1)) + ((lp2) >> (BITS-1));
  }

//      Normalization - deleting of redundant high words of p consisting of
//      0 or 1 and coincident with expansion of sign bit of the previous word.

  {
    register CBNL lp = *p;                      // The previous word.
    register size_t n = n1;                     // Number of words - 1.
    if (n && (CBNL)pass == (lp >> (BITS-1)))    // If bits of the high word
    {                                           // and high bit of the word
      do continue;                              // before high are all 0 or 1
      while (--n != 0 &&                        // then delete the high word.
        (pass = lp, lp = p [-1], --p, (CBNL)pass == (lp >> (BITS-1))));
    }
    else { p [1] = pass; }                      // Storing of the high word.

    return (n + 1);                             // Number of words p.
  }
}
//#define cBigNumberAdd(p1,p2,p) (size_t)(*(p)=_cBigNumberAdd(p1,p2,p))

//      Subtraction is implemented in two variants:
//      conventional and with sign inversion.

size_t  _cBigNumberSub  (                       // Subtraction p = p1 - p2.
                const   CBPTR(CBNL) p1,         // Subtrahend.
                const   CBPTR(CBNL) p2,         // Subtracter (no longer p1).
                        EXPTR(CBNL) p           // Buffer of size *p1 + 2.
        )                                       // p1, p2, p may overlap.
{
  size_t n1 = (size_t)(*CBPTRBASE(p1++));       // Number of words.
  size_t n2 = (size_t)(*CBPTRBASE(p2++));       // Number of words.
  p++;
  assert (n1 >= n2);
  if (n1 == 0) return 0;                        // Return if 0 words.

  unsigned CBNL pass = ~(unsigned CBNL)0;       // Carry to next word of p.
        // carry is contained in the high bit of pass
        // unsigned CBNL provides for unsigned shift of pass.
        // initial value provides for addition of 1.
  {
    register CBNL lp1 = 0;                      // Current word of p1.
    register CBNL lp2 = ~(CBNL)0;               // Current word of ~p2.

//      Addition of words p1, ~p2.

    {
      size_t n;                                 // Counter of words.
      if ((n = n2) != 0)
      {
        do                                      // Cycle on p1, p2.
        {
          lp1 = *p1++; lp2 = ~(*p2++);
          pass = (~(*p++ = (pass >> (BITS-1)) + lp1 + lp2) & (lp1 ^ lp2))
                                                           | (lp1 & lp2);
        }
        while (--n != 0);                       // End of cycle on p1, p2.
      }
    }

//      Either carry or borrow over rest of p1 if p1 is longer then p2.

    {
      size_t n;                                 // Counter of words.
      if ((n = n1 - n2) != 0)
      {
        register const CBNL *pp = p1;           // Optimization.
        if ((lp2 & CBNL_MIN) == 0)              // p2 is negative.
        {
          do                                    // Cycle on p1.
          {
            lp1 = *pp++;
            pass = ~(*p++ = (pass >> (BITS-1)) + lp1) & lp1;
          }
          while (--n != 0);                     // End of cycle on p1.
        }
        else                                    // p2 is not negative.
        {
          do                                    // Cycle on p1.
          {
            lp1 = *pp++;
            pass = ~(*p++ = (pass >> (BITS-1)) + lp1 - 1) | lp1;
          }
          while (--n != 0);                     // End of cycle on p1.
        }
      }
    }
    --p;                                        // Preparing of pointer.

//      Addition of sign extensions of p1, p2~.
//      This produced the high word of p consists of solely 0 or 1
//      The high word p is stored in pass.

    pass = (pass >> (BITS-1)) + ((lp1) >> (BITS-1)) + ((lp2) >> (BITS-1));
  }

//      Normalization - deleting of redundant high words of p consisting of
//      0 or 1 and coincident with expansion of sign bit of the previous word.

  {
    register CBNL lp = *p;                      // The previous word.
    register size_t n = n1;                     // Number of words - 1.
    if (n && (CBNL)pass == (lp >> (BITS-1)))    // If bits of the high word
    {                                           // and high bit of the word
      do continue;                              // before high are all 0 or 1
      while (--n != 0 &&                        // then delete the high word.
        (pass = lp, lp = p [-1], --p, (CBNL)pass == (lp >> (BITS-1))));
    }
    else { p [1] = pass; }                      // Storing of the high word.

    return (n + 1);                             // Number of words p.
  }
}
//#define cBigNumberSub(p1,p2,p) (size_t)(*(p)=_cBigNumberSub(p1,p2,p))

//      Subtraction with sign inversion.

size_t  _cBigNumberSubS (                       // Subtraction p = p2 - p1.
                const   CBPTR(CBNL) p1,         // Subtracter.
                const   CBPTR(CBNL) p2,         // Subtrahend (no longer p1).
                        EXPTR(CBNL) p           // Buffer of size *p1 + 2.
        )                                       // p1, p2, p may overlap.
{
  size_t n1 = (size_t)(*CBPTRBASE(p1++));       // Number of words.
  size_t n2 = (size_t)(*CBPTRBASE(p2++));       // Number of words.
  p++;
  assert (n1 >= n2);
  if (n1 == 0) return 0;                        // Return if 0 words.

  unsigned CBNL pass = ~(unsigned CBNL)0;       // Carry to next word of p.
        // carry is contained in the high bit of pass
        // unsigned CBNL provides for unsigned shift of pass.
        // initial value provides for addition of 1.
  {
    register CBNL lp1 = ~(CBNL)0;               // Current word of ~p1.
    register CBNL lp2 = 0;                      // Current word of p2.

//      Addition of words ~p1, p2.

    {
      size_t n;                                 // Counter of words.
      if ((n = n2) != 0)
      {
        do                                      // Cycle on p1, p2.
        {
          lp1 = ~(*p1++); lp2 = *p2++;
          pass = (~(*p++ = (pass >> (BITS-1)) + lp1 + lp2) & (lp1 ^ lp2))
                                                           | (lp1 & lp2);
        }
        while (--n != 0);                       // End of cycle on p1, p2.
      }
    }

//      Either carry or borrow over rest of p1 if p1 is longer then p2.

    {
      size_t n;                                 // Counter of words.
      if ((n = n1 - n2) != 0)
      {
        register const CBNL *pp = p1;           // Optimization.
        if ((lp2 & CBNL_MIN) == 0)              // p2 is negative.
        {
          do                                    // Cycle on p1.
          {
            lp1 = ~(*pp++);
            pass = ~(*p++ = (pass >> (BITS-1)) + lp1) & lp1;
          }
          while (--n != 0);                     // End of cycle on p1.
        }
        else                                    // p2 is not negative.
        {
          do                                    // Cycle on p1.
          {
            lp1 = ~(*pp++);
            pass = ~(*p++ = (pass >> (BITS-1)) + lp1 - 1) | lp1;
          }
          while (--n != 0);                     // End of cycle on p1.
        }
      }
    }
    --p;                                        // Preparing of pointer.

//      Addition of sign extensions of ~p1, p2.
//      This produced the high word of p consists of solely 0 or 1
//      The high word p is stored in pass.

    pass = (pass >> (BITS-1)) + ((lp1) >> (BITS-1)) + ((lp2) >> (BITS-1));
  }

//      Normalization - deleting of redundant high words of p consisting of
//      0 or 1 and coincident with expansion of sign bit of the previous word.

  {
    register CBNL lp = *p;                      // The previous word.
    register size_t n = n1;                     // Number of words - 1.
    if (n && (CBNL)pass == (lp >> (BITS-1)))    // If bits of the high word
    {                                           // and high bit of the word
      do continue;                              // before high are all 0 or 1
      while (--n != 0 &&                        // then delete the high word.
        (pass = lp, lp = p [-1], --p, (CBNL)pass == (lp >> (BITS-1))));
    }
    else { p [1] = pass; }                      // Storing of the high word.

    return (n + 1);                             // Number of words p.
  }
}
//#define cBigNumberSubS(p1,p2,p) (size_t)(*(p)=_cBigNumberSubS(p1,p2,p))

//================================================
//      Special fast functions of accumulation.
//================================================

//      Static number of low 0-words + 1 (obsoleted, to be excluded).

#ifndef _CBIGNUM_MT
static  size_t  _cBigNumberSkip = 1;            // Number of low 0-words + 1.
#else //_CBIGNUM_MT
#define _cBigNumberSkip 1
#endif//_CBIGNUM_MT

//      Function to count number of low 0-words.

size_t _CBNL_C  cBigNumberSkipLow0 (            // Count low 0-words.
                const   CBPTR(CBNL) p1          // Buffer with number.
)                                               // Returns num of 0-words + 1.
{
  size_t n1 = (size_t)(*CBPTRBASE(p1));         // Number of words.
  size_t n = 1;                                 // Number of low 0-words + 1.

  if (n1 != 0)
  do
  {
    if (p1 [n] != 0) break;                     // Non-0 word is found.
    n++;
  }
  while (--n1 != 0);

#ifndef _CBIGNUM_MT
  _cBigNumberSkip = n;                          // Number of low 0-words + 1.
#endif//_CBIGNUM_MT

  return (n);                                   // Number of low 0-words + 1.
}

//      Function to count low 0-words and copy number skipping low 0-words.
//
//      Function do not requires for normalization of operand
//      but returns normalized result if operand is not 0.
//
//      If operand is 0, either normalized or denormalized, function
//      creates NOT normalized 0 containing 0 words and returns 0.

size_t _CBNL_C  cBigNumberCopySkipLow0 (        // Shift p = p1 >> exwords(p1).
                const   CBPTR(CBNL) p1,         // Number to shift.
                        EXPTR(CBNL) p           // Buffer of size *p1 + 1.
        )                                       // p1, p may overlap.
                                                // Returns num of 0-words.
{
#ifndef _CBIGNUM_MT
  _cBigNumberSkip = 1;                          // Do not skip low 0-words.
#endif//_CBIGNUM_MT

  size_t n1 = (size_t)(*CBPTRBASE(p1));         // Number of words.

//      Normalization.

  if (n1)                                       // If bits of the high word
  {                                             // and high bit of the word
    CBNL pass, lp = p1 [n1];                    // before high are all 0 or 1
    do continue;                                // then delete the high word.
    while (--n1 != 0 &&
        (pass = lp, lp = p1 [n1], pass == (lp >> (BITS-1))));
    ++n1; ++p1;

//      Skipping low 0-words and copying.

    size_t k1 = 0;                              // Number of low 0-words.
    do                                          // Skip 0-words.
    {
      if (p1 [k1] != 0)                         // Non-0 word is found.
      {
        p1 += k1;                               // Shift.
        *p++ = (CBNL)n1;                        // Number of words.
        do                                      // Cycle of copying of words.
          { *p++ = *p1++; }                     // Copying of word.
        while (--n1 != 0);                      // End of cycle.
        return (k1);                            // Number of low 0-words.
      }
      k1++;                                     // Count low 0-words.
    }
    while (--n1 != 0);
  }

//      Return NOT normalized 0 containing 0 words if all words are 0.

  *p = 0;                                       // NOT normalized 0
  return (0);                                   // containing 0 words.
}

#ifndef _CBIGNUM_ASM                            // If not in Cbignumf.cpp.

//      Special addition with left shift for multiplication.
//
//      For optimization purposes it is assumed that:
//      - Size of the first number is not smaller then size of the second
//        number after shift and not smaller then size of result.
//      - Size of the second number before shift is greater then 0.
//      Before addition one must call function cBigNumberSkipLow0()
//      to count number of low 0-words in the second operand.
//
//      Function does not require for normalization of operands, does not
//      provides for normalization of result and does not change size of
//      the first number.

void   _CBNL_C  cBigNumberMAddM (               // Addition p1 += p2<<k2*BITS.
                        EXPTR(CBNL) p1,         // Augend, then result.
                const   CBPTR(CBNL) p2,         // Addend of size <=*p1-k2.
                        size_t      k2          // Left shift of addend.
        )                                       // p1, p2 may overlap.
{
  const CBPTR(CBNL) pp = p1 + (size_t)(*p1);    // The last word of p1.
  size_t n2 = (size_t)(*CBPTRBASE(p2));         // Number of words p2.
#ifndef _CBIGNUM_MT
  assert (_cBigNumberSkip > 0);                 // Check of parameter.
#endif//_CBIGNUM_MT
  assert ((size_t)(*p1) >= n2 + k2);            // Check of size.
  assert (n2 >= _cBigNumberSkip);               // Check of size.
  p1 += k2;                                     // Scaling of p2.

  register unsigned CBNL pass = 0;              // Carry: 0 or 1.
        // unsigned CBNL provides for unsigned shift of pass.
  {
    const CBPTR(CBNL) pp = p1 + n2;             // The last word of p2.
    p1 += _cBigNumberSkip;                      // Set start of p1.
    p2 += _cBigNumberSkip;                      // Set start of p2.
    register CBNL lp1;                          // Current word of p1.
    register CBNL lp2;                          // Current word of p2.

//      Addition of words p1, p2.
//      Operators of cycle calculates the following logical expression:
//      pass = ((~(*p1 = (pass >> (BITS-1)) + lp1 + lp2) & (lp1 ^ lp2))
//                                                       | (lp1 & lp2))
//      Logical expression is listed by the following chain of assignments:
//      pass += (lp1 + lp2)     // *p1 = pass
//      lp1  = ~(lp1 ^ lp2)     // Assignment to lp1 releases one register.
//      pass = ~(pass | lp1)    // It is used identity of de Morgan.
//      pass |= (lp1 & lp2)     // and identity a & b = ~(a^b) & b.
//      Assignments are interleaved by cycle handling checks and increments
//      of pointers to optimize loading of Pentium pipes.

    for (;;)                                    // Cycle on p1, p2.
    {
      lp1 = *p1;        lp2 = *p2;              // Current words of p1, p2.
      pass += lp1;      lp1 ^= lp2;
      pass += lp2;      lp1 = ~lp1;
      *p1 = pass;       pass |= lp1;
      lp1 &= lp2;       pass = ~pass;
      ++p2;             pass |= lp1;
      if (p1 >= pp)     break;
      ++p1;             pass >>= (BITS-1);
    }
    if ((CBNL)(lp2 ^= pass) >= 0) return;       // No carry.
  }

//      Either carry or borrow over rest of p1 if p1 is longer then p2.
//      Starting condition: p1 points to last word obtained,
//                          pass contains carry in the high bit.

  {
    register CBNL lp1;                          // Current word of p1.
    if ((CBNL)pass < 0)                         // p2 is not negative.
    {
      do                                        // Cycle on p1.
      {
        if (p1 >= pp) break;
        lp1 = *++p1;
      }
      while ((~(*p1 = lp1 + 1) & lp1) < 0);
    }
    else                                        // p2 is negative.
    {
      do                                        // Cycle on p1.
      {
        if (p1 >= pp) break;
        lp1 = *++p1;
      }
      while ((~(*p1 = lp1 - 1) | lp1) >= 0);
    }
  }
}

//      Special subtraction with left shift for multiplication.
//
//      For optimization purposes it is assumed that:
//      - Size of the first number is not smaller then size of the second
//        number after shift and not smaller then size of result.
//      - Size of the second number before shift is greater then 0.
//      Before subtraction one must call function cBigNumberSkipLow0()
//      to count number of low 0-words in the second operand.
//
//      Function does not require for normalization of operands, does not
//      provides for normalization of result and does not change size of
//      the first number.

void   _CBNL_C  cBigNumberMSubM (               // Subtract p1 -= p2<<k2*BITS.
                        EXPTR(CBNL) p1,         // Subtrahend, then result.
                const   CBPTR(CBNL) p2,         // Subtracter of size <=*p1-k2.
                        size_t      k2          // Left shift of subtracter.
        )                                       // p1, p2 may overlap.
{
  const CBPTR(CBNL) pp = p1 + (size_t)(*p1);    // The last word of p1.
  size_t n2 = (size_t)(*CBPTRBASE(p2));         // Number of words p2.
#ifndef _CBIGNUM_MT
  assert (_cBigNumberSkip > 0);                 // Check of parameter.
#endif//_CBIGNUM_MT
  assert ((size_t)(*p1) >= n2 + k2);            // Check of size.
  assert (n2 >= _cBigNumberSkip);               // Check of size.
  p1 += k2;                                     // Scaling of p2.

  register CBNL pass = 0;                       // Borrow: 0 or -1.
        // CBNL provides for signed shift of pass.
  {
    const CBPTR(CBNL) pp = p1 + n2;             // The last word of p2.
    p1 += _cBigNumberSkip;                      // Set start of p1.
    p2 += _cBigNumberSkip;                      // Set start of p2.
    register CBNL lp1;                          // Current word of p1.
    register CBNL lp2;                          // Current word of p2.

//      Subtraction of words p1, p2.
//      Operators of cycle calculates the following logical expression:
//      pass = (((*p1 = (pass >> (BITS-1)) + lp1 - lp2) | (lp1 ^ lp2))
//                                                     & (~lp1 | lp2))
//      Logical expression is listed by the following chain of assignments:
//      pass += (lp1 - lp2)     // *p1 = pass
//      pass |= (lp1 ^= lp2)    // Assignment to lp1 releases one register.
//      pass = ~pass;           // It is used identity of de Morgan
//      pass |= (lp1 & ~lp2)    // and identity a & ~b = (a^b) & ~b.
//      pass = ~pass;
//      Assignments are interleaved by cycle handling checks and increments
//      of pointers to optimize loading of Pentium pipes.

    for (;;)                                    // Cycle on p1, p2.
    {
      lp1 = *p1;        lp2 = *p2;              // Current words of p1, p2.
      pass += lp1;      lp1 ^= lp2;
      pass -= lp2;      lp2 = ~lp2;
      *p1 = pass;       pass |= lp1;
      lp1 &= lp2;       pass = ~pass;
      ++p2;             pass |= lp1;
      if (p1 >= pp)     break;
      ++p1;             pass >>= (BITS-1);
                        pass = ~pass;
    }
    if ((CBNL)(lp2 ^= pass) >= 0) return;       // No carry or borrow.
  }

//      Either carry or borrow over rest of p1 if p1 is longer then p2.
//      Starting condition: p1 points to last word obtained,
//                          pass contains ~borrow in the high bit.

  {
    register CBNL lp1;                          // Current word of p1.
    if ((CBNL)pass < 0)                         // p2 is negative.
    {
      do                                        // Cycle on p1.
      {
        if (p1 >= pp) break;
        lp1 = *++p1;
      }
      while ((~(*p1 = lp1 + 1) & lp1) < 0);
    }
    else                                        // p2 is not negative.
    {
      do                                        // Cycle on p1.
      {
        if (p1 >= pp) break;
        lp1 = *++p1;
      }
      while ((~(*p1 = lp1 - 1) | lp1) >= 0);
    }
  }
}

//      Special subtraction with left shift for division.
//
//      For optimization purposes it is assumed that:
//      - Numbers have the same signs.
//      - Size of the first number is not smaller then size of the second
//        number after shift.
//      - Size of the second number before shift is greater then 0.
//      - After subtraction number of meaning bits decreases.
//      - After subtraction sign does not change except for case
//        of zeroing of negative numbers.
//      Before subtraction one must call function cBigNumberSkipLow0()
//      to count number of low 0-words in the subtracter.
//
//      Function does not require for normalization of operands and
//      always returns normalized number.

size_t _CBNL_C _cBigNumberMSubD (               // Subtract p1 -= p2<<k2*BITS.
                        EXPTR(CBNL) p1,         // Subtrahend, then result.
                const   CBPTR(CBNL) p2,         // Subtracter of size <=*p1-k2.
                        size_t      k2          // Left shift of subtracter.
        )                                       // p1, p2 may overlap.
{
  size_t n2 = (size_t)(*CBPTRBASE(p2));         // Number of words p2.
#ifndef _CBIGNUM_MT
  assert (_cBigNumberSkip > 0);                 // Check of parameter.
#endif//_CBIGNUM_MT
  assert ((p1 [(size_t)(*p1)] ^ p2 [n2]) >= 0); // Check of sign.
  assert ((size_t)(*p1) >= n2 + k2);            // Check of size.
  assert (n2 >= _cBigNumberSkip);               // Check of size.
  p1 += k2;                                     // Scaling of p2.

  register CBNL pass = 0;                       // Borrow: 0 or -1.
        // CBNL provides for signed shift of pass.
  {
    const CBPTR(CBNL) pp = p1 + n2;             // The last word of p2.
    p1 += _cBigNumberSkip;                      // Set start of p1.
    p2 += _cBigNumberSkip;                      // Set start of p2.
    register CBNL lp1;                          // Current word of p1.
    register CBNL lp2;                          // Current word of p2.

//      Subtraction of words p1, p2.
//      Operators of cycle calculates the following logical expression:
//      pass = (((*p1 = (pass >> (BITS-1)) + lp1 - lp2) | (lp1 ^ lp2))
//                                                     & (~lp1 | lp2))
//      Logical expression is listed by the following chain of assignments:
//      pass += (lp1 - lp2)     // *p1 = pass
//      pass |= (lp1 ^= lp2)    // Assignment to lp1 releases one register.
//      pass &=~(lp1 & ~lp2)    // It is used identity of de Morgan
//                              // and identity a & ~b = (a^b) & ~b.
//      Assignments are interleaved by cycle handling checks and increments
//      of pointers to optimize loading of Pentium pipes.

    for (;;)                                    // Cycle on p1, p2.
    {
      lp1 = *p1;        lp2 = *p2;              // Current words of p1, p2.
      pass += lp1;      lp1 ^= lp2;
      pass -= lp2;      lp2 = ~lp2;
      *p1 = pass;       lp2 &= lp1;
      if (p1 >= pp)     break;
      lp2 = ~lp2;       pass |= lp1;
      ++p1;             pass &= lp2;
      ++p2;             pass >>= (BITS-1);
    }
  }

//      Normalization - deleting of redundant high words of p consisting of
//      0 or 1 and coincident with expansion of sign bit of the previous word.
//      Starting condition: p1 points to last word obtained,
//                          copy of which resides in pass.

  {
    register size_t n1 = n2 + k2;

//      Loop performs the following operations:
//      do continue;
//      while (--n1 != 0 &&
//              lp1 == (lp1 = pass, pass = p1 [-1], --p1, pass >> (BITS-1)));

    for (;;)
    {
      register CBNL lp1;                        // The high word.
      register CBNL lp2;                        // The previous word.
      if (--n1 == 0)    break;                  // One word remains.
      lp2 = p1 [-1];    --p1;
      lp1 = pass;       pass = lp2;
      lp2 >>= (BITS-1);
      if (lp1 != lp2)   break;
    }
    return (n1 + 1);                            // Number of words p.
  }
}
//#define cBigNumberMSubD(p1,p2,k2) (size_t)(*(p1)=_cBigNumberMSubD(p1,p2,k2))

//      Special 1-bit left shift for multiplication.
//
//      It is assumed that the number is not 0.
//      Before shift one must call function cBigNumberSkipLow0()
//      to count number of low 0-words in the buffer.
//
//      Left shift is performed successively with carry of the high bit
//      to successive word and adding of word with extension of sign bit.
//      If two highest bits of the high word are not the same then number
//      of words is increased - that is, added word becomes sign word.
//
//      Function neither requires for normalization of operand
//      nor normalizes it, but returns normalized result if
//      operand is normalized.

size_t _CBNL_C _cBigNumberMMul2M (              // Shift p1 <<= 1.
                        EXPTR(CBNL) p1          // Buffer of size *p1 + 2.
        )
{
  size_t n1 = (size_t)(*p1);                    // Number of words.
#ifndef _CBIGNUM_MT
  assert (_cBigNumberSkip > 0);                 // Check of parameter.
#endif//_CBIGNUM_MT
  assert (n1 >= _cBigNumberSkip);               // Check of size.

  const CBPTR(CBNL) pp = p1 + n1;               // The last word of p1.
  p1 += _cBigNumberSkip;                        // First non-0 word of p1.
  CBNL num = 0;                                 // Word before shift.
  do                                            // Cycle of shift.
  {
    CBNL pass = (unsigned CBNL)num >> (BITS-1); // Bit from the previous word.
    num = *p1;                                  // Current word before shift.
    *p1++ = (num << 1) | pass;                  // Current word after shift.
  }
  while (p1 <= pp);                             // Cycle of shift.

  num >>= (BITS-2);                             // Extension of sign
  n1 += (num != (*p1 = num >> 1));              // on overflow.

  return (n1);                                  // Number of words.
}
//#define cBigNumberMMul2M(p1)   (size_t)(*(p1)=_cBigNumberMMul2M(p1))

//      Special 1-bit right shift for division and square root.
//
//      It is assumed:
//      - Number is not 0.
//      - The lower bit of the lower non-0 word is 0.
//      Before shift one must call function cBigNumberSkipLow0()
//      to count number of low 0-words in the buffer.
//
//      Right shift is performed successively with carry of the low bit
//      to preceding word and adding of word with extension of sign bit.
//      Shift of the high word is signed then unsigned.
//      If before shift all bits if the high word are the same it is deleted.
//
//      Function neither requires for normalization of operand
//      nor normalizes it, but returns normalized result if
//      operand is normalized.

size_t _CBNL_C _cBigNumberMDiv2D (              // Shift p1 >>= 1.
                        EXPTR(CBNL) p1          // Buffer of size *p1 + 1.
        )
{
  size_t n1 = (size_t)(*p1);                    // Number of words.
#ifndef _CBIGNUM_MT
  assert (_cBigNumberSkip > 0);                 // Check of parameter.
#endif//_CBIGNUM_MT
  assert (n1 >= _cBigNumberSkip);               // Check of size.
  assert ((p1 [_cBigNumberSkip] & (CBNL)1)==0); // Check of lowest bit.

  const CBPTR(CBNL) pp = p1 + _cBigNumberSkip;  // First non-0 word of p1.
  p1 += n1;                                     // The high word.
  CBNL num = *p1;                               // The high word before shift.
  n1 -= (n1 > 1) & (num == (*p1-- = num >> 1)); // Shift of the high word and
                                                // deleting, if 0 or ~0.
  while (p1 >= pp)                              // Cycle of shift.
  {
    CBNL pass = num << (BITS-1);                // Bit from the next word.
    num = *p1;                                  // Current word before shift.
    *p1-- = ((unsigned CBNL)num >> 1) | pass;   // Current word after shift.
  }                                             // End of cycle.

  return (n1);                                  // Number of words.
}
//#define cBigNumberMDiv2D(p1)   (size_t)(*(p1)=_cBigNumberMDiv2D(p1))

#endif//_CBIGNUM_ASM

//      Special 1-word right shift for multiplication.
//      Initial number must contain at least two words.

void    cBigNumberMShrM (                       // Shift p1 >>= BITS.
                        EXPTR(CBNL) p1          // Buffer of size *p1 + 1.
        )
{
  size_t n1 = (size_t)(*p1);                    // Number of words.
  assert (n1 > 1);                              // Check.
  *p1 = (CBNL)--n1;
  do                                            // Cycle of copying of words.
    { p1 [1] = p1 [2]; p1++; }                  // Copying of word.
  while ((CBNL)--n1 > 0);                       // End of cycle.
}

//      Special 1-word left shift for division.
//      Initial number must contain at least one word.

void    cBigNumberMShlD (                       // Shift p1 <<= BITS.
                        EXPTR(CBNL) p1          // Buffer of size *p1 + 2.
        )
{
  size_t n1 = (size_t)(*p1);                    // Number of words.
  assert (n1 > 0);                              // Check.
  *p1 = (CBNL)(n1 + 1);
  do                                            // Cycle of copying of words.
    { p1 [n1 + 1] = p1 [n1]; }                  // Copying of word.
  while ((CBNL)--n1 > 0);                       // End of cycle.
  p1 [1] = 0;
}

//      Functions of fast accumulation with shift optimized
//      for large numbers, in particular CBNL accumulator.
//
//      Functions does not require for normalization of operands
//      and always return normalized result.

void    cBigNumberMAddShl (                     // Addition p1 += p2<<k2*BITS.
                        EXPTR(CBNL) p1,         // Augend, then result
                                                // in buffer of size
                                                // max (*p1, *p2 + k2) + 2.
                const   CBPTR(CBNL) p2,         // Addend.
                        size_t      k2          // Left shift of addend.
        )                                       // p1, p2 may overlap.
{
  size_t n2 = (size_t)(*CBPTRBASE(p2));         // Number of words.

  cBigNumberFitTo (p1, n2 + k2 + 1);            // Denormalization.
#ifndef _CBIGNUM_MT
  _cBigNumberSkip = 1;                          // Do not skip low 0-words.
#endif//_CBIGNUM_MT
  if (n2 > 0) cBigNumberMAddM (p1, p2, k2);     // Fast addition.

  cBigNumberFit (p1);                           // Normalization.
}

void    cBigNumberMSubShl (                     // Subtract p1 -= p2<<k2*BITS.
                        EXPTR(CBNL) p1,         // Subtrahend, then result
                                                // in buffer of size
                                                // max (*p1, *p2 + k2) + 2.
                const   CBPTR(CBNL) p2,         // Subtracter.
                        size_t      k2          // Left shift of subtracter.
        )                                       // p1, p2 may overlap.
{
  size_t n2 = (size_t)(*CBPTRBASE(p2));         // Number of words.

  cBigNumberFitTo (p1, n2 + k2 + 1);            // Denormalization.
#ifndef _CBIGNUM_MT
  _cBigNumberSkip = 1;                          // Do not skip low 0-words.
#endif//_CBIGNUM_MT
  if (n2 > 0) cBigNumberMSubM (p1, p2, k2);     // Fast subtraction.

  cBigNumberFit (p1);                           // Normalization.
}

//================================================
//      Functions of multiplication.
//================================================

//      Here is portable binary implementation on standard C++.
//      Look also Cbignumf.cpp for much faster implementation
//      based on hardware multiplication.

#ifndef _CBIGNUM_HARDWARE_MUL                   // If not in Cbignumf.cpp.

//      Special functions for accumulation of multiplication with shift.
//      Multiplicand is used as work buffer and will be overwritten.
//
//      Multiplier is considered to be UNSIGNED number,
//      i.e. sign bits are interpreted as high bits.
//
//      Functions does not require for normalization of operands
//      and always return normalized result.

void    cBigNumberMAddMulShl (                  // Addition of multiplication
                                                // p += p1 * p2 << k*BITS.
                        EXPTR(CBNL) p1,         // Multiplicand in overwritten
                                                // buffer of size *p1 + 3.
                const   CBPTR(CBNL) p2,         // Unsigned multiplier.
                        size_t      k,          // Left shift of product.
                        EXPTR(CBNL) p           // Augend in buffer of size
                                                // max (*p, *p1 + *p2 + k) + 2.
        )                                       // p1, p2, p may not overlap.
{
  assert (p1 != p);                             // Check if not overlap.
  assert (p2 != p);                             // Check if not overlap.
  assert (p2 != p1);                            // Check if not overlap.
  size_t n1 = (size_t)(*p1);                    // Number of words.
  size_t n2 = (size_t)(*CBPTRBASE(p2));         // Number of words.

//      Algorithm that does not require for table of shifts.

  if (n1 >= cBigNumberSkipLow0 (p1) && n2 != 0) // Are operands not empty?
  {
    cBigNumberFitTo (p, n1 + n2 + k + 1);       // Denormalization.
    --n2;
    size_t k2 = 0;
    for (; k2 != n2; k2++)                      // Cycle on words of p2.
    {
      CBNL num = p2 [k2 + 1];                   // Current word of p2.
      if (num == 0) continue;                   // Optimization.
      CBNL mask = 1;                            // Mask for bits of num.
      for (;; cBigNumberMMul2M (p1))            // Cycle on bits of word of p2.
      {
        if (num & mask) cBigNumberMAddM (p, p1, k + k2);
        if ((mask <<= 1) == 0) break;           // No more bits.
      }                                         // End of cycle on bits of p2.
      cBigNumberMMul2M (p1);                    // Bit shift of multiplicand.
      cBigNumberMShrM (p1);                     // Reset of multiplicand.
    }                                           // End of cycle on words of p2.
    {
      CBNL num = p2 [k2 + 1];                   // The last word of p2.
      CBNL mask = (num != 0);                   // Mask for bits of num.
      if (num > 0)                              // The high bit of num
      do                                        // may be not 0.
        mask <<= 1;                             // Shift mask and num, while
      while ((num <<= 1) > 0);                  // the high bit of num is 0.
      for (;; cBigNumberMMul2M (p1))            // Cycle on bits of word of p2.
      {
        if (num & mask) cBigNumberMAddM (p, p1, k + k2);
        if ((mask <<= 1) == 0) break;           // No more bits.
      }                                         // End of cycle on bits of p2.
    }
  }

  cBigNumberFit (p);                            // Normalization.
#ifndef _CBIGNUM_MT
  _cBigNumberSkip = 1;                          // Do not skip low 0-words.
#endif//_CBIGNUM_MT
}

void    cBigNumberMSubMulShl (                  // Subtract of multiplication
                                                // p -= p1 * p2 << k*BITS.
                        EXPTR(CBNL) p1,         // Multiplicand in overwritten
                                                // buffer of size *p1 + 3.
                const   CBPTR(CBNL) p2,         // Unsigned multiplier.
                        size_t      k,          // Left shift of product.
                        EXPTR(CBNL) p           // Subtrahend in buffer of size
                                                // max (*p, *p1 + *p2 + k) + 2.
        )                                       // p1, p2, p may not overlap.
{
  assert (p1 != p);                             // Check if not overlap.
  assert (p2 != p);                             // Check if not overlap.
  assert (p2 != p1);                            // Check if not overlap.
  size_t n1 = (size_t)(*p1);                    // Number of words.
  size_t n2 = (size_t)(*CBPTRBASE(p2));         // Number of words.

//      Algorithm that does not require for table of shifts.

  if (n1 >= cBigNumberSkipLow0 (p1) && n2 != 0) // Are operands not empty?
  {
    cBigNumberFitTo (p, n1 + n2 + k + 1);       // Denormalization.
    --n2;
    size_t k2 = 0;
    for (; k2 != n2; k2++)                      // Cycle on words of p2.
    {
      CBNL num = p2 [k2 + 1];                   // Current word of p2.
      if (num == 0) continue;                   // Optimization.
      CBNL mask = 1;                            // Mask for bits of num.
      for (;; cBigNumberMMul2M (p1))            // Cycle on bits of word of p2.
      {
        if (num & mask) cBigNumberMSubM (p, p1, k + k2);
        if ((mask <<= 1) == 0) break;           // No more bits.
      }                                         // End of cycle on bits of p2.
      cBigNumberMMul2M (p1);                    // Bit shift of multiplicand.
      cBigNumberMShrM (p1);                     // Reset of multiplicand.
    }                                           // End of cycle on words of p2.
    {
      CBNL num = p2 [k2 + 1];                   // The last word of p2.
      CBNL mask = (num != 0);                   // Mask for bits of num.
      if (num > 0)                              // The high bit of num
      do                                        // may be not 0.
        mask <<= 1;                             // Shift mask and num, while
      while ((num <<= 1) > 0);                  // the high bit of num is 0.
      for (;; cBigNumberMMul2M (p1))            // Cycle on bits of word of p2.
      {
        if (num & mask) cBigNumberMSubM (p, p1, k + k2);
        if ((mask <<= 1) == 0) break;           // No more bits.
      }                                         // End of cycle on bits of p2.
    }
  }

  cBigNumberFit (p);                            // Normalization.
#ifndef _CBIGNUM_MT
  _cBigNumberSkip = 1;                          // Do not skip low 0-words.
#endif//_CBIGNUM_MT
}

#endif//_CBIGNUM_HARDWARE_MUL

//      Special functions for accumulation of multiplication with shift.
//      It is assumed that multiplicand contains table of shifts, obtained
//      by cBigNumberTab(). Method is effective if size of at lease one
//      operand is not greater then _CBNL_KARATSUBA_MIN * 2.
//      For better performance larger operand should be placed first,
//      except for case when it size is greater then _CBNL_TAB_OPT.
//
//      Multiplier is considered to be UNSIGNED number,
//      i.e. sign bits are interpreted as high bits.
//
//      Functions does not require for normalization of operands
//      and always return normalized result.

void    cBigNumberMAddMulShlTab (               // Addition of multiplication
                                                // p += p1 * p2 << k*BITS.
                const   CBPTR(CBNL) p1,         // Multiplicand, shift table.
                const   CBPTR(CBNL) p2,         // Unsigned multiplier.
                        size_t      k,          // Left shift of product.
                        EXPTR(CBNL) p           // Augend in buffer of size
                                                // max (*p, *p1 + *p2 + k) + 2.
        )                                       // p1, p2 may overlap.
{
  assert (p1 != p);                             // Check if not overlap.
  assert (p2 != p);                             // Check if not overlap.
  size_t n1 = (size_t)(*CBPTRBASE(p1));         // Number of words.
  size_t n2 = (size_t)(*CBPTRBASE(p2));         // Number of words.

//      Algorithm with table of shifts.

  if (n1 >= cBigNumberSkipLow0 (p1) && n2 != 0) // Are operands not empty?
  {
    cBigNumberFitTo (p, n1 + n2 + k + 1);       // Denormalization.
    n1 += 2;                                    // Step of table of shifts.
#ifndef _CBIGNUM_TERNARY_MUL
    for (size_t k2 = 0; k2 != n2; k2++)         // Cycle on words of p2.
    {
      CBNL num = p2 [k2 + 1];                   // Current word of p2.
      CBNL mask = 1;                            // Mask for bits of num.
      const CBPTR(CBNL) pp1 = p1;               // Current shift.
      for (;; pp1 += n1)                        // Cycle on bits of word of p2.
      {
        if (num & mask) cBigNumberMAddM (p, pp1, k + k2);
        if ((mask <<= 1) == 0) break;           // No more bits.
      }                                         // End of cycle on bits of p2.
    }                                           // End of cycle on words of p2.
#else //_CBIGNUM_TERNARY_MUL
    CBNL num2 = p2 [n2], num1 = num2;           // High bit of the high word.
    num2 <<= 1;                                 // Next bit.
    CBNL mode = 0;                              // Mode 0=add, -1=subtract.
    if ((num1 & num2) < 0)                      // Both bits are 1.
    {
      cBigNumberMAddM (p, p1, k + n2);          // Addition with shift n2.
      num1 = ~num1; num2 = ~num2;
      mode = ~(CBNL)0;                          // Switch to subtract mode.
    }
    p1 += n1 * BITS; p1 -= n1;                  // Initial shift.
    for (size_t k2 = n2; k2-- != 0;)            // Cycle on words of p2.
    {
      const CBPTR(CBNL) pp1 = p1;               // Cycle on words of p2.
      for (size_t kb = BITS;; pp1 -= n1)        // Cycle on bits of word of p2.
      {
        CBNL num = num1;                        // Current bit.
        num1 = num2;                            // 1st next bit.
        if (kb != 2) num2 <<= 1;                // 2nd next bit.
        else num2 = (k2? p2 [k2]: 0L) ^ mode;   // Load from next word.
        if ((num | (num1 & num2) ) < 0)         // Check if to accumulate.
        {
           if (mode == 0) cBigNumberMAddM (p, pp1, k + k2);
           else           cBigNumberMSubM (p, pp1, k + k2);
           if (num >= 0) { num1 = ~num1; num2 = ~num2; mode = ~mode; }
        }                                       // Switch mode.
        if (--kb == 0) break;                   // Mo more bits.
      }                                         // End of cycle on bits of p2.
    }                                           // End of cycle on words of p2.
#endif//_CBIGNUM_TERNARY_MUL
  }

  cBigNumberFit (p);                            // Normalization.
#ifndef _CBIGNUM_MT
  _cBigNumberSkip = 1;                          // Do not skip low 0-words.
#endif//_CBIGNUM_MT
}

void    cBigNumberMSubMulShlTab (               // Subtract of multiplication
                                                // p -= p1 * p2 << k*BITS.
                const   CBPTR(CBNL) p1,         // Multiplicand, shift table.
                const   CBPTR(CBNL) p2,         // Unsigned multiplier.
                        size_t      k,          // Left shift of product.
                        EXPTR(CBNL) p           // Subtrahend in buffer of size
                                                // max (*p, *p1 + *p2 + k) + 2.
        )                                       // p1, p2 may overlap.
{
  assert (p1 != p);                             // Check if not overlap.
  assert (p2 != p);                             // Check if not overlap.
  size_t n1 = (size_t)(*CBPTRBASE(p1));         // Number of words.
  size_t n2 = (size_t)(*CBPTRBASE(p2));         // Number of words.

//      Algorithm with table of shifts.

  if (n1 >= cBigNumberSkipLow0 (p1) && n2 != 0) // Are operands not empty?
  {
    cBigNumberFitTo (p, n1 + n2 + k + 1);       // Denormalization.
    n1 += 2;                                    // Step of table of shifts.
#ifndef _CBIGNUM_TERNARY_MUL
    for (size_t k2 = 0; k2 != n2; k2++)         // Cycle on words of p2.
    {
      CBNL num = p2 [k2 + 1];                   // Current word of p2.
      CBNL mask = 1;                            // Mask for bits of num.
      const CBPTR(CBNL) pp1 = p1;               // Current shift.
      for (;; pp1 += n1)                        // Cycle on bits of word of p2.
      {
        if (num & mask) cBigNumberMSubM (p, pp1, k + k2);
        if ((mask <<= 1) == 0) break;           // No more bits.
      }                                         // End of cycle on bits of p2.
    }                                           // End of cycle on words of p2.
#else //_CBIGNUM_TERNARY_MUL
    CBNL num2 = p2 [n2], num1 = num2;           // High bit of the high word.
    num2 <<= 1;                                 // Next bit.
    CBNL mode = 0;                              // Mode 0=subtract, -1=add.
    if ((num1 & num2) < 0)                      // Both bits are 1.
    {
      cBigNumberMSubM (p, p1, k + n2);          // Subtraction with shift n2.
      num1 = ~num1; num2 = ~num2;
      mode = ~(CBNL)0;                          // Switch to add mode.
    }
    p1 += n1 * BITS; p1 -= n1;                  // Initial shift.
    for (size_t k2 = n2; k2-- != 0;)            // Cycle on words of p2.
    {
      const CBPTR(CBNL) pp1 = p1;               // Cycle on words of p2.
      for (size_t kb = BITS;; pp1 -= n1)        // Cycle on bits of word of p2.
      {
        CBNL num = num1;                        // Current bit.
        num1 = num2;                            // 1st next bit.
        if (kb != 2) num2 <<= 1;                // 2nd next bit.
        else num2 = (k2? p2 [k2]: 0L) ^ mode;   // Load from next word.
        if ((num | (num1 & num2) ) < 0)         // Check if to accumulate.
        {
           if (mode == 0) cBigNumberMSubM (p, pp1, k + k2);
           else           cBigNumberMAddM (p, pp1, k + k2);
           if (num >= 0) { num1 = ~num1; num2 = ~num2; mode = ~mode; }
        }                                       // Switch mode.
        if (--kb == 0) break;                   // Mo more bits.
      }                                         // End of cycle on bits of p2.
    }                                           // End of cycle on words of p2.
#endif//_CBIGNUM_TERNARY_MUL
  }

  cBigNumberFit (p);                            // Normalization.
#ifndef _CBIGNUM_MT
  _cBigNumberSkip = 1;                          // Do not skip low 0-words.
#endif//_CBIGNUM_MT
}

#ifndef _CBIGNUM_HARDWARE_MUL                   // If not in Cbignumf.cpp.

//      Special functions for accumulation of multiplication with shift.
//      To speed up operation, functions use either Karatsuba method or
//      method of block multiplication and build temporary tables of shifts.
//
//      Multiplier is considered to be UNSIGNED number,
//      i.e. sign bits are interpreted as high bits.
//
//      Functions does not require for normalization of operands
//      and always return normalized result.

void    cBigNumberMAddMulShlKar (               // Addition of multiplication
                                                // p += p1 * p2 << k*BITS.
                        EXPTR(CBNL) p1,         // Multiplicand in overwritten
                                                // buffer of size *p1 + 3 +
                                                // *p2 < _CBNL_TAB_MIN?
                                                //  (min (*p1, _CBNL_TAB_HIGH)
                                                //   + 2) * (BITS - 1).
                        EXPTR(CBNL) p2,         // Unsigned multiplier.
                        size_t      k,          // Left shift of product.
                        EXPTR(CBNL) p           // Augend in buffer of size
                                                // max (*p, *p1 + *p2 + k + 1)
                                                //                        + 2.
        )                                       // p1, p2, p may not overlap.
{
  assert (p1 != p);                             // Check if not overlap.
  assert (p2 != p);                             // Check if not overlap.
  size_t n1 = (size_t)(*p1);                    // Number of words.
  size_t n2 = (size_t)(*p2);                    // Number of words.

//      Check if Karatsuba method is applicable.

#ifdef  _CBIGNUM_KARATSUBA_MUL
  if (n1 > _CBNL_KARATSUBA_MIN && n2 > _CBNL_KARATSUBA_MIN)
  {

//      Multiply large 1st operand by pieces.
//      NOTE: Algorithm requires to keep p2 intact in the recursive call.

    while (n1 > n2 * 2 - n2 / 4)                // Check of size.
    {
      CBNL lp = p1 [n1 -= n2];                  // Save word to be overwritten.
      p1 [n1] = (CBNL) n2;                      // Put size of piece.
      cBigNumberMAddMulShlKar (p1 + n1, p2, k + n1, p);
      p1 [n1] = lp;                             // Restore word.
      p1 [n1 + 1] = 0; n1 += (lp < 0);          // Add high 0-word.
      p1 [0] = (CBNL) n1;                       // Put size of last piece.
    }

//      Multiply operands of similar sizes by Karatsuba.

    size_t n = (n1 >= n2? n1: n2 - 1) / 2;      // Size of low part.

//      NOTE: To prevent overflow of p we must keep high part of p2
//            larger then low part of p2 for rare special case when
//            p1 is twice or more shorter then p2.

    cBigTemp c12; c12.checkexpand (n * 2 + 5);
    EXPTR(CBNL) p12 = EXPTRTYPE(c12);           // Buffer for multiplication.

    cBigTemp c1; c1.checkexpand (n + 2);
    cBigTemp c2; c2.checkexpand (n + 4);
    cBigNumberCopyShr (p2, n, EXPTRTYPE(c2));   // Get high part from p2 to c2.

//      Size of p1 may be n or smaller if p1 is twice or more shorter then p2.
//      NOTE: Algorithm requires to keep c2 intact in the recursive call.

    if (n1 > n)
    {
      cBigNumberCopyShr (p1, n, EXPTRTYPE(c1)); // Get high part from p1 to c1.
      CBNL lp = p1 [n];                         // Save word to be overwritten.
      p1 [n] = (CBNL)(n1 - n);                  // Put size of piece.
      p12 [0] = 0;                              // Clear multiply accumulator.
      cBigNumberMAddMulShlKar (p1 + n, EXPTRTYPE(c2), 0, p12);
                                                // Multiply c1 * c2.
      cBigNumberMAddShl (p, p12, k + n * 2);    // Accumulate c1 * c2.
      cBigNumberMSubShl (p, p12, k + n);        // Accumulate c1 * c2.
      p1 [n] = lp;                              // Restore word.
      p1 [n + 1] = 0;                           // Add high 0-word to p1.
      p1 [0] = (CBNL)(n + (lp < 0));            // Get low part of p1.
      cBigNumberMAddShl (EXPTRTYPE(c1), p1, 0); // Add c1 + p1.
    }
    else
      cBigNumberCopy (p1, EXPTRTYPE(c1));       // Get copy of p1 to c1.

//      Size of p2 can not be n or smaller.

    assert (n2 > n);
    {
      size_t nc2 = c2.length() + 1;             // Add high 0-word to c2.
      c2.item (0) = (CBNL) nc2;
      ((EXPTR(CBNL)) EXPTRTYPE(c2)) [nc2] = 0;

      CBNL lp = p2 [n + 1];                     // Save word to be overwritten.
      p2 [n + 1] = 0;                           // Add high 0-word to p2.
      p2 [0] = (CBNL)(n + 1);                   // Get low part of p2.
      cBigNumberMAddShl (EXPTRTYPE(c2), p2, 0); // Add c2 + p2.
      p2 [n + 1] = lp;                          // Restore word.

      p2 [0] = (CBNL) n;                        // Remove high 0-word from p2.
      p12 [0] = 0;                              // Clear multiply accumulator.
      cBigNumberMAddMulShlKar (p1, p2, 0, p12); // Multiply p1 * p2.
      p2 [0] = (CBNL) n2;                       // Restore original p2.

      c2.item (0) = (CBNL)(c2.length() - (c2.hiword() == 0));
                                                // Remove high 0-word from c2.
      cBigNumberCopy (CBPTRTYPE(c1), p1);       // Accumulate (c1+p2)*(c2+p2).
      cBigNumberMAddMulShlKar (p1, EXPTRTYPE(c2), k + n, p);

      cBigNumberMSubShl (p, p12, k + n);        // Accumulate p1 * p2.
      cBigNumberMAddShl (p, p12, k);            // Accumulate p1 * p2.
    }

    return;
  }
#endif//_CBIGNUM_KARATSUBA_MUL

//      Multiply 1st operand by pieces if 2nd operand is small.
//
//      The high piece may be either positive or negative, but lower
//      pieces are all positive, so we must add high 0-word to them.
//      The high and lower pieces contains _CBNL_TAB_OPT + 1 words,
//      including the high 0-word addendum for lower pieces, except
//      for the lowest piece that may contains up to _CBNL_TAB_HIGH
//      word with the high 0-word addendum.
//
//      Algorithm extracts pieces in place without copying.

#ifdef  _CBIGNUM_BLOCK_MUL
  if (n1 > _CBNL_TAB_HIGH)
  {
    --n1;                                       // Get high word of high piece.
    do                                          // Cycle on pieces.
    {
      n1 -= _CBNL_TAB_OPT;                      // Total size of lower pieces.
      CBNL lp = p1 [n1];                        // Save word to be overwritten.
      p1 [n1] = _CBNL_TAB_OPT + 1;              // and put size of piece.
#ifdef  _CBIGNUM_SHIFTTAB_MUL
      if (n2 >= _CBNL_TAB_MIN) {                // Use table of shifts?
        cBigNumberTab (p1 + n1);                // Prepare table of shifts.
        cBigNumberMAddMulShlTab (p1 + n1, p2, k + n1, p);
      } else
#endif//_CBIGNUM_SHIFTTAB_MUL
        cBigNumberMAddMulShl (p1 + n1, p2, k + n1, p);
      p1 [n1 + 1] = 0;                          // Add high 0-word to piece.
      p1 [n1] = lp;                             // Restore word.
    }
    while (n1 >= _CBNL_TAB_HIGH);
    p1 [0] = (CBNL)(n1 + 1);                    // Put size of lowest piece.
  }
#endif//_CBIGNUM_BLOCK_MUL

//      Multiply the lowest piece.

#ifdef  _CBIGNUM_SHIFTTAB_MUL
#ifdef  _CBIGNUM_BLOCK_MUL
  if (n2 >= _CBNL_TAB_MIN) {                    // Use table of shifts?
#else
  if (n2 >= _CBNL_TAB_MIN && n1 <= _CBNL_TAB_HIGH) {
#endif//_CBIGNUM_BLOCK_MUL
    cBigNumberTab (p1);                         // Prepare table of shifts.
    cBigNumberMAddMulShlTab (p1, p2, k, p);
  } else
#endif//_CBIGNUM_SHIFTTAB_MUL
    cBigNumberMAddMulShl (p1, p2, k, p);
}

void    cBigNumberMSubMulShlKar (               // Subtract of multiplication
                                                // p -= p1 * p2 << k*BITS.
                        EXPTR(CBNL) p1,         // Multiplicand in overwritten
                                                // buffer of size *p1 + 3 +
                                                // *p2 < _CBNL_TAB_MIN?
                                                //  (min (*p1, _CBNL_TAB_HIGH)
                                                //   + 2) * (BITS - 1).
                        EXPTR(CBNL) p2,         // Unsigned multiplier.
                        size_t      k,          // Left shift of product.
                        EXPTR(CBNL) p           // Subtrahend in buffer of size
                                                // max (*p, *p1 + *p2 + k + 1)
                                                //                        + 2.
        )                                       // p1, p2, p may not overlap.
{
  assert (p1 != p);                             // Check if not overlap.
  assert (p2 != p);                             // Check if not overlap.
  size_t n1 = (size_t)(*p1);                    // Number of words.
  size_t n2 = (size_t)(*p2);                    // Number of words.

//      Check if Karatsuba method is applicable.

#ifdef  _CBIGNUM_KARATSUBA_MUL
  if (n1 > _CBNL_KARATSUBA_MIN && n2 > _CBNL_KARATSUBA_MIN)
  {

//      Multiply large 1st operand by pieces.
//      NOTE: Algorithm requires to keep p2 intact in the recursive call.

    while (n1 > n2 * 2 - n2 / 4)                // Check of size.
    {
      CBNL lp = p1 [n1 -= n2];                  // Save word to be overwritten.
      p1 [n1] = (CBNL) n2;                      // Put size of piece.
      cBigNumberMSubMulShlKar (p1 + n1, p2, k + n1, p);
      p1 [n1] = lp;                             // Restore word.
      p1 [n1 + 1] = 0; n1 += (lp < 0);          // Add high 0-word.
      p1 [0] = (CBNL) n1;                       // Put size of last piece.
    }

//      Multiply operands of similar sizes by Karatsuba.

    size_t n = (n1 >= n2? n1: n2 - 1) / 2;      // Size of low part.

//      NOTE: To prevent overflow of p we must keep high part of p2
//            larger then low part of p2 for rare special case when
//            p1 is twice or more shorter then p2.

    cBigTemp c12; c12.checkexpand (n * 2 + 5);
    EXPTR(CBNL) p12 = EXPTRTYPE(c12);           // Buffer for multiplication.

    cBigTemp c1; c1.checkexpand (n + 2);
    cBigTemp c2; c2.checkexpand (n + 4);
    cBigNumberCopyShr (p2, n, EXPTRTYPE(c2));   // Get high part from p2 to c2.

//      Size of p1 may be n or smaller if p1 is twice or more shorter then p2.
//      NOTE: Algorithm requires to keep c2 intact in the recursive call.

    if (n1 > n)
    {
      cBigNumberCopyShr (p1, n, EXPTRTYPE(c1)); // Get high part from p1 to c1.
      CBNL lp = p1 [n];                         // Save word to be overwritten.
      p1 [n] = (CBNL)(n1 - n);                  // Put size of piece.
      p12 [0] = 0;                              // Clear multiply accumulator.
      cBigNumberMAddMulShlKar (p1 + n, EXPTRTYPE(c2), 0, p12);
                                                // Multiply c1 * c2.
      cBigNumberMSubShl (p, p12, k + n * 2);    // Accumulate c1 * c2.
      cBigNumberMAddShl (p, p12, k + n);        // Accumulate c1 * c2.
      p1 [n] = lp;                              // Restore word.
      p1 [n + 1] = 0;                           // Add high 0-word to p1.
      p1 [0] = (CBNL)(n + (lp < 0));            // Get low part of p1.
      cBigNumberMAddShl (EXPTRTYPE(c1), p1, 0); // Add c1 + p1.
    }
    else
      cBigNumberCopy (p1, EXPTRTYPE(c1));       // Get copy of p1 to c1.

//      Size of p2 can not be n or smaller.

    assert (n2 > n);
    {
      size_t nc2 = c2.length() + 1;             // Add high 0-word to c2.
      c2.item (0) = (CBNL) nc2;
      ((EXPTR(CBNL)) EXPTRTYPE(c2)) [nc2] = 0;

      CBNL lp = p2 [n + 1];                     // Save word to be overwritten.
      p2 [n + 1] = 0;                           // Add high 0-word to p2.
      p2 [0] = (CBNL)(n + 1);                   // Get low part of p2.
      cBigNumberMAddShl (EXPTRTYPE(c2), p2, 0); // Add c2 + p2.
      p2 [n + 1] = lp;                          // Restore word.

      p2 [0] = (CBNL) n;                        // Remove high 0-word from p2.
      p12 [0] = 0;                              // Clear multiply accumulator.
      cBigNumberMAddMulShlKar (p1, p2, 0, p12); // Multiply p1 * p2.
      p2 [0] = (CBNL) n2;                       // Restore original p2.

      c2.item (0) = (CBNL)(c2.length() - (c2.hiword() == 0));
                                                // Remove high 0-word from c2.
      cBigNumberCopy (CBPTRTYPE(c1), p1);       // Accumulate (c1+p2)*(c2+p2).
      cBigNumberMSubMulShlKar (p1, EXPTRTYPE(c2), k + n, p);

      cBigNumberMAddShl (p, p12, k + n);        // Accumulate p1 * p2.
      cBigNumberMSubShl (p, p12, k);            // Accumulate p1 * p2.
    }

    return;
  }
#endif//_CBIGNUM_KARATSUBA_MUL

//      Multiply 1st operand by pieces if 2nd operand is small.
//
//      The high piece may be either positive or negative, but lower
//      pieces are all positive, so we must add high 0-word to them.
//      The high and lower pieces contains _CBNL_TAB_OPT + 1 words,
//      including the high 0-word addendum for lower pieces, except
//      for the lowest piece that may contains up to _CBNL_TAB_HIGH
//      word with the high 0-word addendum.
//
//      Algorithm extracts pieces in place without copying.

#ifdef  _CBIGNUM_BLOCK_MUL
  if (n1 > _CBNL_TAB_HIGH)
  {
    --n1;                                       // Get high word of high piece.
    do                                          // Cycle on pieces.
    {
      n1 -= _CBNL_TAB_OPT;                      // Total size of lower pieces.
      CBNL lp = p1 [n1];                        // Save word to be overwritten.
      p1 [n1] = _CBNL_TAB_OPT + 1;              // and put size of piece.
#ifdef  _CBIGNUM_SHIFTTAB_MUL
      if (n2 >= _CBNL_TAB_MIN) {                // Use table of shifts?
        cBigNumberTab (p1 + n1);                // Prepare table of shifts.
        cBigNumberMSubMulShlTab (p1 + n1, p2, k + n1, p);
      } else
#endif//_CBIGNUM_SHIFTTAB_MUL
        cBigNumberMSubMulShl (p1 + n1, p2, k + n1, p);
      p1 [n1 + 1] = 0;                          // Add high 0-word to piece.
      p1 [n1] = lp;                             // Restore word.
    }
    while (n1 >= _CBNL_TAB_HIGH);
    p1 [0] = (CBNL)(n1 + 1);                    // Put size of lowest piece.
  }
#endif//_CBIGNUM_BLOCK_MUL

//      Multiply the lowest piece.

#ifdef  _CBIGNUM_SHIFTTAB_MUL
#ifdef  _CBIGNUM_BLOCK_MUL
  if (n2 >= _CBNL_TAB_MIN) {                    // Use table of shifts?
#else
  if (n2 >= _CBNL_TAB_MIN && n1 <= _CBNL_TAB_HIGH) {
#endif//_CBIGNUM_BLOCK_MUL
    cBigNumberTab (p1);                         // Prepare table of shifts.
    cBigNumberMSubMulShlTab (p1, p2, k, p);
  } else
#endif//_CBIGNUM_SHIFTTAB_MUL
    cBigNumberMSubMulShl (p1, p2, k, p);
}

//      Universal functions of multiplication work for numbers
//      of any signs. To speed up operation, functions may use
//      Karatsuba method and build temporary tables of shifts.
//
//      For better performance considerably larger operand should
//      be placed first.
//
//      Functions does not require for normalization of operands
//      and always return normalized result.

void    cBigNumberMAddMul (                     // Addition of multiplication
                                                // p += p1 * p2.
                const   CBPTR(CBNL) p1,         // Multiplicand.
                const   CBPTR(CBNL) p2,         // Multiplier.
                        EXPTR(CBNL) p           // Augend in buffer of size
                                                // max (*p, *p1 + *p2 + 1) + 2.
        )                                       // p1, p2, p may overlap.
{
  size_t n1 = (size_t)(*CBPTRBASE(p1));         // Number of words.
  size_t n2 = (size_t)(*CBPTRBASE(p2));         // Number of words.

  cBigTemp cBigBuf1;                            // Allocate temporary buffer.
  cBigTemp cBigBuf2;                            // Allocate temporary buffer.
  cBigBuf1.checkexpand ((n2 < _CBNL_TAB_MIN)? (n1 + 2):
                        (n1 < _CBNL_TAB_HIGH)? ((n1 + 3) * BITS + 1):
                          (n1 + 2 + (_CBNL_TAB_HIGH + 2) * BITS + 1));
  cBigBuf2.checkexpand (n2 + 1);                // Memory allocation.
  EXPTR(CBNL) pp1 = EXPTRTYPE(cBigBuf1);        // Buffer for multiplicand.
  EXPTR(CBNL) pp2 = EXPTRTYPE(cBigBuf2);        // Buffer for multiplier.

//      Copying and normalizing non-zero multiplicand skipping low 0-words.
//      If multiplicand is 0 result will contain 0 words.

  size_t k = cBigNumberCopySkipLow0 (p1, pp1);  // Copying of multiplicand.

  if (CBPTRBASE(p2)[n2] >= 0)                   // Not negative multiplier:
  {
    cBigNumberCopy (p2, pp2);                   // Copying.
    cBigNumberMAddMulShlKar (pp1, pp2, k, p);   // Optimized multiplication.
  }
  else                                          // Negative multiplier:
  {
    cBigNumberNeg (p2, pp2);                    // Sign inversion.
    *pp2 = (CBNL)n2;                            // Do not increase size.
    cBigNumberMSubMulShlKar (pp1, pp2, k, p);   // Optimized multiplication.
  }
}

void    cBigNumberMSubMul (                     // Subtract of multiplication
                                                // p -= p1 * p2.
                const   CBPTR(CBNL) p1,         // Multiplicand.
                const   CBPTR(CBNL) p2,         // Multiplier.
                        EXPTR(CBNL) p           // Subtrahend in buffer of size
                                                // max (*p, *p1 + *p2 + 1) + 2.
        )                                       // p1, p2, p may overlap.
{
  size_t n1 = (size_t)(*CBPTRBASE(p1));         // Number of words.
  size_t n2 = (size_t)(*CBPTRBASE(p2));         // Number of words.

  cBigTemp cBigBuf1;                            // Allocate temporary buffer.
  cBigTemp cBigBuf2;                            // Allocate temporary buffer.
  cBigBuf1.checkexpand ((n2 < _CBNL_TAB_MIN)? (n1 + 2):
                        (n1 < _CBNL_TAB_HIGH)? ((n1 + 3) * BITS + 1):
                          (n1 + 2 + (_CBNL_TAB_HIGH + 2) * BITS + 1));
  cBigBuf2.checkexpand (n2 + 1);                // Memory allocation.
  EXPTR(CBNL) pp1 = EXPTRTYPE(cBigBuf1);        // Buffer for multiplicand.
  EXPTR(CBNL) pp2 = EXPTRTYPE(cBigBuf2);        // Buffer for multiplier.

//      Copying and normalizing non-zero multiplicand skipping low 0-words.
//      If multiplicand is 0 result will contain 0 words.

  size_t k = cBigNumberCopySkipLow0 (p1, pp1);  // Copying of multiplicand.

  if (CBPTRBASE(p2)[n2] >= 0)                   // Not negative multiplier:
  {
    cBigNumberCopy (p2, pp2);                   // Copying.
    cBigNumberMSubMulShlKar (pp1, pp2, k, p);   // Optimized multiplication.
  }
  else                                          // Negative multiplier:
  {
    cBigNumberNeg (p2, pp2);                    // Sign inversion.
    *pp2 = (CBNL)n2;                            // Do not increase size.
    cBigNumberMAddMulShlKar (pp1, pp2, k, p);   // Optimized multiplication.
  }
}

void    cBigNumberMul (                         // Multiplication p = p1 * p2.
                const   CBPTR(CBNL) p1,         // Multiplicand.
                const   CBPTR(CBNL) p2,         // Multiplier.
                        EXPTR(CBNL) p           // Buffer of size
                                                // *p1 + *p2 + 3.
        )                                       // p1, p2, p may overlap.
{
  size_t n1 = (size_t)(*CBPTRBASE(p1));         // Number of words.
  size_t n2 = (size_t)(*CBPTRBASE(p2));         // Number of words.

  cBigTemp cBigBuf1;                            // Allocate temporary buffer.
  cBigTemp cBigBuf2;                            // Allocate temporary buffer.
  cBigBuf1.checkexpand ((n2 < _CBNL_TAB_MIN)? (n1 + 2):
                        (n1 < _CBNL_TAB_HIGH)? ((n1 + 3) * BITS + 1):
                          (n1 + 2 + (_CBNL_TAB_HIGH + 2) * BITS + 1));
  cBigBuf2.checkexpand (n2 + 1);                // Memory allocation.
  EXPTR(CBNL) pp1 = EXPTRTYPE(cBigBuf1);        // Buffer for multiplicand.
  EXPTR(CBNL) pp2 = EXPTRTYPE(cBigBuf2);        // Buffer for multiplier.

//      Copying and normalizing non-zero multiplicand skipping low 0-words.
//      If multiplicand is 0 result will contain 0 words.

  size_t k = cBigNumberCopySkipLow0 (p1, pp1);  // Copying of multiplicand.

//      Clearing of result is the only difference between cBigNumberMul()
//      and cBigNumberMAddMul(). We must clear the result after copying
//      of numbers because operands may overlap.

  if (CBPTRBASE(p2)[n2] >= 0)                   // Not negative multiplier:
  {
    cBigNumberCopy (p2, pp2);                   // Copying.
    p [0] = 0;                                  // Clearing of result.
    cBigNumberMAddMulShlKar (pp1, pp2, k, p);   // Optimized multiplication.
  }
  else                                          // Negative multiplier:
  {
    cBigNumberNeg (p2, pp2);                    // Sign inversion.
    *pp2 = (CBNL)n2;                            // Do not increase size.
    p [0] = 0;                                  // Clearing of result.
    cBigNumberMSubMulShlKar (pp1, pp2, k, p);   // Optimized multiplication.
  }
}

//      Function of CBNL multiplication.

void   _CBNL_C  cLongMul (                      // Multiplication l1 * l2.
                        CBNL    l1,             // Multiplicand.
                        CBNL    l2,             // Multiplier.
                        CBNL    p [2]           // Result.
        )
{
  CBNL c1 [5]; c1 [0] = 1; c1 [1] = l1;         // Buffer for multiplicand.
  CBNL c2 [3]; c2 [0] = 1; c2 [1] = l2;         // Buffer for multiplier.
  CBNL c  [5]; c [0] = c [1] = c [2] = 0;       // Buffer for result.
  EXPTR(CBNL) pp1 = EXPTRTO(CBNL,c1,sizeof(c1)/sizeof(*c1)-1);
  EXPTR(CBNL) pp2 = EXPTRTO(CBNL,c2,sizeof(c2)/sizeof(*c2)-1);
  EXPTR(CBNL) pp  = EXPTRTO(CBNL,c, sizeof(c) /sizeof(*c) -1);

  if (l2 >= 0)                                  // Not negative multiplier:
    cBigNumberMAddMulShl (pp1, pp2, 0, pp);     // Multiplication.
  else {                                        // Negative multiplier:
    c2 [1] = -l2;                               // Sign inversion.
    cBigNumberMSubMulShl (pp1, pp2, 0, pp);     // Multiplication.
  }

  p [0] = pp [1];
  p [1] = pp [2];
}

//      Function of unsigned CBNL multiplication.

void   _CBNL_C  cULongMul (                     // Multiplication l1 * l2.
                unsigned CBNL   l1,             // Unsigned multiplicand.
                unsigned CBNL   l2,             // Unsigned multiplier.
                unsigned CBNL   p [2]           // Unsigned result.
        )
{                                               // Buffer for multiplicand.
  CBNL c1 [5]; c1 [0] = 2; c1 [1] = l1; c1 [2] = 0;
  CBNL c2 [3]; c2 [0] = 1; c2 [1] = l2;         // Buffer for multiplier.
  CBNL c  [6]; c [0] = c [1] = c [2] = 0;       // Buffer for result.
  EXPTR(CBNL) pp1 = EXPTRTO(CBNL,c1,sizeof(c1)/sizeof(*c1)-1);
  EXPTR(CBNL) pp2 = EXPTRTO(CBNL,c2,sizeof(c2)/sizeof(*c2)-1);
  EXPTR(CBNL) pp  = EXPTRTO(CBNL,c, sizeof(c) /sizeof(*c) -1);

  cBigNumberMAddMulShl (pp1, pp2, 0, pp);       // Multiplication.

  p [0] = pp [1];
  p [1] = pp [2];
}

#endif//_CBIGNUM_HARDWARE_MUL

//================================================
//      Function handling division by 0.
//================================================

size_t  cBigNumberMaskDiv0;                     // Mask for division by 0:
                                                //  0 - Divide Error
void    cBigNumberDiv0()                        // >0 - Ignore
{
  cBigNumberMaskDiv0 /= cBigNumberMaskDiv0;
}

//================================================
//      Functions of integral division.
//================================================

//      Special function of division with table of shifts.
//      It is assumed, that dividend and divider have the same signs
//      except for case when dividend is negative and divider is 0.
//      Buffer of divider must contain table of shifts,
//      obtained by function cBigNumberTab.
//
//      Function gets quotient and assigns module in place of dividend.
//      Quotient >= 0. Sign of module is same as sign of dividend.
//      On division by 0, if allowed, quotient is 0 and module is
//      same as dividend.
//
//      Dividend must be normalized.
//      Function returns normalized quotient and module.

void    cBigNumberMModDivShlTab (               // Division
                                                // p = p1 / p2 << k2*BITS,
                                                //    p1 %= p2 << k2*BITS.
                        EXPTR(CBNL) p1,         // Dividend, then module.
                const   CBPTR(CBNL) p2,         // Divider and table of shifts.
                        size_t      k2,         // Left shift of divider.
                        EXPTR(CBNL) p           // Buffer of size *p1 + 2.
        )                                       // p1, p2, p may not overlap.
{
  assert (cBigNumberIsFit (p1));                // Check of normalization.
  assert (p1 != p);                             // Check if not overlap.
  assert (p2 != p);                             // Check if not overlap.
  assert (p2 != p1);                            // Check if not overlap.
  size_t n1 = (size_t)(*p1);                    // Number of words.
  size_t n2 = (size_t)(*CBPTRBASE(p2));         // Number of words.
  size_t n = n2 + k2;                           // Number of words after shift.
  if (n > n1) n = n1; n = n1 - n + 1;           // Word counter in quotient.

//      We must add 0-word to quotient for case when the high bit
//      of quotient is set to 1. If 0-word is redundant it will be
//      deleted on normalization.

  *p = (CBNL)(n + 1); p += n; p [1] = 0;        // Start filling of p.

//      Build next words.

  if (n2 >= cBigNumberSkipLow0 (p2))            // Skip low 0-words.
  {                                             // Check for division by 0.
    assert ((p1 [n1] ^ CBPTRBASE(p2)[n2]) >= 0);// Check if signs are the same.
    CBNL lt = (CBPTRBASE(p2)[n2] < 0) * 2 - 1;  // Constant to compare
                                                // absolute values of numbers.
                                                // (p1 and p2 >= 0)? -1: 1
    n2 += 2;                                    // Step of table of shifts.
    do                                          // Cycle on words of quotient.
    {
      --n;                                      // Word counter in quotient.
      p2 += n2 * BITS;                          // Word shift of divider.
      unsigned CBNL mask = (unsigned CBNL)      // Initial mask.
                        (((CBNL)1) << (BITS-1));
      *p = 0;                                   // Clearing word of quotient.
      do                                        // Cycle on bits of quotient.
      {
        p2 -= n2;                               // Current shift.
        CBNL diff = (CBNL)(n + k2) - *p1 + *p2; // Difference of sizes.
        if (diff <= 0 && (diff < 0 ||           // If remainder not less
            cBigNumberCompHigh (p1, p2) != lt)) // then shifted divider,
        {                                       // then
          cBigNumberMSubD (p1, p2, n + k2);     // subtract divider and
          (*p) += mask;                         // set bit of quotient.
        }
      }
      while ((mask >>= 1) != 0);                // End of cycle on bits.
      --p;                                      // Word of quotient is ready.
    }
    while (n);                                  // End of cycle on words.
    assert ((p1 [(size_t)(*p1)] ^ lt) < 0 ||    // Check if signs are the same
            (p1 [(size_t)(*p1)] + *p1) == 1);   // or remainder is 0.
  //assert (cBigNumberComp (p1, p2) == lt);     // Check if remainder<divider.
    assert (cBigNumberIsFit (p1));              // Check of normalization.
  }
  else                                          // Divider is 0.
  {                                             // If division by 0
    cBigNumberDiv0();                           // is enabled then module
    do { *p-- = 0; } while (--n);               // is same as dividend.
  }

  cBigNumberFit (p);                            // Normalization of quotient.
  assert (p [(size_t)(*p)] >= 0);               // Check if non-negative.
#ifndef _CBIGNUM_MT
  _cBigNumberSkip = 1;                          // Do not skip low 0-words.
#endif//_CBIGNUM_MT
}

//      Universal function of division with module works for
//      numbers of any signs and does not require for preliminary
//      prepared table of shifts. If it may speed up the operation,
//      the function builds temporary table of shifts, otherwise
//      it applies algorithm, which does not use table of shifts.
//
//      Function gets quotient and assigns module in place of dividend.
//      Sign of quotient is production of signs of dividend and divider.
//      Sign of module is same as sign of dividend.
//      On division by 0, if allowed, quotient is 0 and module is
//      same as dividend.
//
//      Dividend must be normalized.
//      Function returns normalized quotient and module.

void    cBigNumberMModDiv (                     // Division p = p1 / p2,
                                                //          p1 %= p2.
                        EXPTR(CBNL) p1,         // Dividend, then module.
                const   CBPTR(CBNL) p2,         // Divider.
                        EXPTR(CBNL) p           // Buffer of size *p1 + 3.
        )                                       // p2 may overlap with p1, p.
{
  assert (p1 != p);                             // Check if not overlap.
  size_t n1 = (size_t)(*p1);                    // Number of words.
  size_t n2 = (size_t)(*CBPTRBASE(p2));         // Number of words.

  cBigTemp cBigBuf2;                            // Allocate temporary buffer.
#ifdef  _CBIGNUM_SHIFTTAB_DIV
  CBNL nt = (CBNL)((n2 - n1 + _CBNL_TAB_MIN - 1) & (n2 - _CBNL_TAB_MAX));
  cBigBuf2.checkexpand ((size_t)((nt < 0)? ((n2 + 3) * BITS + 1): (n2 + 3)));
#else //_CBIGNUM_SHIFTTAB_DIV
  cBigBuf2.checkexpand ((size_t)(n2 + 3));
#endif//_CBIGNUM_SHIFTTAB_DIV
  EXPTR(CBNL) pp2 = EXPTRTYPE(cBigBuf2);        // Buffer for divider.

//      Copying and normalizing non-zero divider skipping low 0-words.
//      If divider is 0 result will contain 0 words.

  size_t k2 = cBigNumberCopySkipLow0 (p2, pp2); // Copying of divider.

//      Providing for same signs in dividend and divider.

  CBNL signquot = p1 [n1] ^ CBPTRBASE(p2)[n2];  // Sign of quotient.
  if (signquot < 0) cBigNumberNeg (pp2, pp2);   // Sign inversion of divider.

//      Algorithm with table of shifts.

#ifdef  _CBIGNUM_SHIFTTAB_DIV
  if (nt < 0)
  {
    cBigNumberTab (pp2);                        // Prepare table of shifts.
    cBigNumberMModDivShlTab (p1, pp2, k2, p);   // Division.
    if (signquot < 0) cBigNumberNeg (p, p);     // Sign inversion of quotient.
    return;
  }
#endif//_CBIGNUM_SHIFTTAB_DIV

//      Algorithms that do not require for table of shifts.

  n2 = (size_t)(*pp2);                          // Number of words.
  if (n2 == 0)                                  // Check for division by 0.
  {                                             // If division by 0
    cBigNumberDiv0();                           // is enabled then module
    p [0] = 1; p [1] = 0;                       // is same as dividend
    return;                                     // and quotient is 0.
  }
  assert (cBigNumberIsFit (p1));                // Check of normalization.
  assert (cBigNumberIsFit (pp2));               // Check of normalization.

  size_t n = n2 + k2;                           // Number of words after shift.

  if (n2 > 1 || n != n1)                        // Generic division.
  {
    if (n > n1)                                 // If dividend is shorter
    {                                           // the divider then module
      p [0] = 1; p [1] = 0;                     // is same as dividend
      return;                                   // and quotient is 0.
    }
    n = n1 - n + 1;                             // Word counter in quotient.

//      We must add 0-word to quotient for case when the high bit
//      of quotient is set to 1. If 0-word is redundant it will be
//      deleted on normalization.

    *p = (CBNL)(n + 1); p += n; p [1] = 0;      // Start filling of p.

//      Build next words.

    assert ((p1 [n1] ^ pp2 [n2]) >= 0);         // Check if signs are the same.
    CBNL lt = (pp2 [n2] < 0) * 2 - 1;           // Constant to compare
                                                // absolute values of numbers.
                                                // (p1 and pp2 >= 0)? -1: 1
    do                                          // Cycle on words of quotient.
    {
      --n;                                      // Word counter in quotient.
      *p = 0;                                   // Clearing word of quotient.
      cBigNumberMShlD (pp2);                    // Word shift of divider.
      unsigned CBNL mask = ((unsigned CBNL)1)   // Initial mask.
                            << (BITS-1);
      do                                        // Cycle on bits of quotient.
      {
        CBNL diff = (CBNL)cBigNumberMDiv2D (pp2)// Bit shift of divider.
                  + (CBNL)(n + k2) - *p1;       // Difference of sizes.
        if (diff <= 0 && (diff < 0 ||           // If remainder not less
            cBigNumberCompHigh (p1, pp2) != lt))// then shifted divider,
        {                                       // then
          cBigNumberMSubD (p1, pp2, n + k2);    // subtract divider and
          (*p) += mask;                         // set bit of quotient.
        }
      }
      while ((mask >>= 1) != 0);                // End of cycle on bits.
      --p;                                      // Word of quotient is ready.
    }
    while (n);                                  // End of cycle on words.
    assert ((p1 [(size_t)(*p1)] ^ lt) < 0 ||    // Check if signs are the same
            (p1 [(size_t)(*p1)] + *p1) == 1);   // or remainder is 0.
  //assert (cBigNumberComp (p1, pp2) == lt);    // Check if remainder<divider.
    assert (cBigNumberIsFit (p1));              // Check of normalization.
    cBigNumberFit (p);                          // Normalization of quotient.
  }
  else                                          // Single-word division.
  {
    CBNL p1n = p1 [n1];                         // Word of dividend.
    CBNL p2n = pp2 [n2];                        // Word of divider.
    assert ((p1n ^ p2n) >= 0);                  // Check if signs are the same.
    assert (p2n != 0);                          // Check if not 0.

#ifndef _CBIGNUM_HARDWARE_DIV
    CBNL pn = 0;                                // Clearing word of quotient.
    int nb = cLongBits (p1n) - cLongBits (p2n); // Difference in number of
    if (nb >= 0)                                // meaning bits must be >= 0.
    {
      unsigned CBNL mask = ((unsigned CBNL)1)   // Initial mask.
                            << nb;              // Align
      p2n <<= nb;                               // divider to dividend.

      if (p2n > 0)                              // Positive dividend.
      {
        for (;; p2n >>= 1)                      // Cycle on remaining bits
        {                                       // of quotient.
#ifdef  _CBIGNUM_REDUCE_JUMPS
          CBNL m = ((p1n < p2n) - 1);           // If remainder not less
          p1n -= p2n & m;                       // subtract divider and
          pn += mask & m;                       // set bit of quotient.
#else //_CBIGNUM_REDUCE_JUMPS
          if (p1n >= p2n)                       // If remainder not less
          {                                     // then shifted divider,
            p1n -= p2n;                         // subtract divider and
            pn += mask;                         // set bit of quotient.
          }
#endif//_CBIGNUM_REDUCE_JUMPS
          if ((mask >>= 1) == 0) break;
        }                                       // End of cycle on bits.
      }
      else                                      // Negative dividend.
      {
        for (;; p2n >>= 1)                      // Cycle on remaining bits
        {                                       // of quotient.
#ifdef  _CBIGNUM_REDUCE_JUMPS
          CBNL m = ((p1n > p2n) - 1);           // If remainder not less
          p1n -= p2n & m;                       // subtract divider and
          pn += mask & m;                       // set bit of quotient.
#else //_CBIGNUM_REDUCE_JUMPS
          if (p1n <= p2n)                       // If remainder not less
          {                                     // then shifted divider,
            p1n -= p2n;                         // subtract divider and
            pn += mask;                         // set bit of quotient.
          }
#endif//_CBIGNUM_REDUCE_JUMPS
          if ((mask >>= 1) == 0) break;
        }                                       // End of cycle on bits.
        if (p1n == 0)                           // Correction for case
        {                                       // when negative dividend
          for (size_t n = n1; --n > 0;)         // have non-zero low words.
          {
            if (p1 [n] != 0) { pn--; p1n += p2n; break; }
          }
        }
      }
    }
#else //_CBIGNUM_HARDWARE_DIV
    CBNL pn;
    if (p1n != CBNL_MIN || p2n != -1)            // Fix CPU crash.
    {
      pn = p1n / p2n;                            // Quotient.
      p1n -= pn * p2n;                           // Remainder.
    }
    else                                         // CBNL_MIN/-1.
    {
      pn = p1n;                                  // -Quotient.
      p1n = 0;                                   // Remainder.
    }
    if (p2n < 0 && p1n == 0)                     // Correction for case
    {                                            // when negative dividend
      for (size_t n = n1; --n > 0;)              // have non-zero low words.
      {
        if (p1 [n] != 0) { pn--; p1n += p2n; break; }
      }
    }
#endif//_CBIGNUM_HARDWARE_DIV

    p [0] = 1; p [1] = pn;                      // Store quotient.
    if (pn < 0) { p [0] = 2; p [2] = 0; }       // Correction for CBNL_MIN/-1.
    p1 [n1] = p1n;                              // Store remainder.
    cBigNumberFit (p1);                         // Normalization.
    assert ((p1 [(size_t)(*p1)] ^ pp2 [n2]) >= 0// Check if signs are the same
         || (p1 [(size_t)(*p1)] + *p1) == 1);   // or remainder is 0.
    assert (cBigNumberIsFit (p));               // Check of normalization.
  }

  assert (p [(size_t)(*p)] >= 0);               // Check if non-negative.
  if (signquot < 0) cBigNumberNeg (p, p);       // Sign inversion of quotient.
}

//================================================
//      Functions of module.
//================================================

//      Special function of module with table of shifts.
//      It is assumed, that dividend and divider have the same signs
//      except for case when dividend is negative and divider is 0.
//      Buffer of divider must contain table of shifts,
//      obtained by function cBigNumberTab.
//
//      Function of module is reduced variant of function of
//      division after exclusion of calculation of quotient.
//      Function assigns module in place of dividend.
//      Sign of module is same as sign of dividend.
//      On division by 0, if allowed, module is same as dividend.
//
//      Dividend must be normalized.
//      Function returns normalized module.

void    cBigNumberMModShlTab (                  // Module p1 %= p2 << k2*BITS.
                        EXPTR(CBNL) p1,         // Dividend, then module.
                const   CBPTR(CBNL) p2,         // Divider and table of shifts.
                        size_t      k2          // Left shift of divider.
        )                                       // p1, p2 may not overlap.
{
  assert (cBigNumberIsFit (p1));                // Check of normalization.
  assert (p1 != p2);                            // Check if not overlap.
  size_t n1 = (size_t)(*p1);                    // Number of words.
  size_t n2 = (size_t)(*CBPTRBASE(p2));         // Number of words.
  size_t n = n2 + k2;                           // Number of words after shift.
  if (n > n1) n = n1; n = n1 - n + 1;           // Word counter in quotient.

  if (n2 >= cBigNumberSkipLow0 (p2))            // Skip low 0-words.
  {                                             // Check for division by 0.
    assert ((p1 [n1] ^ CBPTRBASE(p2)[n2]) >= 0);// Check if signs are the same.
    CBNL lt = (CBPTRBASE(p2)[n2] < 0) * 2 - 1;  // Constant to compare
                                                // absolute values of numbers.
                                                // (p1 and p2 >= 0)? -1: 1
    n2 += 2;                                    // Step of table of shifts.
    do                                          // Cycle on words of quotient.
    {
      --n;                                      // Word counter in quotient.
      p2 += n2 * BITS;                          // Word shift of divider.
      unsigned CBNL mask = (unsigned CBNL)      // Initial mask.
                        (((CBNL)1) << (BITS-1));
      do                                        // Cycle on bits of quotient.
      {
        p2 -= n2;                               // Current shift.
        CBNL diff = (CBNL)(n + k2) - *p1 + *p2; // Difference of sizes.
        if (diff <= 0 && (diff < 0 ||           // If remainder not less
            cBigNumberCompHigh (p1, p2) != lt)) // then shifted divider,
        {                                       // then
          cBigNumberMSubD (p1, p2, n + k2);     // subtract divider.
        }
      }
      while ((mask >>= 1) != 0);                // End of cycle on bits.
    }
    while (n);                                  // End of cycle on words.
    assert ((p1 [(size_t)(*p1)] ^ lt) < 0 ||    // Check if signs are the same
            (p1 [(size_t)(*p1)] + *p1) == 1);   // or remainder is 0.
  //assert (cBigNumberComp (p1, p2) == lt);     // Check if remainder<divider.
    assert (cBigNumberIsFit (p1));              // Check of normalization.
  }
  else                                          // Divider is 0.
  {                                             // If division by 0
    cBigNumberDiv0();                           // is enabled then module
  }

#ifndef _CBIGNUM_MT
  _cBigNumberSkip = 1;                          // Do not skip low 0-words.
#endif//_CBIGNUM_MT
}

//      Universal function of module works for numbers
//      of any signs and does not require for preliminary
//      prepared table of shifts. If it may speed up the operation,
//      the function builds temporary table of shifts, otherwise
//      it applies algorithm, which does not use table of shifts.
//
//      Function of module is reduced variant of function of
//      division after exclusion of calculation of quotient.
//      Function assigns module in place of dividend.
//      Sign of module is same as sign of dividend.
//      On division by 0, if allowed, module is same as dividend.
//
//      Dividend must be normalized.
//      Function returns normalized module.

void    cBigNumberMMod (                        // Module p1 %= p2.
                        EXPTR(CBNL) p1,         // Dividend, then module.
                const   CBPTR(CBNL) p2          // Divider.
        )                                       // p1, p2 may overlap.
{
  size_t n1 = (size_t)(*p1);                    // Number of words.
  size_t n2 = (size_t)(*CBPTRBASE(p2));         // Number of words.

  cBigTemp cBigBuf2;                            // Allocate temporary buffer.
#ifdef  _CBIGNUM_SHIFTTAB_DIV
  CBNL nt = (CBNL)((n2 - n1 + _CBNL_TAB_MIN - 1) & (n2 - _CBNL_TAB_MAX));
  cBigBuf2.checkexpand ((size_t)((nt < 0)? ((n2 + 3) * BITS + 1): (n2 + 3)));
#else //_CBIGNUM_SHIFTTAB_DIV
  cBigBuf2.checkexpand ((size_t)(n2 + 3));
#endif//_CBIGNUM_SHIFTTAB_DIV
  EXPTR(CBNL) pp2 = EXPTRTYPE(cBigBuf2);        // Buffer for divider.

//      Copying and normalizing non-zero divider skipping low 0-words.
//      If divider is 0 result will contain 0 words.

  size_t k2 = cBigNumberCopySkipLow0 (p2, pp2); // Copying of divider.

//      Providing for same signs in dividend and divider.

  if ((p1 [n1] ^ CBPTRBASE(p2)[n2]) < 0) cBigNumberNeg (pp2, pp2);

//      Algorithm with table of shifts.

#ifdef  _CBIGNUM_SHIFTTAB_DIV
  if (nt < 0)
  {
    cBigNumberTab (pp2);                        // Prepare table of shifts.
    cBigNumberMModShlTab (p1, pp2, k2);         // Module.
    return;
  }
#endif//_CBIGNUM_SHIFTTAB_DIV

//      Algorithms that do not require for table of shifts.

  n2 = (size_t)(*pp2);                          // Number of words.
  if (n2 == 0)                                  // Check for division by 0.
  {                                             // If division by 0
    cBigNumberDiv0();                           // is enabled then module
    return;                                     // is same as dividend.
  }
  assert (cBigNumberIsFit (p1));                // Check of normalization.
  assert (cBigNumberIsFit (pp2));               // Check of normalization.

  size_t n = n2 + k2;                           // Number of words after shift.

  if (n2 > 1 || n != n1)                        // Generic division.
  {                                             // If dividend is shorter
    if (n > n1) return;                         // the divider then module
                                                // is same as dividend.
    n = n1 - n + 1;                             // Word counter in quotient.

    assert ((p1 [n1] ^ pp2 [n2]) >= 0);         // Check if signs are the same.
    CBNL lt = (pp2 [n2] < 0) * 2 - 1;           // Constant to compare
                                                // absolute values of numbers.
                                                // (p1 and pp2 >= 0)? -1: 1
    do                                          // Cycle on words of quotient.
    {
      --n;                                      // Word counter in quotient.
      cBigNumberMShlD (pp2);                    // Word shift of divider.
      unsigned CBNL mask = ((unsigned CBNL)1)   // Initial mask.
                            << (BITS-1);
      do                                        // Cycle on bits of quotient.
      {
        CBNL diff = (CBNL)cBigNumberMDiv2D (pp2)// Bit shift of divider.
                  + (CBNL)(n + k2) - *p1;       // Difference of sizes.
        if (diff <= 0 && (diff < 0 ||           // If remainder not less
            cBigNumberCompHigh (p1, pp2) != lt))// then shifted divider,
        {                                       // then
          cBigNumberMSubD (p1, pp2, n + k2);    // subtract divider.
        }
      }
      while ((mask >>= 1) != 0);                // End of cycle on bits.
    }
    while (n);                                  // End of cycle on words.
    assert ((p1 [(size_t)(*p1)] ^ lt) < 0 ||    // Check if signs are the same
            (p1 [(size_t)(*p1)] + *p1) == 1);   // or remainder is 0.
  //assert (cBigNumberComp (p1, pp2) == lt);    // Check if remainder<divider.
    assert (cBigNumberIsFit (p1));              // Check of normalization.
  }
  else                                          // Single-word division.
  {
    CBNL p1n = p1 [n1];                         // Word of dividend.
    CBNL p2n = pp2 [n2];                        // Word of divider.
    assert ((p1n ^ p2n) >= 0);                  // Check if signs are the same.
    assert (p2n != 0);                          // Check if not 0.

#ifndef _CBIGNUM_HARDWARE_DIV
    int nb = cLongBits (p1n) - cLongBits (p2n); // Difference in number of
    if (nb >= 0)                                // meaning bits must be >= 0.
    {
      unsigned CBNL mask = ((unsigned CBNL)1)   // Initial mask.
                            << nb;              // Align
      p2n <<= nb;                               // divider to dividend.

      if (p2n > 0)                              // Positive dividend.
      {
        for (;; p2n >>= 1)                      // Cycle on remaining bits
        {                                       // of quotient.
#ifdef  _CBIGNUM_REDUCE_JUMPS
          CBNL m = ((p1n < p2n) - 1);           // If remainder not less
          p1n -= p2n & m;                       // subtract divider.
#else //_CBIGNUM_REDUCE_JUMPS
          if (p1n >= p2n)                       // If remainder not less
          {                                     // then shifted divider,
            p1n -= p2n;                         // subtract divider.
          }
#endif//_CBIGNUM_REDUCE_JUMPS
          if ((mask >>= 1) == 0) break;
        }                                       // End of cycle on bits.
      }
      else                                      // Negative dividend.
      {
        for (;; p2n >>= 1)                      // Cycle on remaining bits
        {                                       // of quotient.
#ifdef  _CBIGNUM_REDUCE_JUMPS
          CBNL m = ((p1n > p2n) - 1);           // If remainder not less
          p1n -= p2n & m;                       // subtract divider.
#else //_CBIGNUM_REDUCE_JUMPS
          if (p1n <= p2n)                       // If remainder not less
          {                                     // then shifted divider,
            p1n -= p2n;                         // subtract divider.
          }
#endif//_CBIGNUM_REDUCE_JUMPS
          if ((mask >>= 1) == 0) break;
        }                                       // End of cycle on bits.
        if (p1n == 0)                           // Correction for case
        {                                       // when negative dividend
          for (size_t n = n1; --n > 0;)         // have non-zero low words.
          {
            if (p1 [n] != 0) { p1n += p2n; break; }
          }
        }
      }
    }
#else //_CBIGNUM_HARDWARE_DIV
    if (p1n != CBNL_MIN || p2n != -1)            // Fix CPU crash.
    {
      p1n = p1n % p2n;                           // Remainder.
    }
    else                                         // CBNL_MIN/-1.
    {
      p1n = 0;                                   // Remainder.
    }
    if (p2n < 0 && p1n == 0)                     // Correction for case
    {                                            // when negative dividend
      for (size_t n = n1; --n > 0;)              // have non-zero low words.
      {
        if (p1 [n] != 0) { p1n += p2n; break; }
      }
    }
#endif//_CBIGNUM_HARDWARE_DIV

    p1 [n1] = p1n;                              // Store remainder.
    cBigNumberFit (p1);                         // Normalization.
    assert ((p1 [(size_t)(*p1)] ^ pp2 [n2]) >= 0// Check if signs are the same
         || (p1 [(size_t)(*p1)] + *p1) == 1);   // or remainder is 0.
  }
}

//================================================
//      Functions of integral power.
//================================================

//      Function of power uses buffer for base as work buffer
//      and requires it to be of same size as buffer for result.
//      Both buffers must contain enough space for the result
//      and also 2 auxiliary words for multiplication purposes.
//      Note that if p2 contains more then 1 word and p1 is not one
//      of -1,0,1 then size of buffers will be greater 512 Mbytes.
//
//      Function does not require for normalization of operands
//      and always returns normalized result.

void    cBigNumberPow (                         // p1 in power p2.
                        EXPTR(CBNL) p1,         // Base in overwritten buffer
                                                // of size >= *p + 3.
                const   CBPTR(CBNL) p2,         // Degree.
                        EXPTR(CBNL) p           // Buffer for result
                                                // of size *p + 3.
        )                                       // p1, p2, p may not overlap.
{
  assert (p1 != p);                             // Check if not overlap.
  assert (p2 != p);                             // Check if not overlap.
  assert (p2 != p1);                            // Check if not overlap.
  size_t n2 = (size_t)(*CBPTRBASE(p2));         // Number of words.

  p [0] = 1; p [1] = 1;                         // Initial power is 1.

  if (CBPTRBASE(p2)[n2] < 0)                    // Check if degree is negative.
  {
    cBigNumberFit (p1);                         // Normalization.
    cBigNumberMModDiv (p, p1, p1);              // Return integer part 1/p1 or
    if (p1 [1] + (p2 [1] & 1) < 0) p1 [1] = 1;  // abs(1/p1) if degree is even.
    p [1] = p1 [1];
  }
  else if (n2 != 0)                             // Check if power may be not 1.
  {
    p2++;                                       // Skip to number.
    for (;;)                                    // Cycle on words of p2.
    {
      CBNL num = *p2++;                         // Current word of p2.
      CBNL mask = 1;                            // Mask for bits of num.
      if (--n2 == 0)                            // Reduce number of passes
      {                                         // at the last cycle.
        if (num == 0) break;                    // The high word p2 is 0.
        do                                      // The high bit is always 0.
          mask <<= 1;                           // Shift mask and num, while
        while ((num <<= 1) > 0);                // the high bit of num is 0.
      }
      for (;;)                                  // Cycle on bits of word of p2.
      {
        if (num & mask)
          cBigNumberMul (p1, p, p);             // Accumulation of power.
        if ((mask <<= 1) == 0) break;           // No more bits.
        cBigNumberMul (p1, p1, p1);             // Square of base.
      }                                         // End of cycle on bits of p2.
      if (n2 == 0) break;                       // No more words.
      cBigNumberMul (p1, p1, p1);               // Square of base.
    }                                           // End of cycle on words of p2.
  }
}

//      Function of power by module uses buffer for base as work buffer
//      and requires it to be not smaller then buffer for result.
//      Both buffers must be twice as large as buffer for module
//      plus auxiliary word for multiplication purposes.
//
//      Module must not be 0 (if module is 0 and division by 0 is
//      allowed then result = 1).
//
//      Function does not require for normalization of operands
//      and always returns normalized result.

void    cBigNumberPowMod (                      // p1 in power p2 by mod m
                        EXPTR(CBNL) p1,         // Base in overwritten buffer
                                                // of size >= *mod * 2 + 3.
                const   CBPTR(CBNL) p2,         // Degree.
                const   CBPTR(CBNL) mod,        // Module (may overlap).
                        EXPTR(CBNL) p           // Buffer for result
                                                // of size *mod * 2 + 3.
        )                                       // p1, p2, p may not overlap.
{
  assert (p1 != p);                             // Check if not overlap.
  assert (p2 != p);                             // Check if not overlap.
  assert (p2 != p1);                            // Check if not overlap.
  size_t n2 = (size_t)(*CBPTRBASE(p2));         // Number of words.

  p [0] = 1; p [1] = 1;                         // Initial power is 1.

  cBigNumberFit (p1);                           // Normalization.
  if (CBPTRBASE(p2)[n2] < 0)                    // Check if degree is negative.
  {
    cBigNumberMModDiv (p, p1, p1);              // Power is integral 1/p1 or
    if (p1 [1] + (p2 [1] & 1) < 0) p1 [1] = 1;  // abs(1/p1) if degree is even.
    p [1] = p1 [1]; cBigNumberMMod (p, mod);    // Module of power.
    return;
  }

  cBigTemp cBigBuf3;                            // Allocate temporary buffer.
  cBigBuf3.checkexpand (exmuladd (BITS, (size_t)(*CBPTRBASE(mod)),
                                  BITS * 3 + 1));
  EXPTR(CBNL) pmod = EXPTRTYPE(cBigBuf3);       // Buffer for module.

//      Copying and normalizing non-zero module skipping low 0-words.
//      If divider is 0 result will contain 0 words.

  size_t kmod = cBigNumberCopySkipLow0 (mod, pmod);

//      Checking for division by 0 and providing for positive sign of module.
  {
    size_t nmod = (size_t)(*pmod);              // Number of words.
    if (nmod == 0) n2 = 0;                      // Division by 0.
    if (pmod [nmod] < 0) cBigNumberNeg (pmod, pmod);
  }
//      Preparing of module for division with table of shifts.

  cBigNumberTab (pmod);                         // Prepare table of shifts.

//      Obtaining module of initial power (0 if module is 1, otherwise 1).
//      Also, handle division by 0 if not allowed.

  cBigNumberMModShlTab (p, pmod, kmod);         // Module of power.

  if (n2 != 0 && p [1] != 0)                    // Check if power may change.
  {
    p2++;                                       // Skip to number.

//      Make sign of base positive (for division with table of shifts).

    CBNL sign = p1 [(size_t)(*p1)];             // Sign of base in high bit.
    if (sign < 0)
    {
      cBigNumberNeg (p1, p1);                   // Sign inversion of base.
      sign = (*p2 << (BITS-1));                 // Sign of result in high bit.
    }
    cBigNumberMModShlTab (p1, pmod, kmod);      // Module of base.

//      Power by module.

    for (;;)                                    // Cycle on words of p2.
    {
      CBNL num = *p2++;                         // Current word of p2.
      CBNL mask = 1;                            // Mask for bits of num.
      if (--n2 == 0)                            // Reduce number of passes
      {                                         // at the last cycle.
        if (num == 0) break;                    // The high word p2 is 0.
        do                                      // The high bit is always 0.
          mask <<= 1;                           // Shift mask and num, while
        while ((num <<= 1) > 0);                // the high bit of num is 0.
      }
      for (;;)                                  // Cycle on bits of word of p2.
      {
        if (num & mask)
        {                                       // Accumulation of power.
          if (*p1 > *p) cBigNumberMul (p1, p, p);
          else          cBigNumberMul (p, p1, p);
          cBigNumberMModShlTab (p, pmod, kmod);// Module of power.
        }
        if ((mask <<= 1) == 0) break;           // No more bits.
        cBigNumberMul (p1, p1, p1);             // Square of base.
        cBigNumberMModShlTab (p1, pmod, kmod);  // Module of square of base.
      }                                         // End of cycle on bits of p2.
      if (n2 == 0) break;                       // No more words.
      cBigNumberMul (p1, p1, p1);               // Square of base.
      cBigNumberMModShlTab (p1, pmod, kmod);    // Module of square of base.
    }                                           // End of cycle on words of p2.
    if (sign < 0) cBigNumberNeg (p, p);         // Sign inversion.
  }
}

//================================================
//      Function of integral square root.
//================================================

//      Function calculated integral square root and assigns
//      remainder in place of base.
//      If number of bits > CBNL_MAX cBigNumberERange() is called.
//
//      Base must be normalized.
//      Function always returns normalized root and remainder.

void    cBigNumberMRmSqrt (                     // Square root.
                        EXPTR(CBNL) p1,         // Base, then remainder.
                        EXPTR(CBNL) p           // Buffer of size *p1/2 + 2.
        )                                       // p1, p may not overlap.
{
  assert (cBigNumberIsFit (p1));                // Check of normalization.
  assert (p1 != p);                             // Check if not overlap.
  size_t n1 = (size_t)(*p1);                    // Number of words.
  assert (p1 [n1] >= 0);                        // Check if non-negative.

  cBigTemp cBigBuf2;                            // Allocate temporary buffer.
  cBigBuf2.checkexpand (n1 + 1);                // +1 for debug check.
  EXPTR(CBNL) pp2 = EXPTRTYPE(cBigBuf2);        // Buffer for subtracter.
  *pp2 = 0;                                     // Initializing.
  cBigNumberFitTo (pp2, n1);                    // Filling by 0.
#ifndef _CBIGNUM_MT
  _cBigNumberSkip = 1;                          // Do not skip low 0-words.
#endif//_CBIGNUM_MT

//      Algorithm.

  CBNL nbit = (cBigNumberBits (p1) + 1) & ~1;
  if ((nbit -= 2) >= 0)
  {

//      Obtaining of the high bit of subtracter.

    {
      size_t nw = (size_t)(nbit / BITS);        // Word.
      CBNL lw = (((CBNL)1) << (nbit % BITS));   // Mask.
      pp2 [nw + 1] += lw;                       // Set bit.
      assert (pp2 [(size_t)(*pp2)] >= 0);       // Check if non-negative.
      p1  [nw + 1] -= lw;                       // NOT normalized subtraction.
      assert (p1  [(size_t)(*p1)]  >= 0);       // Check if non-negative.
    }

//      Obtaining of the other bits of subtracter.
//      Subtrahend and subtracter are not yet normalized,
//      but will be normalized during successive operations.

    while ((nbit -= 2) >= 0)                    // Cycle on lower bits.
    {
      size_t nw = (size_t)(nbit / BITS);        // Word.
      CBNL lw = (((CBNL)1) << (nbit % BITS));   // Mask.
      pp2 [nw + 1] += lw;                       // Set bit.
      if (cBigNumberComp (p1, pp2) < 0)         // Comparison.
      {
        pp2 [nw] = (CBNL)(n1 - nw);             // Will skip low 0-words.
        pp2 [nw + 1] -= lw;                     // Unset but.
        cBigNumberMDiv2D (pp2 + nw);            // Bit shift of subtracter.
      }
      else
      {
        pp2 [nw] = (CBNL)(n1 - nw);             // Will skip low 0-words.
        cBigNumberMSubD (p1, pp2 + nw, nw);     // Correction of remainder.
        assert (p1 [(size_t)(*p1)] >= 0);       // Check if non-negative.
        assert (cBigNumberIsFit (p1));          // Check of normalization.
        pp2 [nw + 1] -= lw;                     // Unset bit.
        cBigNumberMDiv2D (pp2 + nw);            // Bit shift of subtracter.
        pp2 [nw + 1] += lw;                     // Set bit.
      }
      n1 = (size_t)(pp2 [nw] + nw);             // Recover from code
      pp2 [nw] = 0;                             // written to skip
      pp2 [0] = (CBNL)n1;                       // low 0 words.
      assert (pp2 [(size_t)(*pp2)] >= 0);       // Check if non-negative.
      assert (cBigNumberIsFit (pp2));           // Check of normalization.
    }
  }

  cBigNumberFit (pp2);                          // Normalization of root.
  cBigNumberCopy (pp2, p);                      // Copying of root.

//      Check if square of root obtained is the greatest not less the base.

  assert (cBigNumberComp ((cBigNumberMMul2M (pp2), pp2), p1) >= 0);
}

//================================================
//      Random generator.
//================================================

//      Random generator uses external random function returning
//      unsigned long integers with uniform distribution within
//      the range 0..ULONG_MAX.
//
//      Generator returns uniform-distributed normalized random number,
//      containing limited number of meaning bits.

void    cBigNumberRandom (                      // Random value.
                unsigned long (*pfnRand)(),     // Random generator.
                unsigned CBNL lBits,            // Number of meaning bits.
                        EXPTR(CBNL) p           // Buffer of size lBits/BITS+1.
        )
{
  CBNL   l1 = lBits % BITS;                     // Number of bits.
  size_t n1 = (size_t)(lBits / BITS + 1);       // Number of words.

  if (l1 != 0) l1 = cLongRandom (*pfnRand) &
                    ((unsigned CBNL)~0L >> (BITS-(size_t)l1));
  p [0] = (CBNL)n1;                             // Number of words.
  p [n1] = l1;                                  // The high word.
  while (--n1 != 0) p [n1] = cLongRandom (*pfnRand);
                                                // Other words.
  cBigNumberFit (p);                            // Normalization.
  assert (p [(size_t)(*p)] >= 0);               // Check if non-negative.
}

//      Generator returns uniform-distributed normalized CBNL random number,

#if UCBNL_MAX > ULONG_MAX                       // Inline if CBNL is long.

#define LBITS   (CHAR_BIT * sizeof (long))

unsigned CBNL _CBNL_C cLongRandom (             // Random value.
                unsigned long (*pfnRand)()      // Random generator.
)                                               // Returns random value.
{
  unsigned CBNL temp = (*pfnRand)();
  int sh = LBITS; do temp |= (((CBNL)(*pfnRand)()) << LBITS); while ((sh += LBITS) < BITS);
  return (temp);
}

#endif

#ifdef  NCHECKPTR
#ifdef  __cplusplus
}
#endif//__cplusplus
#endif//NCHECKPTR

#endif//_CBIGNUMF_INL
