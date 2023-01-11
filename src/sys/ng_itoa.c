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
/*
MIT License

Copyright (c) 2017 James Edward Anhalt III - https://github.com/jeaiii/itoa

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdint.h>
#include "ng_arch.h"
#include "ng_defs.h"
#include "ng_string.h"
#include "ng_endian.h"

#define A(N) \
  t = (1ULL << (32 + N / 5 * N * 53 / 16)) / (uint32_t)(1e##N) + 1 - N / 9, \
  t *= u, t >>= N / 5 * N * 53 / 16, t += N / 5 * 4

/* 2 chars at a time, little endian only, unaligned 2 byte writes */
#define _DL(a,b) RTE_CPU_TO_BE_16((a)<<8 | (b))
static const uint16_t s_100s[] = {
  _DL('0','0'), _DL('1','0'), _DL('2','0'), _DL('3','0'), _DL('4','0'),
  _DL('5','0'), _DL('6','0'), _DL('7','0'), _DL('8','0'), _DL('9','0'),
  _DL('0','1'), _DL('1','1'), _DL('2','1'), _DL('3','1'), _DL('4','1'),
  _DL('5','1'), _DL('6','1'), _DL('7','1'), _DL('8','1'), _DL('9','1'),
  _DL('0','2'), _DL('1','2'), _DL('2','2'), _DL('3','2'), _DL('4','2'),
  _DL('5','2'), _DL('6','2'), _DL('7','2'), _DL('8','2'), _DL('9','2'),
  _DL('0','3'), _DL('1','3'), _DL('2','3'), _DL('3','3'), _DL('4','3'),
  _DL('5','3'), _DL('6','3'), _DL('7','3'), _DL('8','3'), _DL('9','3'),
  _DL('0','4'), _DL('1','4'), _DL('2','4'), _DL('3','4'), _DL('4','4'),
  _DL('5','4'), _DL('6','4'), _DL('7','4'), _DL('8','4'), _DL('9','4'),
  _DL('0','5'), _DL('1','5'), _DL('2','5'), _DL('3','5'), _DL('4','5'),
  _DL('5','5'), _DL('6','5'), _DL('7','5'), _DL('8','5'), _DL('9','5'),
  _DL('0','6'), _DL('1','6'), _DL('2','6'), _DL('3','6'), _DL('4','6'),
  _DL('5','6'), _DL('6','6'), _DL('7','6'), _DL('8','6'), _DL('9','6'),
  _DL('0','7'), _DL('1','7'), _DL('2','7'), _DL('3','7'), _DL('4','7'),
  _DL('5','7'), _DL('6','7'), _DL('7','7'), _DL('8','7'), _DL('9','7'),
  _DL('0','8'), _DL('1','8'), _DL('2','8'), _DL('3','8'), _DL('4','8'),
  _DL('5','8'), _DL('6','8'), _DL('7','8'), _DL('8','8'), _DL('9','8'),
  _DL('0','9'), _DL('1','9'), _DL('2','9'), _DL('3','9'), _DL('4','9'),
  _DL('5','9'), _DL('6','9'), _DL('7','9'), _DL('8','9'), _DL('9','9'),
};

#define W(N, I) *(uint16_t*)&b[N] = s_100s[I]
#define Q(N) b[N] = (char)((10ULL * (uint32_t)(t)) >> 32) + '0'
#define D(N) W(N, t >> 32)
#define E t = 100ULL * (uint32_t)(t)

#define L0 b[0] = (char)(u) + '0'
#define L1 W(0, u)
#define L2 A(1), D(0), Q(2)
#define L3 A(2), D(0), E, D(2)
#define L4 A(3), D(0), E, D(2), Q(4)
#define L5 A(4), D(0), E, D(2), E, D(4)
#define L6 A(5), D(0), E, D(2), E, D(4), Q(6)
#define L7 A(6), D(0), E, D(2), E, D(4), E, D(6)
#define L8 A(7), D(0), E, D(2), E, D(4), E, D(6), Q(8)
#define L9 A(8), D(0), E, D(2), E, D(4), E, D(6), E, D(8)

#define LN(N) (L##N, b += N + 1)
#define LZ(N) (L##N, b[N + 1] = '\0')
#define LG(F) (u<100 ? u<10 ? F(0) : F(1) : u<1000000 ? u<10000 ? u<1000 ? \
        F(2) : F(3) : u<100000 ? F(4) : F(5) : u<100000000 ? \
        u<10000000 ? F(6) : F(7) : u<1000000000 ? F(8) : F(9))

void u32toa_jeaiii(uint32_t u, char* b)
{
  uint64_t t;
  LG(LZ);
}

void i32toa_jeaiii(int32_t i, char* b)
{
  uint32_t u = i < 0 ? *b++ = '-', 0 - (uint32_t)(i) : i;
  uint64_t t;
  LG(LZ);
}

void u64toa_jeaiii(uint64_t n, char* b)
{
  uint32_t u;
  uint64_t t;

  if ((uint32_t)(n >> 32) == 0)
  {
    u = (uint32_t)(n);
    LG(LZ);
  }
  else
  {
    uint64_t a = n / 100000000;

    if ((uint32_t)(a >> 32) == 0)
    {
      u = (uint32_t)(a);
      LG(LN);
    }
    else
    {
      u = (uint32_t)(a / 100000000);
      LG(LN);
      u = a % 100000000;
      LN(7);
    }

    u = n % 100000000;
    LZ(7);
  }
}

