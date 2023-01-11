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
#include "ng_list.h"
#include "ng_limits.h"
#include "ng_task.h"
#include "ng_rwlock.h"

ng_result_e
ngrtos_rwlock_init(ng_rwlock_s *l, const ng_string_s *name)
{
  ngrtos_tq_init(&l->rwl_tq, name);
  return NG_result_ok;
}

void
ngrtos_rwlock_destroy(ng_rwlock_s *l)
{
  ng_handle_t *h;
  ngIrqTypeT x;

  do {
    x = __ng_sys_disable_irq();
    h = ngrtos_tq_get(&l->rwl_tq);
    __ng_sys_enable_irq(x);

    ngrtos_task_resume(h);
    
  } while(x != NULL);
}

void 
ngrtos_rwlock_reader_enter(ng_rwlock_s *l)
{
  NGRTOS_ASSERT(!IS_IRQ());

  ngIrqTypeT x;

  x = __ng_sys_disable_irq();
  if (l->rwl_writers)
  {
    ngrtos_tq_add(&l->rwl_tq, NULL);
    __ng_sys_enable_irq(x);
    ngrtos_suspend();
  }
  else
  {
    l->rwl_readers++;
    __ng_sys_enable_irq(x);
  }
}

void 
ngrtos_rwlock_reader_leave(ng_rwlock_s *l)
{
  ngIrqTypeT x;
  ng_handle_t t;

  NGRTOS_ASSERT(!IS_IRQ());

  x = __ng_sys_disable_irq();
  l->rwl_readers--;
  t = ngrtos_tq_get(&l->rwl_tq);
  __ng_sys_enable_irq(x);

  if (t != NULL)
  {
    ngrtos_task_resume(t);
  }
}

void 
ngrtos_rwlock_writer_enter(ng_rwlock_s *l)
{
  NGRTOS_ASSERT(!IS_IRQ());

  ngIrqTypeT x;

  x = __ng_sys_disable_irq();
  if (l->rwl_writers || l->rwl_readers)
  {
    ngrtos_tq_add(&l->rwl_tq, NULL);
    __ng_sys_enable_irq(x);
    ngrtos_suspend();
  }
  else
  {
    l->rwl_writers++;
    __ng_sys_enable_irq(x);
  }
}

void 
ngrtos_rwlock_writer_leave(ng_rwlock_s *l)
{
  ngIrqTypeT x;
  ng_handle_t t;

  x = __ng_sys_disable_irq();
  NGRTOS_ASSERT(l->rwl_writers == 1);
  l->rwl_writers--;
  t = ngrtos_tq_get(&l->rwl_tq);
  __ng_sys_enable_irq(x);

  if (t != NULL)
  {
    ngrtos_task_resume(t);
  }
}
