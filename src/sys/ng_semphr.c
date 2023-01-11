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
#include "ng_semphr.h"

ng_result_e
ngrtos_sem_init(ng_sem_s *sem, const ng_string_s *name)
{
  ngrtos_tq_init(&sem->sem_tq, name);
  return NG_result_ok;
}

void
ngrtos_sem_destroy(ng_sem_s *sem)
{
  ng_handle_t *h;
  ngIrqTypeT x;

  do {
    x = __ng_sys_disable_irq();
    h = ngrtos_tq_get(&sem->sem_tq);
    __ng_sys_enable_irq(x);

    ngrtos_task_resume(h);
    
  } while(x != NULL);
}

void 
ngrtos_sem_wait(ng_sem_s *sem, int wait_ms)
{
  ngIrqTypeT x;

  NGRTOS_ASSERT(!IS_IRQ());

  x = __ng_sys_disable_irq();
  if (sem->sem_count)
  {
    sem->sem_count--;
    __ng_sys_enable_irq(x);
    return;
  }

  if (wait_ms == 0)
  {
    __ng_sys_enable_irq(x);
    return;
  }

  ngrtos_tq_add(&sem->sem_tq, NULL);

  if (wait_ms < 0)
  {
    __ng_sys_enable_irq(x);
    ngrtos_suspend();
    return;
  }

  __ng_sys_enable_irq(x);
  ng_os_delay(wait_ms);
}

void 
ngrtos_sem_wake(ng_sem_s *sem)
{
  ngIrqTypeT x;
  ng_handle_t t;

  x = __ng_sys_disable_irq();
  sem->sem_count++;
  t = ngrtos_tq_get(&sem->sem_tq);
  __ng_sys_enable_irq(x);

  if (t != NULL)
  {
    ngrtos_task_resume(t);
  }
}
