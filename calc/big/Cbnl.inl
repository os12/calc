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
#ifndef _CBNL_INL
#define _CBNL_INL

/*
    Visual C++ 2005 and above in 32 bit mode:
    _CBNL_MI    Use compiler intrinsics
*/
#ifdef  _MSC_VER
#if     _MSC_VER >= 1400
#ifdef  _M_IX86
#define _CBNL_MI
#endif/*_M_IX86*/
#endif/*_MSC_VER*/
#endif/*_MSC_VER*/

#endif/*_CBNL_INL*/
