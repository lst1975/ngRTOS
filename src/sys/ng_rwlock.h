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

#ifndef __ngRTOS_RWLOCK_H__
#define __ngRTOS_RWLOCK_H__

#ifdef __cplusplus
    extern "C" {
#endif

#include "ng_rwlock.h"

typedef struct ng_rwlock ng_rwlock_s;

struct ng_rwlock{
  ng_tq_s rwl_tq;
#define rwl_waiters rwl_tq.waiters
#define rwl_name    rwl_tq.name
#define rwl_writers rwl_tq.flag
#define rwl_readers rwl_tq.count
#define rwl_wcnt    rwl_tq.wcnt
};

void ngrtos_rwlock_destroy(ng_rwlock_s *l);
ng_result_e ngrtos_rwlock_init(ng_rwlock_s *l, const ng_string_s *name);
void ngrtos_rwlock_read_enter(ng_rwlock_s *l);
void ngrtos_rwlock_read_leave(ng_rwlock_s *l);
void ngrtos_rwlock_write_enter(ng_rwlock_s *l);
void ngrtos_rwlock_write_leave(ng_rwlock_s *l);

#ifdef __cplusplus
    }
#endif

#endif /* __ngRTOS_ITOA_H__ */


