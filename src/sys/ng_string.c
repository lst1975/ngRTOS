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
#include "ng_defs.h"
#include "ng_ctype.h"
#include "ng_string.h"
#include "ng_buffer.h"

int ng_strcmp(const char *l, const char *r)
{
	for (; *l==*r && *l; l++, r++);
	return *(unsigned char *)l - *(unsigned char *)r;
}
int
ng_strncmp (const char *s1, const char *s2, ngrtos_size_t n)
{
  if ((!s1 && s2) || (s1 && !s2)) /* no match if only one ptr is NULL */
    return((int)(s1 - s2));
  if ((s1 == s2) || (n == 0)) /* match if ptrs same or length is 0 */
    return(0);

  while (n-- != 0 && *s1 == *s2) {
    if (n == 0 || *s1 == '\0' || *s2 == '\0')
      break;
    s1++;
    s2++;
  }

  return (*(unsigned char *) s1) - (*(unsigned char *) s2);
}

char *
ng_strrchr (const ng_string_s *s, char c)
{
  const char *end;

  if (!s->len)
    return NULL;
  
  for (end = &s->ptr[s->len-1]; end != s->ptr; --end) 
  {
    if (*end == c)
      return (char *)end;
  }

  return NULL;
}

char *
ng_strchr (const ng_string_s *s, char c)
{
  int i;
  
  if (!s->len)
    return NULL;
  
  for (i=0; i < s->len; ++i) 
  {
    if (s->ptr[i] == c)
      return (char *)&s->ptr[i];
  }

  return NULL;
}

ngrtos_size_t
ng_strlen (const char *s)
{
  ngrtos_size_t i=0;
  while (s[i] != '\0')
    i++;
  return i;
}

static const uint64_t offsets[21] = {
   0,
  '0',
  '0'*11ull,
  '0'*111ull,
  '0'*1111ull,
  '0'*11111ull,
  '0'*111111ull,
  '0'*1111111ull,
  '0'*11111111ull,
  '0'*111111111ull,
  '0'*1111111111ull,
  '0'*11111111111ull,
  '0'*111111111111ull,
  '0'*1111111111111ull,
  '0'*11111111111111ull,
  '0'*111111111111111ull,
  '0'*1111111111111111ull,
  '0'*11111111111111111ull,
  '0'*111111111111111111ull,
  '0'*1111111111111111111ull,
  '0'*11111111111111111111ull};

/* convert char *s to an unsigned 64bit integer */
uint64_t ng_fast_strtoui64(const char *s)
{
  uint64_t ret = s[0];
  uint8_t len = 1;
  while(isdigit(s[len]))
  {
      ret = ret*10 + s[len++];
  }
  return ret-offsets[len];
}

/* convert char *s to an unsigned 32bit integer */
uint32_t ng_fast_strtoui32(const char *s)
{
  uint32_t ret = s[0];
  uint8_t len = 1;
  while(isdigit(s[len]))
  {
      ret = ret*10 + s[len++];
  }
  return ret-(uint32_t)(offsets[len]);
}

/* convert char *s to an unsigned 64bit integer
   len is the number of numeric characters
   s does not require the trailing '\0'
 */
uint64_t ng_fast_atoui64(const char *str, uint8_t len)
{
  size_t value = 0;
  switch (len) { /* handle up to 20 digits, assume we're 64-bit */
      case 20:    value += str[len-20] * 10000000000000000000ull;
      case 19:    value += str[len-19] * 1000000000000000000ull;
      case 18:    value += str[len-18] * 100000000000000000ull;
      case 17:    value += str[len-17] * 10000000000000000ull;
      case 16:    value += str[len-16] * 1000000000000000ull;
      case 15:    value += str[len-15] * 100000000000000ull;
      case 14:    value += str[len-14] * 10000000000000ull;
      case 13:    value += str[len-13] * 1000000000000ull;
      case 12:    value += str[len-12] * 100000000000ull;
      case 11:    value += str[len-11] * 10000000000ull;
      case 10:    value += str[len-10] * 1000000000ull;
      case  9:    value += str[len- 9] * 100000000ull;
      case  8:    value += str[len- 8] * 10000000ull;
      case  7:    value += str[len- 7] * 1000000ull;
      case  6:    value += str[len- 6] * 100000ull;
      case  5:    value += str[len- 5] * 10000ull;
      case  4:    value += str[len- 4] * 1000ull;
      case  3:    value += str[len- 3] * 100ull;
      case  2:    value += str[len- 2] * 10ull;
      case  1:    value += str[len- 1];
  }
  return value - offsets[len];
}

