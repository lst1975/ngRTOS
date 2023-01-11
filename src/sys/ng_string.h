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

#ifndef __ngRTOS_STRING_H__
#define __ngRTOS_STRING_H__

#include "ng_defs.h"
#include "ng_align.h"

struct ng_string
{
  const char *ptr;
  uint16_t len;
  uint16_t pad;
};
typedef struct ng_string ng_string_s;
#define __DFB(p, s) {.ptr = p, .len = s }
#define __DFS(p)    {.ptr = p, .len = sizeof(p) - 1 }

int ng_atoi (const char *ptr, int len);
int ng_str_atoi (const char *ptr);
uint32_t ng_fast_atoui32(const char *str, uint8_t len);
uint64_t ng_fast_atoui64(const char *str, uint8_t len);
uint32_t ng_fast_strtoui32(const char *s);
uint64_t ng_fast_strtoui64(const char *s);

char *ng_strcpy (char *dst, const char *src);
char *ng_strncpy (char *dst, const char *src, size_t n);

ngrtos_size_t ng_strlen(const char *s);
int ng_strncmp (const char *s1, const char *s2, ngrtos_size_t n);
int ng_strcmp(const char *l, const char *r);

char *ng_strchr (const ng_string_s *s, char c);
char *ng_strrchr (const ng_string_s *s, char c);
char *ng_strstr(const char *h, const char *n);

#define NG_BUFFER_CMP(a,b) ((a)->len == (b)->len && !ng_memcmp((a),(b),(a)->len))

#endif
