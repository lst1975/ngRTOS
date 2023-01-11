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

#include <stdlib.h>
#include "ng_defs.h"
#include "ng_ctype.h"
#include "ng_utils.h"
#include "ng_limits.h"

/*
 * Copyright (c) 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *        may be used to endorse or promote products derived from this software
 *        without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.      IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
ng_result_e
ng_strtoull(const char *nptr, char **endptr, int base, uint64_t *ret)
{
  uint64_t        result = 0;
  const char     *p;
  const char     *first_nonspace;
  const char     *digits_start;
  int             sign = 1;
  int             out_of_range = 0;

  if (base != 0 && (base < 2 || base > 36))
    goto invalid_input;

  p = nptr;

  /*
   * Process the initial, possibly empty, sequence of white-space characters.
   */
  while (isspace((unsigned char) (*p)))
    p++;

  first_nonspace = p;

  /*
   * Determine sign.
   */
  if (*p == '+')
    p++;
  else if (*p == '-') 
  {
    p++;
    sign = -1;
  }

  if (base == 0) {
    /*
     * Determine base.
     */
    if (*p == '0') 
    {
      if ((p[1] == 'x' || p[1] == 'X')) 
      {
        if (isxdigit((unsigned char)(p[2]))) 
        {
          base = 16;
          p += 2;
        } 
        else 
        {
          /*
           * Special case: treat the string "0x" without any further
           * hex digits as a decimal number.
           */
          base = 10;
        }
      } 
      else 
      {
        base = 8;
        p++;
      }
    } 
    else 
    {
      base = 10;
    }
  } 
  else if (base == 16) 
  {
    /*
     * For base 16, skip the optional "0x" / "0X" prefix.
     */
    if (*p == '0' && (p[1] == 'x' || p[1] == 'X')
      && isxdigit((unsigned char)(p[2]))) 
    {
      p += 2;
    }
  }

  digits_start = p;

  for (; *p; p++) 
  {
    int             digit;
    digit = ('0' <= *p && *p <= '9') ? *p - '0'
        : ('a' <= *p && *p <= 'z') ? (*p - 'a' + 10)
        : ('A' <= *p && *p <= 'Z') ? (*p - 'A' + 10) : 36;
    if (digit < base) 
    {
      if (! out_of_range) 
      {
        if (result > ng_ULLONG_MAX / base
          || result * base > ng_ULLONG_MAX - digit) 
        {
          out_of_range = 1;
        }
        result = result * base + digit;
      }
    } 
    else
      break;
  }

  if (p > first_nonspace && p == digits_start)
    goto invalid_input;

  if (p == first_nonspace)
    p = nptr;

  if (endptr)
    *endptr = (char *) p;

  if (out_of_range) 
  {
    *ret = ng_ULLONG_MAX;
    return NG_result_overflow;
  }

  *ret = sign > 0 ? result : -result;
  return NG_result_ok;

invalid_input:
  if (endptr)
      *endptr = (char *) nptr;
  return NG_result_inval;
}

/*
 * Copyright (c) 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *        may be used to endorse or promote products derived from this software
 *        without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.      IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Convert a string to an unsigned long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
ng_result_e 
ng_strtoul(const char *nptr, char **endptr, int base, unsigned long *ret)
{
  ng_result_e     r = NG_result_ok;
  const char     *s = nptr;
  unsigned long   acc;
  unsigned char   c;
  unsigned long   cutoff;
  int             neg = 0, any, cutlim;

  /*
   * See strtol for comments as to the logic used.
   */
  do 
  {
    c = *s++;
  } 
  while (isspace(c));

  if (c == '-') 
  {
    neg = 1;
    c = *s++;
  } 
  else if (c == '+')
    c = *s++;
  if ((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X')) 
  {
    c = s[1];
    s += 2;
    base = 16;
  }
  if (base == 0)
    base = c == '0' ? 8 : 10;
  cutoff = (unsigned long) ng_ULONG_MAX / (unsigned long) base;
  cutlim = (unsigned long) ng_ULONG_MAX % (unsigned long) base;
  for (acc = 0, any = 0;; c = *s++) 
  {
    if (!isascii(c))
      break;
    if (isdigit(c))
      c -= '0';
    else if (isalpha(c))
      c -= isupper(c) ? 'A' - 10 : 'a' - 10;
    else
      break;
    if (c >= base)
      break;
    if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
      any = -1;
    else 
    {
      any = 1;
      acc *= base;
      acc += c;
    }
  }
  if (any < 0) 
  {
    acc = ng_ULONG_MAX;
    r = NG_result_overflow;
  }
  else if (neg)
    acc = -acc;
  if (endptr != 0)
    *endptr = (char *) (any ? s - 1 : nptr);
  *ret = (acc);
  return r;
}

/**
 * Converts a numeric string to the equivalent uint64_t value.
 * As well as straight number conversion, also recognises the suffixes
 * k, m and g for kilobytes, megabytes and gigabytes respectively.
 *
 * If a negative number is passed in  i.e. a string with the first non-black
 * character being "-", zero is returned. Zero is also returned in the case of
 * an error with the strtoull call in the function.
 *
 * @param str
 *     String containing number to convert.
 * @return
 *     Number.
 */
ng_result_e
rte_str_to_size(const char *str, uint64_t *ret)
{
  char *endptr;
  ng_result_e r;

  while (isspace((int)*str))
    str++;
  if (*str == '-')
    return NG_result_inval;

  r = ng_strtoull(str, &endptr, 0, ret);
  if (r != NG_result_ok)
    return r;

  if (*endptr == ' ')
    endptr++; /* allow 1 space gap */

  switch (*endptr){
  case 'G': case 'g': *ret *= 1024; /* fall-through */
  case 'M': case 'm': *ret *= 1024; /* fall-through */
  case 'K': case 'k': *ret *= 1024; /* fall-through */
  default:
    break;
  }
  return NG_result_ok;
}

void kos_task_switch2other(void *arg)
{
  NGRTOS_UNUSED(arg);
  if (!IS_IRQ())
    __ng_sys_yield();
}

static void __kos_find_heap_size_inc(int *_base, int *_sz)
{
  int base = *_base, sz = *_sz;
  
  for (int i=1; ngrtos_TRUE; i++)
  {
    void *p;
    sz <<= i;
    p = malloc(base + sz);
    if (p == NULL)
    {
      *_sz   = sz>>1;
      *_base = base;
      break;
    }
    base += sz;
    free(p);
  }
}
static void __kos_find_heap_size_dec(int *_base, int *_sz)
{
  int base = *_base, sz = *_sz;
  
  for (int i=1; ngrtos_TRUE; i++)
  {
    void *p;
    sz >>= i;
    p = malloc(base + sz);
    if (p != NULL)
    {
      base += sz;
      free(p);
      break;
    }
  }
  *_sz   = sz;
  *_base = base + *_sz;
}

int kos_find_heap_size(void)
{
  int base = 0; 
  int sz = 1024<<6;

  __kos_find_heap_size_inc(&base, &sz);

  while (sz > 4)
    __kos_find_heap_size_dec(&base, &sz);

  return base;
}
