/* --------------------------------------------------------------
    Signed integers with unlimited range (version 2.0).
    Macro parameters for base functions.

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
#ifndef _CBIGNUMF_H
#define _CBIGNUMF_H
#include "Cbnl.h"

//================================================
//      Determine if assembler package is in use.
//================================================
//      _CBIGNUM_ASM            use code from Cbignumf.cpp

#ifndef _CBIGNUMF_INL
#if 1
#define _CBIGNUM_ASM
#endif
#endif//_CBIGNUMF_INL

//================================================
//      Enabling of hardware optimization.
//================================================
//      _CBIGNUM_HARDWARE_CBR   operations with carry.
//      _CBIGNUM_HARDWARE_MUL   multiplying.
//      _CBIGNUM_HARDWARE_DIV   dividing.

#ifdef  _CBIGNUM_ASM

#ifdef  _CBNL_CBR
#if 1
#define _CBIGNUM_HARDWARE_CBR
#endif
#endif//_CBNL_CBR

#ifdef  _CBNL_MUL
#if 1
#define _CBIGNUM_HARDWARE_MUL
#endif
#endif//_CBNL_MUL

#endif//_CBIGNUM_ASM

#if 0
#define _CBIGNUM_HARDWARE_DIV
#endif

//================================================
//      Enabling of automatic optimization.
//================================================
//      _CBIGNUM_KARATSUBA_MUL  use Karatsuba multiplication if
//                              both numbers have _CBNL_KARATSUBA_MIN
//                              or more words.
//
//      _CBIGNUM_BLOCK_MUL      use block multiplication if
//                              both numbers have _CBNL_TAB_HIGH
//                              or more words (block method is
//                              optimized for cache L1, now is
//                              superceded by more effective
//                              Karatsuba method, except for case
//                              when one number is shorter then
//                              _CBNL_KARATSUBA_MIN words).
//
//      _CBIGNUM_TERNARY_MUL    use 30% faster ternary method for
//                              multiplication with shift tables
//                              instead of simpler binary method
//                              (now both methods are superceded by
//                              hardware multiplication, if in use).
//
//      _CBIGNUM_SHIFTTAB_MUL   build shift tables for accelerating
//                              of multiplication, if both numbers
//                              have _CBNL_TAB_MIN or more words
//                              (now is superceded by more effective
//                              hardware multiplication, if in use).
//
//      _CBIGNUM_SHIFTTAB_DIV   build shift table for accelerating
//                              of division and module, if dividend
//                              have _CBNL_TAB_MIN or more words
//                              then divider but no more then
//                              _CBNL_TAB_MAX words.
//
//      _CBIGNUM_REDUCE_JUMPS   Use code with extra operations to
//                              reduce number of conditional jumps.
//                              No effect on popular microprocessors.

#if 1
#define _CBIGNUM_KARATSUBA_MUL
#endif

#if 1
#define _CBIGNUM_BLOCK_MUL
#endif

#if 1
#define _CBIGNUM_TERNARY_MUL
#endif

#if 1
#define _CBIGNUM_SHIFTTAB_MUL
#endif

#if 1
#define _CBIGNUM_SHIFTTAB_DIV
#endif

#if 0
#define _CBIGNUM_REDUCE_JUMPS
#endif

//================================================
//      Tuning of automatic optimization.
//================================================
//      _CBNL_TAB_MIN           minimal size of multiplier when it is
//                              worth to build table of shifts for
//                              multiplicand or
//                              minimal difference between sizes of
//                              dividend and divider when it is worth
//                              to build table of shifts for divider.
//
//      _CBNL_TAB_OPT           optimal size of piece of multiplicand
//                              for block multiplication with shift,
//                              when Karatsuba method is not applicable;
//                              it is better to take 8n - 3.
//
//      _CBNL_TAB_HIGH          maximal size of piece of multiplicand
//                              for block multiplication with shift,
//                              when Karatsuba method is not applicable.
//
//      _CBNL_TAB_MAX           maximal size of divider, for which
//                              its table of shifts may be build.
//
//      _CBNL_MUL_OPT           optimal size of piece of multiplicand
//                              for block hardware multiplication,
//                              when Karatsuba method is not applicable;
//                              it is better to take 8n - 3.
//
//      _CBNL_MUL_HIGH          maximal size of piece of multiplicand
//                              for block hardware multiplication,
//                              when Karatsuba method is not applicable.
//
//      _CBNL_KARATSUBA_MIN     minimal size of operands to implement
//                              Karatsuba multiplication.
//
//      Here size is number of CBNL C words in the code of number.

#define _CBNL_TAB_MIN   (3)
#define _CBNL_TAB_OPT   (((sizeof(size_t))<=2? 120: 7680)/\
                         (sizeof (CBNL) * sizeof (CBNL)) - 3)
#define _CBNL_TAB_HIGH  (_CBNL_TAB_OPT + _CBNL_TAB_OPT / 2)
#define _CBNL_TAB_MAX   ((EXSIZE_T_MAX & (size_t)0xFFFFFFFFUL)/\
                         (256 * sizeof (CBNL) * sizeof (CBNL)))

#ifndef _CBIGNUM_HARDWARE_MUL
#define _CBNL_KARATSUBA_MIN     (100)
#else
#define _CBNL_KARATSUBA_MIN     (50)
#define _CBNL_MUL_OPT   (7680 / sizeof (CBNL) - 3)
#define _CBNL_MUL_HIGH  (_CBNL_MUL_OPT + _CBNL_MUL_OPT / 2)
#endif//_CBIGNUM_HARDWARE_MUL

#endif//_CBIGNUMF_H
