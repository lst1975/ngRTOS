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
   Unix snprintf implementation.
   Version 1.4

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Revision History:

   1.4:
      *  integrate in FreeRADIUS's libradius:
    *  Fetched from: http://savannah.gnu.org/cgi-bin/viewcvs/mailutils/mailutils/lib/snprintf.c?rev=1.4
    *  Fetched from: http://savannah.gnu.org/cgi-bin/viewcvs/mailutils/mailutils/lib/snprintf.h?rev=1.4
    *  Replace config.h with autoconf.h
    *  Protect with HAVE_SNPRINTF and HAVE_VSNPRINTF
   1.3:
      *  add #include <config.h> ifdef HAVE_CONFIG_H
      *  cosmetic change, when exponent is 0 print xxxE+00
   instead of xxxE-00
   1.2:
      *  put the program under LGPL.
   1.1:
      *  added changes from Miles Bader
      *  corrected a bug with %f
      *  added support for %#g
      *  added more comments :-)
   1.0:
      *  supporting must ANSI syntaxic_sugars
   0.0:
      *  suppot %s %c %d

 THANKS(for the patches and ideas):
     Miles Bader
     Cyrille Rustom
     Jacek Slabocewiz
     Mike Parker(mouse)

*/

#include "ng_config.h"
#include <stdarg.h>
#include "ng_arch.h"
#include "ng_ctype.h"
#include "ng_endian.h"
#include "ng_align.h"
#include "ng_string.h"
#include "ng_itoa.h"
#include "ng_mcopy.h"
#include "ng_snprintf.h"

#define PRIVATE static
#define PUBLIC

/*
 * For the FLOATING POINT FORMAT :
 *  the challenge was finding a way to
 *  manipulate the Real numbers without having
 *  to resort to mathematical function(it
 *  would require to link with -lm) and not
 *  going down to the bit pattern(not portable)
 *
 *  so a number, a real is:

      real = integral + fraction

      integral = ... + a(2)*10^2 + a(1)*10^1 + a(0)*10^0
      fraction = b(1)*10^-1 + b(2)*10^-2 + ...

      where:
       0 <= a(i) => 9
       0 <= b(i) => 9

    from then it was simple math
 */

/*
 * size of the buffer for the integral part
 * and the fraction part
 */
#define MAX_INT  24 + 1 /* 1 for the null */
#define MAX_FRACT 24 + 1

/*
 * If the compiler supports (long long)
 */
#ifndef LONG_LONG
# define LONG_LONG long long
/*# define LONG_LONG int64_t*/
#endif

/*
 * If the compiler supports (long double)
 */
#ifndef LONG_DOUBLE
# define LONG_DOUBLE long double
/*# define LONG_DOUBLE double*/
#endif

#define SWAP_INT(a,b) {int t; t = (a); (a) = (b); (b) = t;}

/* this struct holds everything we need */
struct DATA {
  int length;
  char *holder;
  int counter;
  char const *pf;
/* FLAGS */
  int width, precision;
  int justify; char pad;
  int square, space, star_w, star_p, a_long, a_longlong;
};

/* the floating point stuff */
PRIVATE double pow_10(int);
PRIVATE int log_10(double);
PRIVATE double integral(double, double *);
PRIVATE char * numtoa(char *, char *, double, int, int, char **);

/*
 * numtoa() uses PRIVATE buffers to store the results,
 * So this function is not reentrant
 */
PRIVATE char *ng_itoa(double n, char *buf, int len) 
{
  char p[MAX_INT];
  i64toa_jeaiii((int64_t)n, p);
  ng_strncpy(buf, p, len);
  return buf;
}
PRIVATE char *ng_otoa(double n, char *buf, int len) 
{
  char integral_part[MAX_INT];
  char fraction_part[MAX_FRACT];
  char *p = numtoa(integral_part, fraction_part, n, 8, 0, (char **)0);
  ng_strncpy(buf, p, len);
  return buf;
}

PRIVATE char *ng_dtoa(double n, int p, char *i, char **f) 
{
  return numtoa(i, *f, n, 10, 0, f);
}

/* for the format */
PRIVATE void conv_flag(char *, struct DATA *);
PRIVATE void floating(struct DATA *, double);
PRIVATE void exponent(struct DATA *, double);
PRIVATE void decimal(struct DATA *, double);
PRIVATE void octal(struct DATA *, double);
PRIVATE void hexa(struct DATA *, double);
PRIVATE void strings(struct DATA *, const char *);

/* those are defines specific to snprintf to hopefully
 * make the code clearer :-)
 */
#define RIGHT 1
#define LEFT  0
#define NOT_FOUND -1
#define FOUND 1
#define MAX_FIELD 15

/* the conversion flags */
#define isflag(c) ((c) == '#' || (c) == ' ' || \
       (c) == '*' || (c) == '+' || \
       (c) == '-' || (c) == '.' || \
       isdigit(c))

/* round off to the precision */
#define ROUND(d, p) \
      (d < 0.) ? \
       d - pow_10(-(p)->precision) * 0.5 : \
       d + pow_10(-(p)->precision) * 0.5

/* set default precision */
#define DEF_PREC(p) \
      if ((p)->precision == NOT_FOUND) \
        (p)->precision = 6

/* put a char */
#define PUT_CHAR(c, p) \
      if ((p)->counter < (p)->length) { \
        *(p)->holder++ = (uint8_t)(c); \
        (p)->counter++; \
      }

#define PUT_PLUS(d, p) \
      if ((d) > 0. && (p)->justify == RIGHT) \
        PUT_CHAR('+', p)

#define PUT_SPACE(d, p) \
      if ((p)->space == FOUND && (d) > 0.) \
        PUT_CHAR(' ', p)

/* pad right */
#define PAD_RIGHT(p) \
      if ((p)->width > 0 && (p)->justify != LEFT) \
        for (; (p)->width > 0; (p)->width--) \
     PUT_CHAR((p)->pad, p)

/* pad left */
#define PAD_LEFT(p) \
      if ((p)->width > 0 && (p)->justify == LEFT) \
        for (; (p)->width > 0; (p)->width--) \
     PUT_CHAR((p)->pad, p)

/* if width and prec. in the args */
#define STAR_ARGS(p) \
      if ((p)->star_w == FOUND) \
        (p)->width = va_arg(args, int); \
      if ((p)->star_p == FOUND) \
        (p)->precision = va_arg(args, int)

/*
 * Find the nth power of 10
 */
PRIVATE double
pow_10(int n)
{
  int i;
  double P;

  if (n < 0)
    for (i = 1, P = 1., n = -n ; i <= n ; i++) {P *= .1;}
  else
    for (i = 1, P = 1. ; i <= n ; i++) {P *= 10.0;}
  return P;
}

/*
 * Find the integral part of the log in base 10
 * Note: this not a real log10()
   I just need and approximation(integerpart) of x in:
    10^x ~= r
 * log_10(200) = 2;
 * log_10(250) = 2;
 */