/* convert char *s to an unsigned 32bit integer
   len is the number of numeric characters
   s does not require the trailing '\0'
 */
uint32_t ng_fast_atoui32(const char *str, uint8_t len)
{
  uint32_t value = 0;
  switch (len) { /* handle up to 10 digits, assume we're 32-bit */
      case 10:    value += str[len-10] * 1000000000ul;
      case  9:    value += str[len- 9] * 100000000ul;
      case  8:    value += str[len- 8] * 10000000ul;
      case  7:    value += str[len- 7] * 1000000ul;
      case  6:    value += str[len- 6] * 100000ul;
      case  5:    value += str[len- 5] * 10000ul;
      case  4:    value += str[len- 4] * 1000ul;
      case  3:    value += str[len- 3] * 100ul;
      case  2:    value += str[len- 2] * 10ul;
      case  1:    value += str[len- 1];
  }
  return value - (uint32_t)(offsets[len]);
}

int ng_atoi(const char *ptr, int len)
{
  int v = 0;
  for (int i=0;i<len;i++)
  {
    if (ptr[i] < '0' || ptr[i] > '9')
      break;
    v = v*10 + (ptr[i] - '0');
  }
  return v;
}

int ng_str_atoi(const char *ptr)
{
  int v = 0;
  for (int i=0;i<ptr[i]!='\0';i++)
  {
    if (ptr[i] < '0' || ptr[i] > '9')
      break;
    v = v*10 + (ptr[i] - '0');
  }
  return v;
}

/*
 * Differs from the router version (i.e., copytoASCIZ) in that this
 * version returns a pointer to the resulting string, whereas
 * copytoASCIZ is a void function.
 */

char *
ng_strcpy (char *dst, const char *src)
{
  char *s = dst;
  while ((*dst++ = *src++));
  return s;
}

/*
FUNCTION
	strncpy---counted copy string

ANSI_SYNOPSIS
	#include <string.h>
	char *strncpy(char *dst, const char *src, size_t length);

DESCRIPTION
	strncpy copies not more than length characters from the
	the string pointed to by src (including the terminating
	null character) to the array pointed to by dst.  If the
	string pointed to by src is shorter than length
	characters, null characters are appended to the destination
	array until a total of length characters have been
	written.

RETURNS
	This function returns the initial value of dst.

PORTABILITY
strncpy is ANSI C.

strncpy requires no supporting OS subroutines.

QUICKREF
	strncpy ansi pure
*/

char *
ng_strncpy (char *dst, const char *src, size_t n)
{
  char *dscan;
  const char *sscan;
  size_t count;

  dscan = dst;
  sscan = src;
  count = n;
  /*
   * Copy the source to the dest until there is a NULL
   */
  while (count > 0) {
  	--count;
  	if ((*dscan++ = *sscan++) == '\0')
	    break;
  }
  /*
   * Pad the rest of dest with NULLs
   */
  while (count-- > 0)
  	*dscan++ = '\0';

  return dst;
}

static char *twobyte_strstr(const unsigned char *h, const unsigned char *n)
{
	uint16_t nw = n[0]<<8 | n[1], hw = h[0]<<8 | h[1];
	for (h++; *h && hw != nw; hw = hw<<8 | *++h);
	return *h ? (char *)h-1 : 0;
}

static char *threebyte_strstr(const unsigned char *h, const unsigned char *n)
{
	uint32_t nw = (uint32_t)n[0]<<24 | n[1]<<16 | n[2]<<8;
	uint32_t hw = (uint32_t)h[0]<<24 | h[1]<<16 | h[2]<<8;
	for (h+=2; *h && hw != nw; hw = (hw|*++h)<<8);
	return *h ? (char *)h-2 : 0;
}

