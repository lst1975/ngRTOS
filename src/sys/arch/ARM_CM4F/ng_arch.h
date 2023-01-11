/*************************************************************************************
 *                               ngRTOS Kernel V2.0.1
 * Copyright (C) 2022 Songtao Liu, 980680431@qq.com.  All Rights Reserved.
 **************************************************************************************
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *************************************************************************************
 *                              https://www.ngRTOS.org
 *                              https://github.com/ngRTOS
 **************************************************************************************
 */

#ifndef __NGRTOS_ARCH_H__
#define __NGRTOS_ARCH_H__

#ifdef __cplusplus
    extern "C" {
#endif

#if !defined( __GNUC__ )
/* IAR includes. */
#include <intrinsics.h>
#endif

#include <string.h>
#include <stdint.h>
      
#include "cmsis_compiler.h"          
#include "ng_config.h"
#include "stm32f4xx.h"

#define __NG_BITS_PER_LONG      32
#define __NG_BITS_PER_LONG_LONG 64
#define __NG_CTX_SWITCH_WAIT    0

#if __NG_BITS_PER_LONG_LONG == 32
#define __PrxULL "lu"
#elif __NG_BITS_PER_LONG_LONG == 64
#define __PrxULL "llu"
#endif

#define __NG_FLT_EVAL_METHOD 0 /* All operations are performed in own type */
#define __NG_DBL_MANT_DIG    53
#define __NG_DBL_DIG         15
#define __NG_DBL_MIN_EXP     -1021
#define __NG_DBL_MIN_10_EXP  -307
#define __NG_DBL_MAX_EXP     1024
#define __NG_DBL_MAX_10_EXP  308
#define __NG_DBL_MAX         1.7976931348623157e308  /* 0x1.FFFFFFFFFFFFFp1023 */
#define __NG_DBL_EPSILON     2.2204460492503131e-16  /* 0x1.0p-52 */
#define __NG_DBL_MIN         2.2250738585072014e-308 /* 0x1.0p-1022 */
#define __NG_DBL_DECIMAL_DIG 15

#define __NG_LDBL_MANT_DIG    __NG_DBL_MANT_DIG
#define __NG_LDBL_DIG         __NG_DBL_DIG
#define __NG_LDBL_MIN_EXP     __NG_DBL_MIN_EXP
#define __NG_LDBL_MIN_10_EXP  __NG_DBL_MIN_10_EXP
#define __NG_LDBL_MAX_EXP     __NG_DBL_MAX_EXP
#define __NG_LDBL_MAX_10_EXP  __NG_DBL_MAX_10_EXP
#define __NG_LDBL_MAX         NG_CONCAT2(__NG_DBL_MAX, l)
#define __NG_LDBL_EPSILON     NG_CONCAT2(__NG_DBL_EPSILON, l)
#define __NG_LDBL_MIN         NG_CONCAT2(__NG_DBL_MIN, l)
#define __NG_LDBL_DECIMAL_DIG __NG_DBL_DIG

#define __NG_LITTLE_ENDIAN 1234
#define __NG_BIG_ENDIAN    4321
#define __NG_BYTE_ORDER    __NG_LITTLE_ENDIAN

#define __NG_PAGE_SIZE     0xFFFFFFFFUL

#define __ng_NOP                       __ng_NOP
#define __ng_INLINE                    __INLINE
#define __ng_FORCE_INLINE              __FORCEINLINE
#define __ng_STATIC_FORCEINLINE        __STATIC_FORCEINLINE
#define __ng_USED                      __USED
#define __ng_WEAK                      __WEAK
#define __ng_PACKED                    __PACKED
#define __ng_PACKED_STRUCT             __PACKED_STRUCT

#define __ng_ALIGNED(x)               __ALIGNED(x)

/* Type definitions. */
#define ng_CHAR_T          char
#define ng_FLOAT_T         float
#define ng_DOUBLE_T        double
#define ng_LONG_T          long
#define ng_SHORT_T         short
#define ng_STACK_TYPE_T    uint32_t
#define ng_BASE_TYPE_T     long

#define ng_BYTE_SHIFT      3
#define ng_BITS_PER_LONG   32
#define ng_BIT_WORD(nr)		((nr) >> 5)
#define ng_BIT_MASK(nr)		(1UL << ((nr) % ng_BITS_PER_LONG))

typedef uint32_t ng_ptrdiff_t;
typedef int32_t  ng_intmax_t;
typedef uint32_t ng_intptr_t;

typedef unsigned ng_BASE_TYPE_T   ngBaseTypeT;
typedef unsigned ng_BASE_TYPE_T   ngAtomTypeT;
typedef   uint32_t                ngIrqTypeT;
typedef   ng_STACK_TYPE_T         ngStackTypeT;

#if (configUSE_16_BIT_TICKS == 1)
  typedef uint16_t                ngTickTypeT;
  #define ng_MAX_DELAY            ((ngTickTypeT)0xffff)
  #define ng_TICK_TYPE_IS_ATOMIC  0
#else
  typedef uint32_t                ngTickTypeT;
  #define portMAX_DELAY           ((ngTickTypeT)0xffffffffUL)
  /* 32-bit tick type on a 32-bit architecture, so reads of the tick count do
   * not need to be guarded with a critical section. */
  #define ng_TICK_TYPE_IS_ATOMIC  1
#endif

#define __ngRTOS_STACK_DECREASE          1

#ifdef __cplusplus
    }
#endif

#endif /* __NGRTOS_ARCH_H__ */