PRIVATE int
log_10(double r)
{
  int i = 0;
  double result = 1.;

  if (r < 0.)
    r = -r;

  if (r < 1.) {
    while (result >= r) {result *= .1; i++;}
    return (-i);
  } else {
    while (result <= r) {result *= 10.; i++;}
    return (i - 1);
  }
}

/*
 * This function return the fraction part of a double
 * and set in ip the integral part.
 * In many ways it resemble the modf() found on most Un*x
 */
PRIVATE double
integral(double real, double * ip)
{
  int j;
  double i, s, p;
  double real_integral = 0.;

/* take care of the obvious */
/* equal to zero ? */
  if (real == 0.) {
    *ip = 0.;
    return (0.);
  }

/* negative number ? */
  if (real < 0.)
    real = -real;

/* a fraction ? */
  if ( real < 1.) {
    *ip = 0.;
    return real;
  }
/* the real work :-) */
  for (j = log_10(real); j >= 0; j--) {
    p = pow_10(j);
    s = (real - real_integral)/p;
    i = 0.;
    while (i + 1. <= s) {i++;}
    real_integral += i*p;
  }
  *ip = real_integral;
  return (real - real_integral);
}

#define PRECISION 1.e-6
/*
 * return an ascii representation of the integral part of the number
 * and set fract to be an ascii representation of the fraction part
 * the container for the fraction and the integral part or staticly
 * declare with fix size
 */
PRIVATE char *
numtoa(char *integral_part, char *fraction_part, double number, 
  int base, int precision, char ** fract)
{
  register int i, j;
  double ip, fp; /* integer and fraction part */
  double fraction;
  int digits = MAX_INT - 1;
  double sign;
  int ch;

/* taking care of the obvious case: 0.0 */
  if (number == 0.) {
    integral_part[0] = '0';
    integral_part[1] = '\0';
    fraction_part[0] = '0';
    fraction_part[1] = '\0';
    return integral_part;
  }

  /* for negative numbers */
  if ((sign = number) < 0.) {
    number = -number;
    digits--; /* sign consume one digit */
  }

  fraction = integral(number, &ip);
  number = ip;
  /* do the integral part */
  if ( ip == 0.) {
    integral_part[0] = '0';
    i = 1;
  } else {
    for ( i = 0; i < digits && number != 0.; ++i) {
      number /= base;
      fp = integral(number, &ip);
      ch = (int)((fp + PRECISION)*base); /* force to round */
      integral_part[i] = (ch <= 9) ? ch + '0' : ch + 'a' - 10;
      if (! isxdigit(integral_part[i])) /* bail out overflow !! */
        break;
      number = ip;
     }
  }

  /* Oh No !! out of bound, ho well fill it up ! */
  if (number != 0.)
    for (i = 0; i < digits; ++i)
      integral_part[i] = '9';

  /* put the sign ? */
  if (sign < 0.)
    integral_part[i++] = '-';

  integral_part[i] = '\0';

  /* reverse every thing */
  for ( i--, j = 0; j < i; j++, i--)
    SWAP_INT(integral_part[i], integral_part[j]);

  /* the fractionnal part */
  for (i=0, fp=fraction; precision > 0 && i < MAX_FRACT ; i++, precision--  ) {
    fraction_part[i] = (int)((fp + PRECISION)*10. + '0');
    if (! isdigit(fraction_part[i])) /* underflow ? */
      break;
    fp = (fp*10.0) - (double)(long)((fp + PRECISION)*10.);
  }
  fraction_part[i] = '\0';

  if (fract != (char **)0)
    *fract = fraction_part;

  return integral_part;

}

/* for %d and friends, it puts in holder
 * the representation with the right padding
 */
PRIVATE void
decimal(struct DATA *p, double d)
{
  char buf[MAX_INT];
  char *tmp=buf;

  i64toa_jeaiii((int64_t)d, buf);
  p->width -= ng_strlen(tmp);
  PAD_RIGHT(p);
  PUT_PLUS(d, p);
  PUT_SPACE(d, p);
  while (*tmp) { /* the integral */
    PUT_CHAR(*tmp, p);
    tmp++;
  }
  PAD_LEFT(p);
}

/* for %o octal representation */
PRIVATE void
octal(struct DATA *p, double d)
{
  char buf[MAX_INT];
  char *tmp;

  tmp = ng_otoa(d, buf, sizeof(buf));
  p->width -= ng_strlen(tmp);
  PAD_RIGHT(p);
  if (p->square == FOUND) /* had prefix '0' for octal */
    PUT_CHAR('0', p);
  while (*tmp) { /* octal */
    PUT_CHAR(*tmp, p);
    tmp++;
  }
  PAD_LEFT(p);
}

/* for %x %X hexadecimal representation */
PRIVATE void
hexa(struct DATA *d, const double n)
{
  char buf[MAX_INT];
  char *tmp=buf;

  u64toh_jeaiii((uint64_t)n, buf, *d->pf != 'X');
  d->width -= ng_strlen(tmp);
  PAD_RIGHT(d);
  if (d->square == FOUND) { /* prefix '0x' for hexa */
    PUT_CHAR('0', d); 
    PUT_CHAR(*d->pf, d);
  }
  while (*tmp) { /* hexa */
    PUT_CHAR((*d->pf == 'X' ? toupper(*tmp) : *tmp), d);
    tmp++;
  }
  PAD_LEFT(d);
}

/* %s strings */
PRIVATE void
strings(struct DATA *p, const char *tmp)
{
  int i;

  i = ng_strlen(tmp);
  if (p->precision != NOT_FOUND) /* the smallest number */
    i = (i < p->precision ? i : p->precision);
  p->width -= i;
  PAD_RIGHT(p);
  while (i-- > 0) { /* put the sting */
    PUT_CHAR(*tmp, p);
    tmp++;
  }
  PAD_LEFT(p);
}

/* %f or %g  floating point representation */
PRIVATE void
floating(struct DATA *p, double d)
{
  char integral_part[MAX_INT];
  char fraction_part[MAX_FRACT];
  char *tmp, *tmp2;
  int i;

  DEF_PREC(p);
  d = ROUND(d, p);
  tmp2 = fraction_part;
  tmp = ng_dtoa(d, p->precision, integral_part, &tmp2);
  /* calculate the padding. 1 for the dot */
  p->width = p->width -
      ((d > 0. && p->justify == RIGHT) ? 1:0) -
      ((p->space == FOUND) ? 1:0) -
      ng_strlen(tmp) - p->precision - 1;
  PAD_RIGHT(p);
  PUT_PLUS(d, p);
  PUT_SPACE(d, p);
  while (*tmp) { /* the integral */
    PUT_CHAR(*tmp, p);
    tmp++;
  }
  if (p->precision != 0 || p->square == FOUND)
    PUT_CHAR('.', p);  /* put the '.' */
  if (*p->pf == 'g' || *p->pf == 'G') /* smash the trailing zeros */
    for (i = ng_strlen(tmp2) - 1; i >= 0 && tmp2[i] == '0'; i--)
       tmp2[i] = '\0';
  for (; *tmp2; tmp2++)
    PUT_CHAR(*tmp2, p); /* the fraction */

  PAD_LEFT(p);
}

