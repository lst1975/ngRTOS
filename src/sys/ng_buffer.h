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

#ifndef __NGRTOS_BUFFER_H__
#define __NGRTOS_BUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ng_defs.h"
#include "mem/ng_mem.h"
#include "ng_string.h"

struct ng_buffer
{
  uint8_t *base;
  union {
    uint8_t *ptr;
    const uint8_t *cptr;
  };
  uint16_t len;
  uint16_t size;
};
typedef struct ng_buffer ng_buffer_s;

#define NG_BUF_CURC(b) (b)->ptr[(b)->len]
#define NG_BUF_END(b) &(b)->ptr[(b)->len]
#define NG_BUF_LEN(b) (b)->len

static inline void ng_buf_set(ng_buffer_s *b, uint8_t *ptr, int n)
{
  b->base = ptr; 
  b->ptr  = ptr; 
  b->len  = n;
  b->size = n;
}

static inline void ng_buf_go(ng_buffer_s *b, int n)
{
  b->ptr += n; 
  b->len -= n;
}

static inline ng_result_e
__ng_buffer_enlarge(ng_buffer_s *b, int len, int is_dup)
{
  if (len > b->size)
  {
    uint8_t *p;
    len = RTE_ALIGN(len,RTE_CACHE_LINE_SIZE,int);
    p = (uint8_t *)ng_malloc(len);
    if (p == NULL)
    {
      return NG_result_nmem;
    }
    if (b->ptr != NULL)
    {
      if (is_dup)
        ng_memcpy(p, b->ptr, b->len);
      ng_free(b->ptr);
    }
    else
    {
      b->len  = 0;
    }
    b->ptr  = p;
    b->size = len;
  }

  return NG_result_ok;
}

static inline ng_result_e
ng_buffer_enlarge(ng_buffer_s *b, int len)
{
  return __ng_buffer_enlarge(b, len, 0);
}

static inline ng_result_e
ng_buffer_enlarge_dup(ng_buffer_s *b, int len)
{
  return __ng_buffer_enlarge(b, len, 1);
}

static inline void
ng_buffer_clear(ng_buffer_s *b)
{
  b->len = 0;
}


static inline void
ng_buffer_puts(ng_buffer_s *b, const void *s, int len)
{
  memcpy(&b->ptr[b->len], s, len);
  b->len += len;
}

static inline void
ng_buffer_putb(ng_buffer_s *b, const ng_buffer_s *s)
{
  memcpy(&b->ptr[b->len], s->ptr, s->len);
  b->len += s->len;
}

static inline char *
ng_buf_strchr (const ng_buffer_s *b, char c)
{
  ng_string_s s = { .ptr = (char *)b->cptr, .len = b->len };
  return ng_strchr(&s, c);
}

static inline char *
ng_buf_strrchr (const ng_buffer_s *b, char c)
{
  ng_string_s s = { .ptr = (char *)b->ptr, .len = b->len };
  return ng_strrchr(&s, c);
}

#ifdef __cplusplus
}
#endif

#endif /* __NGRTOS_BUFFER_H__ */