static char *fourbyte_strstr(const unsigned char *h, const unsigned char *n)
{
	uint32_t nw = (uint32_t)n[0]<<24 | n[1]<<16 | n[2]<<8 | n[3];
	uint32_t hw = (uint32_t)h[0]<<24 | h[1]<<16 | h[2]<<8 | h[3];
	for (h+=3; *h && hw != nw; hw = hw<<8 | *++h);
	return *h ? (char *)h-3 : 0;
}

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

#define BITOP(a,b,op) \
 ((a)[(size_t)(b)/(8*sizeof *(a))] op (size_t)1<<((size_t)(b)%(8*sizeof *(a))))

static char *twoway_strstr(const unsigned char *h, const unsigned char *n)
{
	const unsigned char *z;
	size_t l, ip, jp, k, p, ms, p0, mem, mem0;
	size_t byteset[32 / sizeof(size_t)] = { 0 };
	size_t shift[256];

	/* Computing length of needle and fill shift table */
	for (l=0; n[l] && h[l]; l++)
		BITOP(byteset, n[l], |=), shift[n[l]] = l+1;
	if (n[l]) return 0; /* hit the end of h */

	/* Compute maximal suffix */
	ip = -1; jp = 0; k = p = 1;
	while (jp+k<l) {
		if (n[ip+k] == n[jp+k]) {
			if (k == p) {
				jp += p;
				k = 1;
			} else k++;
		} else if (n[ip+k] > n[jp+k]) {
			jp += k;
			k = 1;
			p = jp - ip;
		} else {
			ip = jp++;
			k = p = 1;
		}
	}
	ms = ip;
	p0 = p;

	/* And with the opposite comparison */
	ip = -1; jp = 0; k = p = 1;
	while (jp+k<l) {
		if (n[ip+k] == n[jp+k]) {
			if (k == p) {
				jp += p;
				k = 1;
			} else k++;
		} else if (n[ip+k] < n[jp+k]) {
			jp += k;
			k = 1;
			p = jp - ip;
		} else {
			ip = jp++;
			k = p = 1;
		}
	}
	if (ip+1 > ms+1) ms = ip;
	else p = p0;

	/* Periodic needle? */
	if (ng_memcmp(n, n+p, ms+1)) {
		mem0 = 0;
		p = MAX(ms, l-ms-1) + 1;
	} else mem0 = l-p;
	mem = 0;

	/* Initialize incremental end-of-haystack pointer */
	z = h;

	/* Search loop */
	for (;;) {
		/* Update incremental end-of-haystack pointer */
		if (z-h < l) {
			/* Fast estimate for MIN(l,63) */
			size_t grow = l | 63;
			const unsigned char *z2 = ng_memstr(z, 0, grow);
			if (z2) {
				z = z2;
				if (z-h < l) return 0;
			} else z += grow;
		}

		/* Check last byte first; advance by shift on mismatch */
		if (BITOP(byteset, h[l-1], &)) {
			k = l-shift[h[l-1]];
			if (k) {
				if (k < mem) k = mem;
				h += k;
				mem = 0;
				continue;
			}
		} else {
			h += l;
			mem = 0;
			continue;
		}

		/* Compare right half */
		for (k=MAX(ms+1,mem); n[k] && n[k] == h[k]; k++);
		if (n[k]) {
			h += k-ms;
			mem = 0;
			continue;
		}
		/* Compare left half */
		for (k=ms+1; k>mem && n[k-1] == h[k-1]; k--);
		if (k <= mem) return (char *)h;
		h += p;
		mem = mem0;
	}
}

char *ng_strstr(const char *h, const char *n)
{
	/* Return immediately on empty needle */
	if (!n[0]) return (char *)h;

	/* Use faster algorithms for short needles */
	h = ng_strchr(h, *n);
	if (!h || !n[1]) return (char *)h;
	if (!h[1]) return 0;
	if (!n[2]) return twobyte_strstr((void *)h, (void *)n);
	if (!h[2]) return 0;
	if (!n[3]) return threebyte_strstr((void *)h, (void *)n);
	if (!h[3]) return 0;
	if (!n[4]) return fourbyte_strstr((void *)h, (void *)n);

	return twoway_strstr((void *)h, (void *)n);
}