/* %e %E %g exponent representation */
PRIVATE void
exponent(struct DATA *p, double d)
{
  char integral_part[MAX_INT];
  char fraction_part[MAX_FRACT];
  char *tmp, *tmp2;
  int j, i;

  DEF_PREC(p);
  j = log_10(d);
  d = d / pow_10(j);  /* get the Mantissa */
  d = ROUND(d, p);
  tmp2 = fraction_part;
  tmp = ng_dtoa(d, p->precision, integral_part, &tmp2);
  /* 1 for unit, 1 for the '.', 1 for 'e|E',
   * 1 for '+|-', 3 for 'exp' */
  /* calculate how much padding need */
  p->width = p->width -
       ((d > 0. && p->justify == RIGHT) ? 1:0) -
       ((p->space == FOUND) ? 1:0) - p->precision - 7;
  PAD_RIGHT(p);
  PUT_PLUS(d, p);
  PUT_SPACE(d, p);
  while (*tmp) {/* the integral */
    PUT_CHAR(*tmp, p);
    tmp++;
  }
  if (p->precision != 0 || p->square == FOUND)
    PUT_CHAR('.', p);  /* the '.' */
  if (*p->pf == 'g' || *p->pf == 'G') /* smash the trailing zeros */
    for (i = ng_strlen(tmp2) - 1; i >= 0 && tmp2[i] == '0'; i--)
       tmp2[i] = '\0';
  for (; *tmp2; tmp2++)
    PUT_CHAR(*tmp2, p); /* the fraction */

  if (*p->pf == 'g' || *p->pf == 'e') { /* the exponent put the 'e|E' */
    PUT_CHAR('e', p);
  } else
    PUT_CHAR('E', p);
  if (j >= 0) {  /* the sign of the exp */
    PUT_CHAR('+', p);
  } else {
    PUT_CHAR('-', p);
    j = -j;
  }
  
  tmp = ng_itoa((double)j, integral_part, sizeof(integral_part));
  if (j < 9) {  /* need to pad the exponent with 0 '000' */
    PUT_CHAR('0', p); PUT_CHAR('0', p);
  } else if (j < 99)
    PUT_CHAR('0', p);
  while (*tmp) { /* the exponent */
    PUT_CHAR(*tmp, p);
    tmp++;
  }
  PAD_LEFT(p);
}

/* initialize the conversion specifiers */
PRIVATE void
conv_flag(char * s, struct DATA * p)
{
  char number[MAX_FIELD/2];
  int i;

  /* reset the flags.  */
  p->precision = p->width = NOT_FOUND;
  p->star_w = p->star_p = NOT_FOUND;
  p->square = p->space = NOT_FOUND;
  p->a_long = p->justify = NOT_FOUND;
  p->a_longlong = NOT_FOUND;
  p->pad = ' ';

  for(;s && *s ;s++)
  {
    switch (*s)
    {
      case ' ': 
        p->space = FOUND; 
        break;
        
      case '#': 
        p->square = FOUND; 
        break;
        
      case '*': 
        if (p->width == NOT_FOUND)
          p->width = p->star_w = FOUND;
        else
          p->precision = p->star_p = FOUND;
        break;
        
      case '+': 
        p->justify = RIGHT; 
        break;
        
      case '-': 
        p->justify = LEFT; 
        break;
        
      case '.': 
        if (p->width == NOT_FOUND)
          p->width = 0;
        break;
        
      case '0': 
        p->pad = '0'; 
        break;
        
      case '1': case '2': case '3':
      case '4': case '5': case '6':
      case '7': case '8': case '9':     /* gob all the digits */
        for (i = 0; isdigit(*s); i++, s++)
          if (i < MAX_FIELD/2 - 1)
            number[i] = *s;
        number[i] = '\0';
        if (p->width == NOT_FOUND)
          p->width = ng_atoi(number, i);
        else
          p->precision = ng_atoi(number, i);
        s--;   /* went to far go back */
        break;
    }
  }
}

static inline char *hex_byte_pack_upper(char *buf, uint8_t byte)
{
  const char hex_asc_upper[] = "0123456789ABCDEF";
#define hex_asc_upper_lo(x)  hex_asc_upper[((x) & 0x0f)]
#define hex_asc_upper_hi(x)  hex_asc_upper[((x) & 0xf0) >> 4]
  *buf++ = hex_asc_upper_hi(byte);
  *buf++ = hex_asc_upper_lo(byte);
  return buf;
}

