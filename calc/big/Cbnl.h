/* --------------------------------------------------------------
    Signed integers with unlimited range (version 2.1b).
    Macro for compiler detection.

    http://www.imach.uran.ru/cbignum

    Copyright 1999-2017 by Raul N.Shakirov, IMach of RAS(UB).
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
#ifndef _CBNL_H
#define _CBNL_H

/* --------------------------------------------------------------
    Type CBNL.
-------------------------------------------------------------- */

/*
    Visual C++ in 64 bit mode (x64/Itanium).
*/
#ifndef CBNL
#ifdef  _MSC_VER
#ifdef  _WIN64
#define CBNL __int64
#define CBNL_MIN    _I64_MIN
#define CBNL_MAX    _I64_MAX
#define UCBNL_MAX   _UI64_MAX
#include "Cbnl64.inl"
#endif/*_WIN64*/
#endif/*_MSC_VER*/
#endif/*CBNL*/

/*
    Other translators.
*/
#ifndef CBNL
#define CBNL long
#define CBNL_MIN    LONG_MIN
#define CBNL_MAX    LONG_MAX
#define UCBNL_MAX   ULONG_MAX
#include "Cbnl.inl"
#endif/*CBNL*/

/* --------------------------------------------------------------
    Type CBNC for bit counting in CBNL.
-------------------------------------------------------------- */

#ifndef CBNC
#define CBNC int
#endif/*CBNC*/

/* --------------------------------------------------------------
    Calling convention for some time-critical functions.
-------------------------------------------------------------- */

/*
    Visual C++ 6.0 and above in 32 bit mode.
*/
#ifndef _CBNL_C
#ifdef  _MSC_VER
#if     _MSC_VER >= 1200
#ifdef  _M_IX86
#if 0
#define _CBNL_C     __fastcall
#endif
#endif/*_M_IX86*/
#endif/*_MSC_VER*/
#endif/*_MSC_VER*/
#endif/*_CBNL_C*/

/*
    Other translators.
*/
#ifndef _CBNL_C
#define _CBNL_C
#endif/*_CBNL_C*/

#endif/*_CBNL_H*/
