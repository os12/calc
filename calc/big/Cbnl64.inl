/* --------------------------------------------------------------
    Signed integers with unlimited range (version 2.1b).
    Macro for compiler detection in portable mode.

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
#ifndef _CBNL64_INL
#define _CBNL64_INL

/*
    Visual C++ in 64 bit mode (x64/Itanium):
    _CBNL_MI    Use compiler intrinsics
    _CBNL_MUL   Implement multiplication to double word
*/

#ifdef  _MSC_VER
#if     _MSC_VER >= 1400
#ifdef  _WIN64
#define _CBNL_MI
#define _CBNL_MUL
#endif/*_WIN64*/
#endif/*_MSC_VER*/
#endif/*_MSC_VER*/

#endif/*_CBNL64_INL*/
