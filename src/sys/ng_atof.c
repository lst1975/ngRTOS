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
 *                              
 *                              https://github.com/ngRTOS
 **************************************************************************************
 */

/*
   function : GT_atof
   description:   attempt at a simple, fast atof routine
                  (for known magnitude numbers)
                  Conversion from text to floating point.

   assume: 1. all numbers will be smaller/larger than 1.0E +/-19
           2. there are no scientific notation type strings
           3. leading blanks (spaces) have been removed
           4. negative sign must appear in the first character (if it does at all)
   comments:  this routine may be up to 3 times faster than stdlib's atof or sscanf




*/

#include <stdint.h>
#include "ng_arch.h"
#include "ng_defs.h"
#include "ng_ctype.h"
#include "ng_string.h"

struct dpair{
  double d;
  double m;
};
static struct dpair ptable[]={
  { (double)0, 1},
  { (double)('0'), 1 },
  { (double)('0')* 10, 10 },
  { (double)('0')* 100, 100 },
  { (double)('0')* 1000, 1000 },
  { (double)('0')* 10000, 10000 },
  { (double)('0')* 100000, 100000 },
  { (double)('0')* 1000000, 1000000 },
  { (double)('0')* 10000000, 10000000 },
  { (double)('0')* 100000000.0, 100000000.0 },
  { (double)('0')* 1000000000.0, 1000000000.0 },
  { (double)('0')* 10000000000.0, 10000000000.0 },
  { (double)('0')* 100000000000.0, 100000000000.0 },
  { (double)('0')* 1000000000000.0, 1000000000000.0 },
  { (double)('0')* 10000000000000.0, 10000000000000.0 },
  { (double)('0')* 100000000000000.0, 100000000000000.0 },
  { (double)('0')* 1000000000000000.0, 1000000000000000.0 },
  { (double)('0')* 10000000000000000.0, 10000000000000000.0 },
  { (double)('0')* 100000000000000000.0, 100000000000000000.0 },
  { (double)('0')* 1000000000000000000.0, 1000000000000000000.0 },
};
static struct dpair ntable[]={
  { (double)0, 1},
  { (double)('0')* 0.1, 0.1 },
  { (double)('0')* 0.01, 0.01 },
  { (double)('0')* 0.001, 0.001 },
  { (double)('0')* 0.0001, 0.0001 },
  { (double)('0')* 0.00001, 0.00001 },
  { (double)('0')* 0.000001, 0.000001 },
  { (double)('0')* 0.0000001, 0.0000001 },
  { (double)('0')* 0.00000001, 0.00000001 },
  { (double)('0')* 0.000000001, 0.000000001 },
  { (double)('0')* 0.0000000001, 0.0000000001 },
  { (double)('0')* 0.00000000001, 0.00000000001 },
  { (double)('0')* 0.000000000001, 0.000000000001 },
  { (double)('0')* 0.0000000000001, 0.0000000000001 },
  { (double)('0')* 0.00000000000001, 0.00000000000001 },
  { (double)('0')* 0.000000000000001, 0.000000000000001 },
  { (double)('0')* 0.0000000000000001, 0.0000000000000001 },
  { (double)('0')* 0.00000000000000001, 0.00000000000000001 },
  { (double)('0')* 0.000000000000000001, 0.000000000000000001 },
  { (double)('0')* 0.0000000000000000001, 0.0000000000000000001 },
};

double GT_atof(char *s)

{ double tf;
  int nc,i,neg;

  tf = 0.0;
  if (s[0] == '-')
  { neg = 1;
  }
  else
  { neg = 0;
  }
  /* search for decimal point */
  for (nc=neg;( isdigit(s[nc]) &&
              (s[nc] != '.') &&
              (s[nc] != '\0')); nc++) ;
  for (i=neg; ( (isdigit(s[i]) || (s[i] == '.')) &&
                (s[i] != '\0') ) ;i++)
  {
    /* printf("%c %d %lf\n",s[i],nc-i,tf); */
    int k = nc - i;
    if (k > 0)
    {
      if (k < 20)
        tf += (s[i]*ptable[k].m - ptable[k].d);
    }
    else if (k == 0)
    {
    }
    else if (k > -20)
    {
      tf += (s[i]*ntable[-k].m - ntable[-k].d);
    }
  }

  if (neg) tf *= -1.0;
  if (_tolower(s[i]) == 'e')
  {
    int e;
    if ((s[++i]) == '-')
      nc = 0;
    else
      nc = 1;
    e = ng_fast_strtoui32(&s[i+1]);
    if (nc)
      for (i=0;i<e;i++)
        tf *= 10;
    else
      for (i=0;i<e;i++)
        tf /= 10;
  }
  return(tf);
}

