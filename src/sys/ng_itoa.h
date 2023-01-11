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
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *************************************************************************************
 *                              https://www.ngRTOS.org
 *                              https://github.com/ngRTOS
 **************************************************************************************
 */

#ifndef __ngRTOS_ITOA_H__
#define __ngRTOS_ITOA_H__

#ifdef __cplusplus
    extern "C" {
#endif

void u32toa_jeaiii(uint32_t u, char* b);
void i32toa_jeaiii(int32_t i, char* b);
void u64toa_jeaiii(uint64_t n, char* b);
void i64toa_jeaiii(int64_t i, char* b);
uint32_t u32toh_jeaiii(uint64_t num, char *s, ng_bool_t lowerAlpha);
uint32_t u64toh_jeaiii(uint64_t num, char *s, ng_bool_t lowerAlpha);
void ftoa_jeaiii(double i, char* b);

double ng_modf(double x, double *iptr);
float ng_modff(float x, float *iptr);

#ifdef __cplusplus
    }
#endif

#endif /* __ngRTOS_ITOA_H__ */

