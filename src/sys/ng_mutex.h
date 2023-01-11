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

#ifndef __ngRTOS_MUTEX__
#define __ngRTOS_MUTEX__

#ifdef __cplusplus
    extern "C" {
#endif

#include "ng_tq.h"

typedef struct ng_mutex ng_mutex_s;

struct ng_mutex{
  ng_tq_s mux_tq;
#define mux_waiters mux_tq.waiters
#define mux_name    mux_tq.name
#define mux_flag    mux_tq.flag
#define mux_count   mux_tq.count
#define mux_wcnt    mux_tq.wcnt
};

void ngrtos_mutex_destroy(ng_mutex_s *mux);
ng_result_e ngrtos_mutex_init(ng_mutex_s *mux, const ng_string_s *name);
void ngrtos_mutex_enter(ng_mutex_s *mux);
void ngrtos_mutex_leave(ng_mutex_s *mux);

#ifdef __cplusplus
    }
#endif

#endif /* __ngRTOS_ITOA_H__ */


