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

/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_CTYPE_H
#define _LINUX_CTYPE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * NOTE! This ctype does not handle EOF like the standard C
 * library is required to.
 */

#define ngctype_U	  0x01	/* upper */
#define ngctype_L	  0x02	/* lower */
#define ngctype_D	  0x04	/* digit */
#define ngctype_C	  0x08	/* cntrl */
#define ngctype_P	  0x10	/* punct */
#define ngctype_S	  0x20	/* white space (space/lf/tab) */
#define ngctype_X   0x40	/* hex digit */
#define ngctype_SP	0x80	/* hard space (0x20) */

extern const unsigned char _ctype[];

#define __ismask(x) (_ctype[(int)(unsigned char)(x)])

#define isalnum(c)	((__ismask(c)&(ngctype_U|ngctype_L|ngctype_D)) != 0)
#define isalpha(c)	((__ismask(c)&(ngctype_U|ngctype_L)) != 0)
#define iscntrl(c)	((__ismask(c)&(ngctype_C)) != 0)
#define isgraph(c)	((__ismask(c)&(ngctype_P|ngctype_U|ngctype_L|ngctype_D)) != 0)
#define islower(c)	((__ismask(c)&(ngctype_L)) != 0)
#define isprint(c)	((__ismask(c)&(ngctype_P|ngctype_U|ngctype_L|ngctype_D|ngctype_SP)) != 0)
#define ispunct(c)	((__ismask(c)&(ngctype_P)) != 0)
/* Note: isspace() must return false for %NUL-terminator */
#define isspace(c)	((__ismask(c)&(ngctype_S)) != 0)
#define isupper(c)	((__ismask(c)&(ngctype_U)) != 0)
#define isxdigit(c)	((__ismask(c)&(ngctype_D|ngctype_X)) != 0)

#define isascii(c) (((unsigned char)(c))<=0x7f)
#define toascii(c) (((unsigned char)(c))&0x7f)

static inline int isdigit(int c)
{
	return '0' <= c && c <= '9';
}

static inline unsigned char __tolower(unsigned char c)
{
	if (isupper(c))
		c -= 'A'-'a';
	return c;
}

static inline unsigned char __toupper(unsigned char c)
{
	if (islower(c))
		c -= 'a'-'A';
	return c;
}

#define tolower(c) __tolower(c)
#define toupper(c) __toupper(c)

/*
 * Fast implementation of tolower() for internal usage. Do not use in your
 * code.
 */
static inline char _tolower(const char c)
{
	return c | 0x20;
}

/* Fast check for octal digit */
static inline int isodigit(const char c)
{
	return c >= '0' && c <= '7';
}

#ifdef __cplusplus
}
#endif

#endif

