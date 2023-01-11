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
 * THE ABOVE COPYRIGHT NOTICE AND THIS PERMISSION NOTICE SHALL BE INCLUDED IN ALL
 * COPIES OR SUBSTANTIAL PORTIONS OF THE SOFTWARE. WHAT'S MORE, A DECLARATION OF 
 * NGRTOS MUST BE DISPLAYED IN THE FINAL SOFTWARE OR PRODUCT RELEASE. NGRTOS HAS 
 * NOT ANY LIMITATION OF CONTRIBUTIONS TO IT, WITHOUT ANY LIMITATION OF CODING STYLE, 
 * DRIVERS, CORE, APPLICATIONS, LIBRARIES, TOOLS, AND ETC. ANY LICENSE IS PERMITTED 
 * UNDER THE ABOVE LICENSE. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF 
 * ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO 
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES 
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
 * IN THE SOFTWARE.
 *
 *************************************************************************************
 *                              https://www.ngRTOS.org
 *                              https://github.com/ngRTOS
 **************************************************************************************
 */

#ifndef __ngRTOS_DEFS_H__
#define __ngRTOS_DEFS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include "ng_arch.h"
#include "ng_func.h"

/*
 * Defines the prototype to which task functions must conform.  Defined in this
 * file to ensure the type is known before portable.h is included.
 */
typedef void (*ng_task_func_f)(void *);

typedef size_t ngrtos_size_t;
typedef uint32_t ngrtos_couter_t;
typedef void* ng_handle_t;
typedef int32_t ng_time_t;

typedef char * caddr_t;
typedef unsigned long u_int;
typedef unsigned long u_long;
typedef unsigned short u_short;
typedef unsigned char u_char;
typedef unsigned long long u_llong;

#define ng_min_t(type, a, b) ((type)(a) < (type)(b) ? (a) : (b))
#define ng_max_t(type, a, b) ((type)(a) > (type)(b) ? (a) : (b))

#define NG_DUMMY 
#define NG_TIME_MAX ((ng_time_t)(ng_INT_MAX))

#define NG_ALIGN_64BYTES_MARKER(n) char marker ## n [0]

#ifndef __ng_FallThrough
#define __ng_FallThrough
#define fallthrough
#endif
    
/* Indirect macros required for expanded argument pasting, eg. __LINE__. */
#define __ng_PASTE(a, b) a##b

#define NG_PAD_16(n) uint16_t n
#define NG_PAD_8(n)  uint8_t n

/*
 * These are non-NULL pointers that will result in page faults
 * under normal circumstances, used to verify that nobody uses
 * non-initialized list entries.
 */
#define LIST_POISON1  ((ng_list_s *) 0x00100100)
#define LIST_POISON2  ((ng_list_s *) 0x00200200)

#define NGRTOS_UNUSED(x) (void)(x)

#define NGRTOS_ERROR(x, ...)  (void)(0)
#define NGRTOS_WARN(x, ...)   (void)(0)
#define NGRTOS_EVENT(x, ...)  (void)(0)
#define NGRTOS_DEBUG(x, ...)  (void)(0)
#define NGRTOS_PRINT(x, ...)  (void)(0)

#define NG_CONCAT4(x,y,z,o) x ## y ## z ## o
#define NG_CONCAT3(x,y,z) x ## y ## z
#define NG_CONCAT2(x,y) x ## y

#define ngrtos_offsetof(type, member)	((ngrtos_size_t)&((type *)NULL)->member) 
/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:        the pointer to the member.
 * @type:       the type of the container struct this is embedded in.
 * @member:     the name of the member within the struct.
 *
 */
#define ngrtos_container_of(ptr, type, member) \
  (type *)(((ng_intptr_t)((void *)(ptr))) - ngrtos_offsetof(type,member))

#define ngrtos_difftime(a,b) (int32_t)(((uint32_t)(a))-(uint32_t)(b))
#define ngrtos_difftime_ll(a,b) (long long)(((u_llong)(a))-(u_llong)(b))
#define ngrtos_difftime_l(a,b) (long)(((u_long)(a))-(u_long)(b))

typedef ng_BASE_TYPE_T ng_bool_t;
#define ngrtos_FALSE 0
#define ngrtos_TRUE 1

#define ngrtos_READ_ONCE(x)	(x)
#define ngrtos_WRITE_ONCE(x, val)	((x)=(val))	

#define ngRTOS_BIT_SET(b,f) ((b) |= (f))
#define ngRTOS_BIT_UNSET(b,f) ((b) &= ~(f))
#define ngRTOS_BIT_TEST(b,f) (((b)&(f))==(f))

#define ngRTOS_BITS_PER_TYPE(type)	(sizeof(type) << ng_BYTE_SHIFT)

typedef enum {
  NG_result_ok,
  NG_result_inval,
  NG_result_nmem,
  NG_result_nent,
  NG_result_nsupport,
  NG_result_overflow,
  NG_result_timeout,
  NG_result_failed,
  NG_result_esys,
  NG_result_eirq,
  NG_result_ealign,
  NG_result_busy,
  NG_result_lock,
  NG_result_error,
  NG_result_nready,
  NG_result_disconnected,
} ng_result_e;

struct ngrtos_buffer{
  uint8_t *data;
  ngrtos_size_t old_pos;
  ngrtos_size_t pos;
  ngrtos_size_t size;
};
typedef struct ngrtos_buffer ngrtos_buffer_t;

static inline void 
ngrtos_buffer_init(ngrtos_buffer_t *b, uint8_t *data, 
  ngrtos_size_t size)
{
  b->data = data;
  b->size = size;
  b->old_pos = 0;
  b->pos = 0;
}

static inline void
ngrtos_assert_failed(const char *file, int line, ...)
{
  __disable_irq();
  for( ;; ){};
}

#if configENABLE_ASSERT
#define NGRTOS_ASSERT(x,...) if (!(x)) { \
  ngrtos_assert_failed(__FILE__,__LINE__, ##__VA_ARGS__); \
}
#else
#define NGRTOS_ASSERT(x,...) (void)(0)
#endif

#ifdef __cplusplus
}
#endif

#endif
