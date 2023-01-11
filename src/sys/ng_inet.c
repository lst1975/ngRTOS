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
 
/*  $OpenBSD: inet_pton.c,v 1.10 2015/09/13 21:36:08 guenther Exp $  */

/* Copyright (c) 1996 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include <stdint.h>
#include "ng_inet.h"
#include "ng_defs.h"
#include "ng_string.h"
#include "ng_errno.h"
#include "ng_mcopy.h"
#include "ng_buffer.h"

/*
 * WARNING: Don't even consider trying to compile this on a system where
 * sizeof(int) < 4.  sizeof(int) > 4 is fine; all the world's not a VAX.
 */

/*
 * Define constants based on rfc883
 */
#define PACKETSZ  512    /* maximum packet size */
#define MAXDNAME  1025    /* maximum presentation domain name */
#define MAXCDNAME  255    /* maximum compressed domain name */
#define MAXLABEL  63    /* maximum length of domain label */
#define HFIXEDSZ  12    /* #/bytes of fixed data in header */
#define QFIXEDSZ  4    /* #/bytes of fixed data in query */
#define RRFIXEDSZ  10    /* #/bytes of fixed data in r record */
#define INT32SZ    4    /* for systems without 32-bit ints */
#define INT16SZ    2    /* for systems without 16-bit ints */
#define INADDRSZ  4    /* IPv4 T_A */
#define IN6ADDRSZ  16    /* IPv6 T_AAAA */

/* int
 * inet_pton(af, src, dst)
 *  convert from presentation format (which usually means ASCII printable)
 *  to network format (which is usually some kind of binary format).
 * return:
 *  1 if the address was valid for the specified address family
 *  0 if the address wasn't valid (`dst' is untouched in this case)
 *  -1 if some other error occurred (`dst' is untouched in this case, too)
 * author:
 *  Paul Vixie, 1996.
 */
int
ng_inet_pton(int af, const char *src, void *dst)
{
  switch (af) {
  case AF_INET:
    return (ng_inet_pton4(src, dst));
  case AF_INET6:
    return (ng_inet_pton6(src, dst));
  default:
    return (-ng_err_EAFNOSUPPORT);
  }
  /* NOTREACHED */
}

/* int
 * inet_pton4(src, dst)
 *  like inet_aton() but without all the hexadecimal and shorthand.
 * return:
 *  1 if `src' is a valid dotted quad, else 0.
 * notice:
 *  does not touch `dst' unless it's returning 1.
 * author:
 *  Paul Vixie, 1996.
 */
int
ng_inet_pton4(const char *src, uint8_t *dst)
{
  static const ng_buffer_s digits = { 
    .cptr = "0123456789", 
    .len = sizeof("0123456789")-1 };
  int saw_digit, octets, ch;
  uint8_t tmp[INADDRSZ], *tp;

  saw_digit = 0;
  octets = 0;
  *(tp = tmp) = 0;
  while ((ch = *src++) != '\0') {
    const char *pch;

    if ((pch = ng_buf_strchr(&digits, ch)) != NULL) {
      uint32_t _new = *tp * 10 + (pch - (const char *)digits.cptr);

      if (_new > 255)
        return ngrtos_FALSE;
      if (! saw_digit) {
        if (++octets > 4)
          return ngrtos_FALSE;
        saw_digit = 1;
      }
      *tp = _new;
    } else if (ch == '.' && saw_digit) {
      if (octets == 4)
        return ngrtos_FALSE;
      *++tp = 0;
      saw_digit = 0;
    } else
      return ngrtos_FALSE;
  }
  if (octets < 4)
    return ngrtos_FALSE;

  memcpy(dst, tmp, INADDRSZ);
  return ngrtos_TRUE;
}

/* int
 * inet_pton6(src, dst)
 *  convert presentation level address to network order binary form.
 * return:
 *  1 if `src' is a valid [RFC1884 2.2] address, else 0.
 * notice:
 *  does not touch `dst' unless it's returning 1.
 * credit:
 *  inspired by Mark Andrews.
 * author:
 *  Paul Vixie, 1996.
 */
int
ng_inet_pton6(const char *src, uint8_t *dst)
{
  static const ng_buffer_s xdigits_l = { 
    .cptr = "0123456789abcdef", 
    .len = sizeof("0123456789abcdef")-1 };
  static const ng_buffer_s xdigits_u = { 
    .cptr = "0123456789ABCDEF", 
    .len = sizeof("0123456789ABCDEF")-1 };
  const ng_buffer_s *xdigits;
  
  uint8_t tmp[IN6ADDRSZ], *tp, *endp, *colonp;
  const char *curtok;
  int ch, saw_xdigit, count_xdigit;
  uint32_t val;

  ng_memset((tp = tmp), '\0', IN6ADDRSZ);
  endp = tp + IN6ADDRSZ;
  colonp = NULL;
  /* Leading :: requires some special handling. */
  if (*src == ':')
    if (*++src != ':')
      return ngrtos_FALSE;
  curtok = src;
  saw_xdigit = count_xdigit = 0;
  val = 0;
  while ((ch = *src++) != '\0') {
    const char *pch;
    
    xdigits = &xdigits_l;
    pch = ng_buf_strchr(xdigits, ch);
    if (pch == NULL)
    {
      xdigits = &xdigits_u;
      pch = ng_buf_strchr(xdigits, ch);
    }
    if (pch != NULL) {
      if (count_xdigit >= 4)
        return ngrtos_FALSE;
      val <<= 4;
      val |= (pch - (const char *)xdigits->cptr);
      if (val > 0xffff)
        return ngrtos_FALSE;
      saw_xdigit = 1;
      count_xdigit++;
      continue;
    }
    if (ch == ':') {
      curtok = src;
      if (!saw_xdigit) {
        if (colonp)
          return ngrtos_FALSE;
        colonp = tp;
        continue;
      } else if (*src == '\0') {
        return ngrtos_FALSE;
      }
      if (tp + INT16SZ > endp)
        return ngrtos_FALSE;
      *tp++ = (uint8_t) (val >> 8) & 0xff;
      *tp++ = (uint8_t) val & 0xff;
      saw_xdigit = 0;
      count_xdigit = 0;
      val = 0;
      continue;
    }
    if (ch == '.' && ((tp + INADDRSZ) <= endp) &&
        ng_inet_pton4(curtok, tp) > 0) {
      tp += INADDRSZ;
      saw_xdigit = 0;
      count_xdigit = 0;
      break;  /* '\0' was seen by inet_pton4(). */
    }
    return ngrtos_FALSE;
  }
  if (saw_xdigit) {
    if (tp + INT16SZ > endp)
      return (0);
    *tp++ = (uint8_t) (val >> 8) & 0xff;
    *tp++ = (uint8_t) val & 0xff;
  }
  if (colonp != NULL) {
    /*
     * Since some memmove()'s erroneously fail to handle
     * overlapping regions, we'll do the shift by hand.
     */
    const int n = tp - colonp;
    int i;

    if (tp == endp)
      return ngrtos_FALSE;
    for (i = 1; i <= n; i++) {
      endp[- i] = colonp[n - i];
      colonp[n - i] = 0;
    }
    tp = endp;
  }
  if (tp != endp)
    return (0);
  ng_memcpy(dst, tmp, IN6ADDRSZ);
  return ngrtos_TRUE;
}