static inline char *hex_byte_pack(char *buf, uint8_t byte)
{
  const char hex_asc[]       = "0123456789abcdef";
#define hex_asc_lo(x)  hex_asc[((x) & 0x0f)]
#define hex_asc_hi(x)  hex_asc[((x) & 0xf0) >> 4]
  *buf++ = hex_asc_hi(byte);
  *buf++ = hex_asc_lo(byte);
  return buf;
}
static void uuid_string(struct DATA *d, const uint8_t *addr)
{
/* UUID constants */

#define UUID_BUFFER_LENGTH          16  /* Length of UUID in memory */
#define UUID_STRING_LENGTH          36  /* Total length of a UUID string */

/* Positions for required hyphens (dashes) in UUID strings */

#define UUID_HYPHEN1_OFFSET         8
#define UUID_HYPHEN2_OFFSET         13
#define UUID_HYPHEN3_OFFSET         18
#define UUID_HYPHEN4_OFFSET         23
static const int guid_index[16] = {3, 2, 1, 0, 5, 4, 7, 6, 8, 9, 10, 11, 12, 13, 14, 15};
static const int uuid_index[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  char uuid[UUID_STRING_LENGTH + 1];
  char *p = uuid;
  int i;
  const uint8_t *index = (const uint8_t *)uuid_index;
  int8_t uc = ngrtos_FALSE;

  switch (*(++d->pf)) {
  case 'L':
    uc = ngrtos_TRUE;
    fallthrough;
  case 'l':
    index = (const uint8_t *)guid_index;
    break;
  case 'B':
    uc = ngrtos_TRUE;
    break;
  }

  for (i = 0; i < 16; i++) {
    if (uc)
      p = hex_byte_pack_upper(p, addr[index[i]]);
    else
      p = hex_byte_pack(p, addr[index[i]]);
    switch (i) {
    case 3:
    case 5:
    case 7:
    case 9:
      *p++ = '-';
      break;
    }
  }

  *p = 0;

  strings(d, uuid);
}

static void 
mac_address_string(struct DATA *d, const uint8_t *addr)
{
  char mac_addr[sizeof("xx:xx:xx:xx:xx:xx")];
  char *p = mac_addr;
  int i;
  char separator;
  int8_t reversed = ngrtos_FALSE;

  switch (d->pf[1]) {
  case 'F':
    separator = '-';
    break;

  case 'R':
    reversed = ngrtos_TRUE;
    fallthrough;

  default:
    separator = ':';
    break;
  }

  for (i = 0; i < 6; i++) {
    if (reversed)
      p = hex_byte_pack(p, addr[5 - i]);
    else
      p = hex_byte_pack(p, addr[i]);

    if (d->pf[0] == 'M' && i != 5)
      *p++ = separator;
  }
  *p = '\0';

  strings(d, mac_addr);
}

/*
 * Decimal conversion is by far the most typical, and is used for
 * /proc and /sys data. This directly impacts e.g. top performance
 * with many processes running. We optimize it for speed by emitting
 * two characters at a time, using a 200 byte lookup table. This
 * roughly halves the number of multiplications compared to computing
 * the digits one at a time. Implementation strongly inspired by the
 * previous version, which in turn used ideas described at
 * <http://www.cs.uiowa.edu/~jones/bcd/divide.html> (with permission
 * from the author, Douglas W. Jones).
 *
 * It turns out there is precisely one 26 bit fixed-point
 * approximation a of 64/100 for which x/100 == (x * (uint64_t)a) >> 32
 * holds for all x in [0, 10^8-1], namely a = 0x28f5c29. The actual
 * range happens to be somewhat larger (x <= 1073741898), but that's
 * irrelevant for our purpose.
 *
 * For dividing a number in the range [10^4, 10^6-1] by 100, we still
 * need a 32x32->64 bit multiply, so we simply use the same constant.
 *
 * For dividing a number in the range [100, 10^4-1] by 100, there are
 * several options. The simplest is (x * 0x147b) >> 19, which is valid
 * for all x <= 43698.
 */

static const uint16_t decpair[100] = {
#define _(x) (uint16_t) RTE_CPU_TO_BE_16(((x % 10) | ((x / 10) << 8)) + 0x3030)
	_( 0), _( 1), _( 2), _( 3), _( 4), _( 5), _( 6), _( 7), _( 8), _( 9),
	_(10), _(11), _(12), _(13), _(14), _(15), _(16), _(17), _(18), _(19),
	_(20), _(21), _(22), _(23), _(24), _(25), _(26), _(27), _(28), _(29),
	_(30), _(31), _(32), _(33), _(34), _(35), _(36), _(37), _(38), _(39),
	_(40), _(41), _(42), _(43), _(44), _(45), _(46), _(47), _(48), _(49),
	_(50), _(51), _(52), _(53), _(54), _(55), _(56), _(57), _(58), _(59),
	_(60), _(61), _(62), _(63), _(64), _(65), _(66), _(67), _(68), _(69),
	_(70), _(71), _(72), _(73), _(74), _(75), _(76), _(77), _(78), _(79),
	_(80), _(81), _(82), _(83), _(84), _(85), _(86), _(87), _(88), _(89),
	_(90), _(91), _(92), _(93), _(94), _(95), _(96), _(97), _(98), _(99),
#undef _
};

/*
 * This will print a single '0' even if r == 0, since we would
 * immediately jump to out_r where two 0s would be written but only
 * one of them accounted for in buf. This is needed by ip4_string
 * below. All other callers pass a non-zero value of r.
*/
static char *
put_dec_trunc8(char *buf, unsigned r)
{
	unsigned q;

	/* 1 <= r < 10^8 */
	if (r < 100)
		goto out_r;

	/* 100 <= r < 10^8 */
	q = (r * (uint64_t)0x28f5c29) >> 32;
	*((uint16_t *)buf) = decpair[r - 100*q];
	buf += 2;

	/* 1 <= q < 10^6 */
	if (q < 100)
		goto out_q;

	/*  100 <= q < 10^6 */
	r = (q * (uint64_t)0x28f5c29) >> 32;
	*((uint16_t *)buf) = decpair[q - 100*r];
	buf += 2;

	/* 1 <= r < 10^4 */
	if (r < 100)
		goto out_r;

	/* 100 <= r < 10^4 */
	q = (r * 0x147b) >> 19;
	*((uint16_t *)buf) = decpair[r - 100*q];
	buf += 2;
out_q:
	/* 1 <= q < 100 */
	r = q;
out_r:
	/* 1 <= r < 100 */
	*((uint16_t *)buf) = decpair[r];
  buf += r < 10 ? 1 : 2;
	return buf;
}

static char *
__ip4_string(char *p, const uint8_t *addr, const char *fmt)
{
  int i;
  int8_t leading_zeros = (fmt[0] == 'i');
  int index;
  int step;

  switch (fmt[2]) {
  case 'h':
#if __NG_BYTE_ORDER == __NG_BIG_ENDIAN
    index = 0;
    step = 1;
#else
    index = 3;
    step = -1;
#endif
    break;
  case 'l':
    index = 3;
    step = -1;
    break;
  case 'n':
  case 'b':
  default:
    index = 0;
    step = 1;
    break;
  }
  for (i = 0; i < 4; i++) {
    char temp[4];  /* hold each IP quad in reverse order */
    int digits = put_dec_trunc8(temp, addr[index]) - temp;
    if (leading_zeros) {
      if (digits < 3)
        *p++ = '0';
      if (digits < 2)
        *p++ = '0';
    }
    /* reverse the digits in the quad */
    while (digits--)
      *p++ = temp[digits];
    if (i < 3)
      *p++ = '.';
    index += step;
  }
  *p = '\0';

  return p;
}

void
ng_inet_ntop4(char *p, const uint8_t *addr)
{
  int i;
  int index = 0;
  int step  = 1;

  for (i = 0; i < 4; i++) 
  {
    char temp[4];  /* hold each IP quad in reverse order */
    int digits = put_dec_trunc8(temp, addr[index]) - temp;
    /* reverse the digits in the quad */
    while (digits--)
      *p++ = temp[digits];
    if (i < 3)
      *p++ = '.';
    index += step;
  }
  *p = '\0';
}

struct in_addr {
  uint32_t s_addr;
};
struct in6_addr {
  union {
    uint32_t u32_addr[4];
    uint16_t u16_addr[8];
    uint8_t  u8_addr[16];
  } un;
#define s6_addr    un.u8_addr
#define s6_addr16	 un.u16_addr
#define s6_addr32  un.u32_addr
};

struct sockaddr {
  uint8_t     sa_len;
  uint8_t     sa_family;
  char        sa_data[14];
};

/* members are in network byte order */
struct sockaddr_in {
  uint8_t        sin_len;
  uint8_t        sin_family;
  uint16_t       sin_port;
  struct in_addr  sin_addr;
#define SIN_ZERO_LEN 8
  char           sin_zero[SIN_ZERO_LEN];
};

struct sockaddr_in6 {
  uint8_t         sin6_len;      /* length of this structure    */
  uint8_t         sin6_family;   /* AF_INET6                    */
  uint16_t        sin6_port;     /* Transport layer port #      */
  uint32_t        sin6_flowinfo; /* IPv6 flow information       */
  struct in6_addr sin6_addr;     /* IPv6 address                */
  uint32_t        sin6_scope_id; /* Set of interfaces for scope */
};

/*
 * Note that we must __force cast these to unsigned long to make sparse happy,
 * since all of the endian-annotated types are fixed size regardless of arch.
 */
static inline int 
ipv6_addr_v4mapped(const struct in6_addr *a)
{
  return (
#if __NG_BITS_PER_LONG == 64
    *(unsigned long *)a |
#else
    (unsigned long)(a->s6_addr32[0] | a->s6_addr32[1]) |
#endif
    (unsigned long)(a->s6_addr32[2] ^
          rte_cpu_to_be_32(0x0000ffff))) == 0UL;
}

static inline int 
ipv6_addr_is_isatap(const struct in6_addr *addr)
{
  return (addr->s6_addr32[2] | rte_cpu_to_be_32(0x02000000)) 
    == rte_cpu_to_be_32(0x02005EFE);
}

static char *
ip6_compressed_string(char *p, const uint8_t *addr)
{
  const char hex_asc[] = "0123456789abcdef";
#define hex_asc_lo(x)	hex_asc[((x) & 0x0f)]
  int i, j, range;
  unsigned char zerolength[8];
  int longest = 1;
  int colonpos = -1;
  uint16_t word;
  uint8_t hi, lo;
  int8_t needcolon = ngrtos_FALSE;
  int8_t useIPv4;
  struct in6_addr in6;

  ng_memcpy(&in6, addr, sizeof(struct in6_addr));

  useIPv4 = ipv6_addr_v4mapped(&in6) || ipv6_addr_is_isatap(&in6);

  ng_memset(zerolength, 0, sizeof(zerolength));

  if (useIPv4)
    range = 6;
  else
    range = 8;

  /* find position of longest 0 run */
  for (i = 0; i < range; i++) {
    for (j = i; j < range; j++) {
      if (in6.s6_addr16[j] != 0)
        break;
      zerolength[i]++;
    }
  }
  for (i = 0; i < range; i++) {
    if (zerolength[i] > longest) {
      longest = zerolength[i];
      colonpos = i;
    }
  }
  if (longest == 1)    /* don't compress a single 0 */
    colonpos = -1;

  /* emit address */
  for (i = 0; i < range; i++) {
    if (i == colonpos) {
      if (needcolon || i == 0)
        *p++ = ':';
      *p++ = ':';
      needcolon = ngrtos_FALSE;
      i += longest - 1;
      continue;
    }
    if (needcolon) {
      *p++ = ':';
      needcolon = ngrtos_FALSE;
    }
    /* hex uint16_t without leading 0s */
    word = rte_ntohs(in6.s6_addr16[i]);
    hi = word >> 8;
    lo = word & 0xff;
    if (hi) {
      if (hi > 0x0f)
        p = hex_byte_pack(p, hi);
      else
        *p++ = hex_asc_lo(hi);
      p = hex_byte_pack(p, lo);
    }
    else if (lo > 0x0f)
      p = hex_byte_pack(p, lo);
    else
      *p++ = hex_asc_lo(lo);
    needcolon = ngrtos_TRUE;
  }

  if (useIPv4) {
    if (needcolon)
      *p++ = ':';
    p = __ip4_string(p, (const uint8_t *)&in6.s6_addr[12], "I4");
  }
  *p = '\0';

  return p;
}

static char *
ip6_string(char *p, const uint8_t *addr, const char *fmt)
{
  int i;

  for (i = 0; i < 8; i++) {
    p = hex_byte_pack(p, *addr++);
    p = hex_byte_pack(p, *addr++);
    if (fmt[0] == 'I' && i != 7)
      *p++ = ':';
  }
  *p = '\0';

  return p;
}

void
ng_inet_ntop6(char *ip6_addr, const uint8_t *addr, const char *fmt)
{
  if (fmt[0] == 'I' && fmt[2] == 'c')
    ip6_compressed_string(ip6_addr, addr);
  else
    ip6_string(ip6_addr, addr, fmt);
}

static void
__ip6_addr_string(struct DATA *d, const uint8_t *addr)
{
  char ip6_addr[sizeof("xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:255.255.255.255")];
  ng_inet_ntop6(ip6_addr, addr, d->pf);
  strings(d, ip6_addr);
}

static void
__ip4_addr_string(struct DATA *d, const uint8_t *addr)
{
  char ip4_addr[sizeof("255.255.255.255")];

  __ip4_string(ip4_addr, addr, d->pf);

  strings(d, ip4_addr);
}

static void
ip6_addr_string_sa(struct DATA *d, const struct sockaddr_in6 *sa)
{
  int8_t have_p = ngrtos_FALSE;
  int8_t have_s = ngrtos_FALSE;
  int8_t have_f = ngrtos_FALSE;
  int8_t have_c = ngrtos_FALSE;
  char ip6_addr[sizeof("[xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:255.255.255.255]") +
          sizeof(":12345") + sizeof("/123456789") +
          sizeof("%1234567890")];
  char *p = ip6_addr, *pend = ip6_addr + sizeof(ip6_addr);
  const uint8_t *addr = (const uint8_t *) &sa->sin6_addr;
  char fmt6[2] = { d->pf[0], '6' };
  uint8_t off = 0;

  d->pf++;
  while (isalpha(*++d->pf)) {
    switch (*d->pf) {
    case 'p':
      have_p = ngrtos_TRUE;
      break;
    case 'f':
      have_f = ngrtos_TRUE;
      break;
    case 's':
      have_s = ngrtos_TRUE;
      break;
    case 'c':
      have_c = ngrtos_TRUE;
      break;
    }
  }

  if (have_p || have_s || have_f) {
    *p = '[';
    off = 1;
  }

  if (fmt6[0] == 'I' && have_c)
    p = ip6_compressed_string(ip6_addr + off, addr);
  else
    p = ip6_string(ip6_addr + off, addr, fmt6);

  if (have_p || have_s || have_f)
    *p++ = ']';

  if (have_p) {
    *p++ = ':';
    p = ng_itoa(rte_ntohs(sa->sin6_port), p, pend - p);
  }
  if (have_f) {
    *p++ = '/';
#define IPV6_FLOWINFO_MASK    rte_cpu_to_be_32(0x0FFFFFFF)
    p = ng_itoa(rte_ntohl(sa->sin6_flowinfo & IPV6_FLOWINFO_MASK),
            p, pend - p);
  }
  if (have_s) 
  {
    *p++ = '%';
    p = ng_itoa(sa->sin6_scope_id, p, pend - p);
  }
  *p = '\0';

  strings(d, ip6_addr);
}

static void
ip4_addr_string_sa(struct DATA *d, const struct sockaddr_in *sa)
{
  int8_t have_p = ngrtos_FALSE;
  char *p, ip4_addr[sizeof("255.255.255.255") + sizeof(":12345")];
  char *pend = ip4_addr + sizeof(ip4_addr);
  const uint8_t *addr = (const uint8_t *) &sa->sin_addr.s_addr;
  char fmt4[3] = { d->pf[0], '4', 0 };

  d->pf++;
  while (isalpha(*++d->pf)) {
    switch (*d->pf) {
    case 'p':
      have_p = ngrtos_TRUE;
      break;
    case 'h':
    case 'l':
    case 'n':
    case 'b':
      fmt4[2] = *d->pf;
      break;
    }
  }

  p = __ip4_string(ip4_addr, addr, fmt4);
  if (have_p) {
    *p++ = ':';
    p = ng_itoa(rte_ntohs(sa->sin_port), p, pend - p);
  }
  *p = '\0';

  strings(d, ip4_addr);
}

static void
ip_addr_string(struct DATA *d, const void *ptr)
{
  char *err_fmt_msg;

  switch (d->pf[1]) {
  case '6':
    __ip6_addr_string(d, (const uint8_t *)ptr);
    return;
  case '4':
    __ip4_addr_string(d, (const uint8_t *)ptr);
    return;
  case 'S': {
    const union {
      struct sockaddr    raw;
      struct sockaddr_in  v4;
      struct sockaddr_in6  v6;
    } *sa = ptr;

#define AF_INET         2
#define AF_INET6        10
    switch (sa->raw.sa_family) {
    case AF_INET:
      ip4_addr_string_sa(d, &sa->v4);
      return;
    case AF_INET6:
      ip6_addr_string_sa(d, &sa->v6);
      return;
    default:
      strings(d, "(einval)");
      return;
    }}
  }

  err_fmt_msg = d->pf[0] == 'i' ? "(%pi?)" : "(%pI?)";
  strings(d, err_fmt_msg);
}

/*
 * Show a '%p' thing.  A kernel extension is that the '%p' is followed
 * by an extra set of alphanumeric characters that are extended format
 * specifiers.
 *
 * Please update scripts/checkpatch.pl when adding/removing conversion
 * characters.  (Search for "check for vsprintf extension").
 *
 * Right now we handle:
 *
 * - 'M' For a 6-byte MAC address, it prints the address in the
 *       usual colon-separated hex notation
 * - 'm' For a 6-byte MAC address, it prints the hex address without colons
 * - 'MF' For a 6-byte MAC FDDI address, it prints the address
 *       with a dash-separated hex notation
 * - '[mM]R' For a 6-byte MAC address, Reverse order (Bluetooth)
 * - 'I' [46] for IPv4/IPv6 addresses printed in the usual way
 *       IPv4 uses dot-separated decimal without leading 0's (1.2.3.4)
 *       IPv6 uses colon separated network-order 16 bit hex with leading 0's
 *       [S][pfs]
 *       Generic IPv4/IPv6 address (struct sockaddr *) that falls back to
 *       [4] or [6] and is able to print port [p], flowinfo [f], scope [s]
 * - 'i' [46] for 'raw' IPv4/IPv6 addresses
 *       IPv6 omits the colons (01020304...0f)
 *       IPv4 uses dot-separated decimal with leading 0's (010.123.045.006)
 *       [S][pfs]
 *       Generic IPv4/IPv6 address (struct sockaddr *) that falls back to
 *       [4] or [6] and is able to print port [p], flowinfo [f], scope [s]
 * - '[Ii][4S][hnbl]' IPv4 addresses in host, network, big or little endian order
 * - 'I[6S]c' for IPv6 addresses printed as specified by
 *       https://tools.ietf.org/html/rfc5952
 * - 'E[achnops]' For an escaped buffer, where rules are defined by combination
 *                of the following flags (see string_escape_mem() for the
 *                details):
 *                  a - ESCAPE_ANY
 *                  c - ESCAPE_SPECIAL
 *                  h - ESCAPE_HEX
 *                  n - ESCAPE_NULL
 *                  o - ESCAPE_OCTAL
 *                  p - ESCAPE_NP
 *                  s - ESCAPE_SPACE
 *                By default ESCAPE_ANY_NP is used.

 * - 'U' For a 16 byte UUID/GUID, it prints the UUID/GUID in the form
 *       "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
 *       Options for %pU are:
 *         b big endian lower case hex (default)
 *         B big endian UPPER case hex
 *         l little endian lower case hex
 *         L little endian UPPER case hex
 *           big endian output byte order is:
 *             [0][1][2][3]-[4][5]-[6][7]-[8][9]-[10][11][12][13][14][15]
 *           little endian output byte order is:
 *             [3][2][1][0]-[5][4]-[7][6]-[8][9]-[10][11][12][13][14][15]

 * - 't[RT][dt][r][s]' For time and date as represented by:
 *      R    struct rtc_time
 *      T    time64_t
 * - 'C' For a clock, it prints the name (Common Clock Framework) or address
 *       (legacy clock framework) of the clock
 * - 'Cn' For a clock, it prints the name (Common Clock Framework) or address
 *        (legacy clock framework) of the clock
 * - 'G' For flags to be printed as a collection of symbolic strings that would
 *       construct the specific value. Supported flags given by option:
 *       p page flags (see struct page) given as pointer to unsigned long
 *       g gfp flags (GFP_* and __GFP_*) given as pointer to gfp_t
 *       v vma flags (VM_*) given as pointer to unsigned long
 *
 * ** When making changes please also update:
 *  Documentation/core-api/printk-formats.rst
 *
 * Note: The default behaviour (unadorned %p) is to hash the address,
 * rendering it useful as a unique identifier.
 */
PRIVATE void
pointer(struct DATA *d, const void *ptr)
{
  switch (*d->pf) {
  case 'M':      /* Colon separated: 00:01:02:03:04:05 */
  case 'm':      
    /* Contiguous: 000102030405 */
    /* [mM]F (FDDI) */
    /* [mM]R (Reverse order; Bluetooth) */
    mac_address_string(d, (const uint8_t *)ptr);
    break;
  case 'I':      
    /* Formatted IP supported
     * 4:  1.2.3.4
     * 6:  0001:0203:...:0708
     * 6c:  1::708 or 1::1.2.3.4
     */
  case 'i':      
     /* Contiguous:
     * 4:  001.002.003.004
     * 6:   000102...0f
     */
    ip_addr_string(d, ptr);
    return;
  case 'U':
    uuid_string(d, (const uint8_t *)ptr);
    return;
  case 'u':
  case 'k':
    switch (d->pf[1]) {
    case 's':
      strings(d, (const char *)ptr);
      return;
    default:
      strings(d, "(einval)");
      return;
    }
  default:
    hexa(d, (const double)(ng_intptr_t)ptr);
    return;
  }
}

PRIVATE void
bitsstr(struct DATA *d, unsigned LONG_LONG _uquad, const char *b)
{
  char *z, *e;
  int tmp, n;
  char buf[512];

  z = buf;
  e = buf + sizeof(buf);
  
  if (*b == 8)
    z+=ng_snprintf(buf, sizeof buf, "%llo", _uquad);
  else if (*b == 10)
  {
    u64toa_jeaiii(_uquad, z);
    z+=ng_strlen(z);
  }
  else if (*b == 16)
  {
    u64toh_jeaiii(_uquad, z, ngrtos_FALSE);
    z+=ng_strlen(z);
  }
  else
    return;
  
  b++;
  
  if (_uquad) 
  {
    tmp = 0;
    while ((n = *b++) != 0) 
    {
      if (n & 0x80)
        n &= 0x7f;
      else if (n <= ' ')
        n = n - 1;
      if (_uquad & (1LL << n)) 
      {
        if (z < e)
          *z++ = (tmp ? ',':'<');
        while (*b > ' ' &&
            (*b & 0x80) == 0) 
        {
          if (z < e)
            *z++ = *b;
          b++;
        }
        tmp = 1;
      } 
      else 
      {
        while (*b > ' ' &&
            (*b & 0x80) == 0)
          b++;
      }
    }
    if (tmp) 
    {
      if (z < e)
        *z++ = '>';
    }
  }

  if (z < e)
    *z = '\0';
  else
    *(z-1) = '\0';
  strings(d, buf);
}

PRIVATE int
__ng_vsnprintf(struct DATA *data, va_list args)
{
  char conv_field[MAX_FIELD];
  double d; /* temporary holder */
  int state;
  int i;

  for (; *data->pf && (data->counter < data->length); data->pf++) 
  {
    /* we got a magic % cookie */
    if ( *data->pf == '%' ) 
    { 
      conv_flag((char *)0, data); /* initialise format flags */
      for (state = 1; *data->pf && state;) {
        switch (*(++data->pf)) {
          case '\0': /* a NULL here ? ? bail out */
            *data->holder = '\0';
            return data->counter;
            break;
          case 'f':  /* float, double */
            STAR_ARGS(data);
            if (data->a_long == FOUND)
               d = va_arg(args, LONG_DOUBLE);
            else
               d = va_arg(args, double);
            floating(data, d);
            state = 0;
            break;
          case 'g':
          case 'G':
            STAR_ARGS(data);
            DEF_PREC(data);
            if (data->a_long == FOUND)
              d = va_arg(args, LONG_DOUBLE);
            else
              d = va_arg(args, double);
            i = log_10(d);
            /*
             * for '%g|%G' ANSI: use f if exponent
             * is in the range or [-4,p] exclusively
             * else use %e|%E
             */
            if (-4 < i && i < data->precision)
              floating(data, d);
            else
              exponent(data, d);
            state = 0;
            break;
          case 'e':
          case 'E':  /* Exponent double */
            STAR_ARGS(data);
            if (data->a_long == FOUND)
               d = va_arg(args, LONG_DOUBLE);
            else
               d = va_arg(args, double);
            exponent(data, d);
            state = 0;
            break;
          case 'u':  /* unsigned decimal */
            STAR_ARGS(data);
            if (data->a_longlong == FOUND)
              d = va_arg(args, unsigned LONG_LONG);
            else if (data->a_long == FOUND)
              d = va_arg(args, unsigned long);
            else
              d = va_arg(args, unsigned int);
            decimal(data, d);
            state = 0;
            break;
          case 'd':  /* decimal */
            STAR_ARGS(data);
            if (data->a_longlong == FOUND)
              d = va_arg(args, LONG_LONG);
            else if (data->a_long == FOUND)
              d = va_arg(args, long);
            else
              d = va_arg(args, int);
            decimal(data, d);
            state = 0;
            break;
          case 'o':  /* octal */
            STAR_ARGS(data);
            if (data->a_longlong == FOUND)
              d = va_arg(args, LONG_LONG);
            else if (data->a_long == FOUND)
              d = va_arg(args, long);
            else
              d = va_arg(args, int);
            octal(data, d);
            state = 0;
            break;
          case 'x':
          case 'X':  /* hexadecimal */
            STAR_ARGS(data);
            if (data->a_longlong == FOUND)
              d = va_arg(args, LONG_LONG);
            else if (data->a_long == FOUND)
              d = va_arg(args, long);
            else
              d = va_arg(args, int);
            hexa(data, d);
            state = 0;
            break;
          case 'c': /* character */
            d = va_arg(args, int);
            PUT_CHAR(d, data);
            state = 0;
            break;
          case 's':  /* string */
            STAR_ARGS(data);
            strings(data, va_arg(args, char *));
            state = 0;
            break;
          case 'n':
            /* what's the count ? */
            *(va_arg(args, int *)) = data->counter; 
            state = 0;
            break;
          case 'p':
            STAR_ARGS(data);
            if (data->pf[1] == 'b') 
            {
              unsigned LONG_LONG u = va_arg(args, unsigned LONG_LONG);
              const char *n = va_arg(args, const char *);
              bitsstr(data, u, n);
            }
            else
              pointer(data, va_arg(args, const void *));
            state = 0;
            break;
          case 'q':
            data->a_longlong = FOUND;
            break;
          case 'L':
          case 'l':
            if (data->a_long == FOUND)
              data->a_longlong = FOUND;
            else
              data->a_long = FOUND;
            break;
          case 'h':
            break;
          case '%':  /* nothing just % */
            PUT_CHAR('%', data);
            state = 0;
            break;
          case '#': case ' ': case '+': case '*':
          case '-': case '.': case '0': case '1':
          case '2': case '3': case '4': case '5':
          case '6': case '7': case '8': case '9':
           /* initialize width and precision */
            for (i = 0; isflag(*data->pf); i++, data->pf++)
              if (i < MAX_FIELD - 1)
          conv_field[i] = *data->pf;
            conv_field[i] = '\0';
            conv_flag(conv_field, data);
            data->pf--;   /* went to far go back */
            break;
          default:
            /* is this an error ? maybe bail out */
            state = 0;
            break;
        } /* end switch */
      } /* end of for state */
    } 
    else 
    { /* not % */
      PUT_CHAR(*data->pf, data);  /* add the char the string */
    }
  }

  *data->holder = '\0'; /* the end ye ! */

  return data->counter;
}

PUBLIC int
ng_vsnprintf(char *string, size_t length, char const * format, 
  va_list args)
{
  struct DATA data;
  data.length  = length - 1; /* leave room for '\0' */
  data.holder  = string;
  data.pf      = format;
  data.counter = 0;
  return __ng_vsnprintf(&data, args);
}

PUBLIC int
ng_snprintf(char *string, size_t length, char const * format, ...)
{
  int rval;
  struct DATA data;
  va_list args;

  data.length  = length - 1; /* leave room for '\0' */
  data.holder  = string;
  data.pf      = format;
  data.counter = 0;

  va_start(args, format);

  rval = __ng_vsnprintf (&data, args);

  va_end(args);

  return rval;
}


#ifdef DRIVER

#include <stdio.h>

/* set of small tests for snprintf() */
int main()
{
  char holder[100];
  int i;

/*
  printf("Suite of test for snprintf:\n");
  printf("a_format\n");
  printf("printf() format\n");
  printf("snprintf() format\n\n");
*/
/* Checking the field widths */

  printf("/%%d/, 336\n");
  snprintf(holder, sizeof holder, "/%d/\n", 336);
  printf("/%d/\n", 336);
  printf("%s\n", holder);

  printf("/%%2d/, 336\n");
  snprintf(holder, sizeof holder, "/%2d/\n", 336);
  printf("/%2d/\n", 336);
  printf("%s\n", holder);

  printf("/%%10d/, 336\n");
  snprintf(holder, sizeof holder, "/%10d/\n", 336);
  printf("/%10d/\n", 336);
  printf("%s\n", holder);

  printf("/%%-10d/, 336\n");
  snprintf(holder, sizeof holder, "/%-10d/\n", 336);
  printf("/%-10d/\n", 336);
  printf("%s\n", holder);

/* long long  */

  printf("/%%lld/, 336\n");
  snprintf(holder, sizeof holder, "/%lld/\n", (LONG_LONG)336);
  printf("/%lld/\n", (LONG_LONG)336);
  printf("%s\n", holder);

  printf("/%%2qd/, 336\n");
  snprintf(holder, sizeof holder, "/%2qd/\n", (LONG_LONG)336);
  printf("/%2qd/\n", (LONG_LONG)336);
  printf("%s\n", holder);

/* floating points */

  printf("/%%f/, 1234.56\n");
  snprintf(holder, sizeof holder, "/%f/\n", 1234.56);
  printf("/%f/\n", 1234.56);
  printf("%s\n", holder);

  printf("/%%e/, 1234.56\n");
  snprintf(holder, sizeof holder, "/%e/\n", 1234.56);
  printf("/%e/\n", 1234.56);
  printf("%s\n", holder);

  printf("/%%4.2f/, 1234.56\n");
  snprintf(holder, sizeof holder, "/%4.2f/\n", 1234.56);
  printf("/%4.2f/\n", 1234.56);
  printf("%s\n", holder);

  printf("/%%3.1f/, 1234.56\n");
  snprintf(holder, sizeof holder, "/%3.1f/\n", 1234.56);
  printf("/%3.1f/\n", 1234.56);
  printf("%s\n", holder);

  printf("/%%10.3f/, 1234.56\n");
  snprintf(holder, sizeof holder, "/%10.3f/\n", 1234.56);
  printf("/%10.3f/\n", 1234.56);
  printf("%s\n", holder);

  printf("/%%10.3e/, 1234.56\n");
  snprintf(holder, sizeof holder, "/%10.3e/\n", 1234.56);
  printf("/%10.3e/\n", 1234.56);
  printf("%s\n", holder);

  printf("/%%+4.2f/, 1234.56\n");
  snprintf(holder, sizeof holder, "/%+4.2f/\n", 1234.56);
  printf("/%+4.2f/\n", 1234.56);
  printf("%s\n", holder);

  printf("/%%010.2f/, 1234.56\n");
  snprintf(holder, sizeof holder, "/%010.2f/\n", 1234.56);
  printf("/%010.2f/\n", 1234.56);
  printf("%s\n", holder);

#define BLURB "Outstanding acting !"
/* strings precisions */

  printf("/%%2s/, \"%s\"\n", BLURB);
  snprintf(holder, sizeof holder, "/%2s/\n", BLURB);
  printf("/%2s/\n", BLURB);
  printf("%s\n", holder);

  printf("/%%22s/ %s\n", BLURB);
  snprintf(holder, sizeof holder, "/%22s/\n", BLURB);
  printf("/%22s/\n", BLURB);
  printf("%s\n", holder);

  printf("/%%22.5s/ %s\n", BLURB);
  snprintf(holder, sizeof holder, "/%22.5s/\n", BLURB);
  printf("/%22.5s/\n", BLURB);
  printf("%s\n", holder);

  printf("/%%-22.5s/ %s\n", BLURB);
  snprintf(holder, sizeof holder, "/%-22.5s/\n", BLURB);
  printf("/%-22.5s/\n", BLURB);
  printf("%s\n", holder);

/* see some flags */

  printf("%%x %%X %%#x, 31, 31, 31\n");
  snprintf(holder, sizeof holder, "%x %X %#x\n", 31, 31, 31);
  printf("%x %X %#x\n", 31, 31, 31);
  printf("%s\n", holder);

  printf("**%%d**%% d**%% d**, 42, 42, -42\n");
  snprintf(holder, sizeof holder, "**%d**% d**% d**\n", 42, 42, -42);
  printf("**%d**% d**% d**\n", 42, 42, -42);
  printf("%s\n", holder);

/* other flags */

  printf("/%%g/, 31.4\n");
  snprintf(holder, sizeof holder, "/%g/\n", 31.4);
  printf("/%g/\n", 31.4);
  printf("%s\n", holder);

  printf("/%%.6g/, 31.4\n");
  snprintf(holder, sizeof holder, "/%.6g/\n", 31.4);
  printf("/%.6g/\n", 31.4);
  printf("%s\n", holder);

  printf("/%%.1G/, 31.4\n");
  snprintf(holder, sizeof holder, "/%.1G/\n", 31.4);
  printf("/%.1G/\n", 31.4);
  printf("%s\n", holder);

  printf("abc%%n\n");
  printf("abc%n", &i); printf("%d\n", i);
  snprintf(holder, sizeof holder, "abc%n", &i);
  printf("%s", holder); printf("%d\n\n", i);

  printf("%%*.*s --> 10.10\n");
  snprintf(holder, sizeof holder, "%*.*s\n", 10, 10, BLURB);
  printf("%*.*s\n", 10, 10, BLURB);
  printf("%s\n", holder);

  printf("%%%%%%%%\n");
  snprintf(holder, sizeof holder, "%%%%\n");
  printf("%%%%\n");
  printf("%s\n", holder);

#define BIG "Hello this is a too big string for the buffer"
/*  printf("A buffer to small of 10, trying to put this:\n");*/
  printf("<%%>, %s\n", BIG);
  i = snprintf(holder, 10, "%s\n", BIG);
  printf("<%s>\n", BIG);
  printf("<%s>\n", holder);

  return 0;
}
#endif /* !DRIVER */
