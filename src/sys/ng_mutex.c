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
#include "ng_mutex.h"

ng_result_e
ngrtos_mutex_init(ng_mutex_s *mux, const ng_string_s *name)
{
  NGRTOS_ASSERT(!IS_IRQ());
  ngrtos_tq_init(&mux->mux_tq, name);
  return NG_result_ok;
}

void
ngrtos_mutex_destroy(ng_mutex_s *mux)
{
  ng_handle_t *h;
  ngIrqTypeT x;

  NGRTOS_ASSERT(!IS_IRQ());
  
  do {
    ngrtos_mutex_enter(mux);
    __DMB();
    x = __ng_sys_disable_irq();
    h = ngrtos_tq_get(&mux->mux_tq);
    __ng_sys_enable_irq(x);
    __DMB();
    ngrtos_mutex_leave(mux);

    ngrtos_task_resume(h);
    
  } while(x != NULL);
}

void 
ngrtos_mutex_enter(ng_mutex_s *mux)
{
  ngIrqTypeT x;

  x = __ng_sys_disable_irq();
  if (!IS_IRQ() && mux->mux_count)
  {
    ngrtos_tq_add(&mux->mux_tq, NULL);
    __ng_sys_enable_irq(x);
    ngrtos_suspend();
  }
  else
  {
    mux->mux_count++;
    __ng_sys_enable_irq(x);
  }
}

void 
ngrtos_mutex_leave(ng_mutex_s *mux)
{
  ngIrqTypeT x;
  ng_handle_t t;

  x = __ng_sys_disable_irq();
  mux->mux_count--;
  if (IS_IRQ() || mux->mux_count)
  {
    __ng_sys_enable_irq(x);
    return;
  }
  
  t = ngrtos_tq_get(&mux->mux_tq);
  __ng_sys_enable_irq(x);

  if (t != NULL)
  {
    ngrtos_task_resume(t);
  }
}