void i64toa_jeaiii(int64_t i, char* b)
{
  uint64_t n = i < 0 ? *b++ = '-', 0 - (uint64_t)(i) : i;
  u64toa_jeaiii(n, b);
}

float ng_modff(float x, float *iptr)
{
	union {float f; uint32_t i;} u = {x};
	uint32_t mask;
	int e = (int)(u.i>>23 & 0xff) - 0x7f;

	/* no fractional part */
	if (e >= 23) {
		*iptr = x;
		if (e == 0x80 && u.i<<9 != 0) { /* nan */
			return x;
		}
		u.i &= 0x80000000UL;
		return u.f;
	}
	/* no integral part */
	if (e < 0) {
		u.i &= 0x80000000UL;
		*iptr = u.f;
		return x;
	}

	mask = 0x007fffffUL>>e;
	if ((u.i & mask) == 0) {
		*iptr = x;
		u.i &= 0x80000000UL;
		return u.f;
	}
	u.i &= ~mask;
	*iptr = u.f;
	return x - u.f;
}

double ng_modf(double x, double *iptr)
{
	union {double f; uint64_t i;} u = {x};
	uint64_t mask;
	int e = (int)(u.i>>52 & 0x7ff) - 0x3ff;

	/* no fractional part */
	if (e >= 52) {
		*iptr = x;
		if (e == 0x400 && u.i<<12 != 0) /* nan */
			return x;
		u.i &= 1ULL<<63;
		return u.f;
	}

	/* no integral part*/
	if (e < 0) {
		u.i &= 1ULL<<63;
		*iptr = u.f;
		return x;
	}

	mask = -1ULL>>12>>e;
	if ((u.i & mask) == 0) {
		*iptr = x;
		u.i &= 1ULL<<63;
		return u.f;
	}
	u.i &= ~mask;
	*iptr = u.f;
	return x - u.f;
}

void ftoa_jeaiii(double i, char* b)
{
  int len;
  double integ = (int64_t)i;
  double frag = i - integ;
  double n;

  n = integ < 0 ? *b++ = '-', 0 - (uint64_t)(integ) : integ;
  u64toa_jeaiii((uint64_t)n, b);
  len = ng_strlen(b);
  b[len]='.';

  n = 0;
  while (frag)
  {
    frag *= 10;
    if (frag - (int64_t)frag == 0)
      break;
  }
  u64toa_jeaiii((uint64_t)frag, b+len+1);
}

static const char digitsUpperAlpha[513] =
  "000102030405060708090A0B0C0D0E0F"
  "101112131415161718191A1B1C1D1E1F"
  "202122232425262728292A2B2C2D2E2F"
  "303132333435363738393A3B3C3D3E3F"
  "404142434445464748494A4B4C4D4E4F"
  "505152535455565758595A5B5C5D5E5F"
  "606162636465666768696A6B6C6D6E6F"
  "707172737475767778797A7B7C7D7E7F"
  "808182838485868788898A8B8C8D8E8F"
  "909192939495969798999A9B9C9D9E9F"
  "A0A1A2A3A4A5A6A7A8A9AAABACADAEAF"
  "B0B1B2B3B4B5B6B7B8B9BABBBCBDBEBF"
  "C0C1C2C3C4C5C6C7C8C9CACBCCCDCECF"
  "D0D1D2D3D4D5D6D7D8D9DADBDCDDDEDF"
  "E0E1E2E3E4E5E6E7E8E9EAEBECEDEEEF"
  "F0F1F2F3F4F5F6F7F8F9FAFBFCFDFEFF";

static const char digitsLowerAlpha[513] =
  "000102030405060708090a0b0c0d0e0f"
  "101112131415161718191a1b1c1d1e1f"
  "202122232425262728292a2b2c2d2e2f"
  "303132333435363738393a3b3c3d3e3f"
  "404142434445464748494a4b4c4d4e4f"
  "505152535455565758595a5b5c5d5e5f"
  "606162636465666768696a6b6c6d6e6f"
  "707172737475767778797a7b7c7d7e7f"
  "808182838485868788898a8b8c8d8e8f"
  "909192939495969798999a9b9c9d9e9f"
  "a0a1a2a3a4a5a6a7a8a9aaabacadaeaf"
  "b0b1b2b3b4b5b6b7b8b9babbbcbdbebf"
  "c0c1c2c3c4c5c6c7c8c9cacbcccdcecf"
  "d0d1d2d3d4d5d6d7d8d9dadbdcdddedf"
  "e0e1e2e3e4e5e6e7e8e9eaebecedeeef"
  "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff";

uint32_t 
u32toh_jeaiii(uint64_t num, char *s, ng_bool_t lowerAlpha)
{
  uint32_t x = (uint32_t)num;
	int i = 3;
	char *lut = (char *)((lowerAlpha) ? digitsLowerAlpha : digitsUpperAlpha);
	while (i >= 0)
	{
		int pos = (x & 0xFF) << 1;
		char ch = lut[pos];
		s[i << 1] = ch;

		ch = lut[pos + 1];
		s[i << 1 + 1] = ch;

		x >>= 8;
		i -= 1;
	}

  return 8;
}

uint32_t 
u64toh_jeaiii(uint64_t num, char *s, ng_bool_t lowerAlpha)
{
  u32toh_jeaiii(num>>32, s,   lowerAlpha);
  u32toh_jeaiii(num,     s+8, lowerAlpha);
  return 16;
}
