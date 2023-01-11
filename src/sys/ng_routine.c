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
#include "ng_config.h"

#if configEnable_COROUTINE
#include "ng_defs.h"
#include "ng_port.h"
#include "ng_limits.h"
#include "ng_prio.h"
#include "ng_list.h"
#include "ng_routine.h"
#include "ng_task.h"
#include "mem/ng_mem.h"

typedef struct __schedule  ng_schedule_s;
typedef struct __coroutine ng_coroutine_s;

struct __schedule {
  ng_list_s ready;
  int count;
  ng_coroutine_s *running;
};

struct __coroutine {
  ng_task_func_f func;
  void          *arg;
  uint16_t       priority;
  uint16_t       status;
  ng_schedule_s *sch;
  ng_string_s    name;
  ng_list_s      link;
};

static ng_schedule_s *croutine_sched;

static ng_coroutine_s * 
_co_new(ng_schedule_s *S, int priority, 
  ng_task_func_f func, void *arg, const char *name, 
  int len) 
{
  ng_coroutine_s *co;
  
  co = (ng_coroutine_s *)ng_malloc(sizeof(*co));
  if (co == NULL)
  {
    NGRTOS_ERROR("Failed to alloc ng_coroutine_s.\n");
    return NULL;
  }
  co->priority    = priority;
  co->sch         = S;
  co->status      = COROUTINE_READY;
  co->name.prt    = name;
  co->name.len    = len;
  return co;
}

__STATIC_FORCEINLINE void
_co_delete(ng_schedule_s *S, ng_coroutine_s *co) 
{
  ngIrqTypeT b = __ng_sys_disable_irq();
  NGRTOS_ASSERT(co->status == COROUTINE_READY);
  list_del(&co->link);
  __ng_sys_enable_irq(b);
  ng_free(co);
}

static ng_coroutine_s * 
__coroutine_new(ng_schedule_s *S, int16_t priority,
  ng_task_func_f func, void *ud, const char *name, int len) 
{
  ng_coroutine_s *co;

  co = _co_new(S, priority, func, ud, name, len);
  if (co != NULL)
  {
    ngIrqTypeT b = __ng_sys_disable_irq();
    list_add_tail(&co->link, &S->ready);
    __ng_sys_enable_irq(b);
    return co;
  }
  return NULL;
}

static ng_coroutine_s * 
__coroutine_running(ng_schedule_s *S)
{
  return S->running;
}

ng_coroutine_s * 
ng_coroutine_new(int16_t priority, ng_task_func_f func, 
  void *ud, const char *name, int len) 
{
  ng_schedule_s *S = (ng_schedule_s *)__ngrtos_get_task_arg();
  if (S != croutine_sched)
    return NULL;
  return __coroutine_new(S, priority, func, ud, name, len);
}

ng_result_e
ng_coroutine_del(ng_coroutine_s *co) 
{
  ng_schedule_s *S = (ng_schedule_s *)__ngrtos_get_task_arg();
  if (S != croutine_sched)
    return NG_result_failed;
  return _co_delete(S, co);
}

ng_coroutine_s *
ng_coroutine_running(void)
{
  ng_schedule_s *S = (ng_schedule_s *)__ngrtos_get_task_arg();
  if (S != croutine_sched)
    return NG_result_failed;
  return __coroutine_running(S);
}

static void 
__ng_coroutine_main(void *arg)
{
  ng_schedule_s *S = (ng_schedule_s *)arg;
  
  while (ngrtos_TRUE)
  {
    ng_list_s *e;
    ngIrqTypeT b;

    b = __ng_sys_disable_irq();
    e = ng_pri_get(&S->ready);
    __ng_sys_enable_irq(b);

    if (e != NULL)
    {
      ng_coroutine_s *C;
      C = list_entry(e,ng_coroutine_s, link);
      C->status = COROUTINE_RUNNING;
      C->func(C->arg);
      C->status = COROUTINE_READY;
      list_add_tail(&C->link, &S->ready);
    }
    else
    {
      ng_os_delay(100);
    }
  }
}

static void 
__ng_coroutine_close(void *arg) 
{
  ng_schedule_s *S;

  S = (ng_schedule_s *)arg;
  if (S == NULL)
    return;
  ng_free(S);
}

void 
ng_coroutine_close(ng_handle_t h) 
{
  ngrtos_task_delete(h);
}

ng_handle_t
ng_coroutine_open(int task_pri) 
{
  ng_handle_t h;
  ng_schedule_s *S;

  S = (ng_schedule_s *)ng_malloc(sizeof(*S));
  if (S == NULL)
  {
    NGRTOS_ERROR("Failed to alloc ng_schedule_s.\n");
    goto err0;
  }
  ng_pri_init(&S->ready);
  S->running = NULL;
  
  ng_task_init_s _init = {
      .name      = "COROUTINE",
      .func      = __ng_coroutine_main, 
      .finish    = __ng_coroutine_close,
      .arg       = S,
      ._sp       = NULL,
      .sp_size   = ngRTOS_MINIMAL_STACK_SIZE<<3,
      .priority  = task_pri < 0 ? ngRTOS_PRIO_LOWEST : task_pri,
      .ctrl      = NGRTOS_TCTRL_COROUTINE
    };

  h = ngrtos_task_create(&_init);
  if (h == NULL)
  {
    NGRTOS_ERROR("Failed to init system %s task.\n", _init.name);
    goto err1;
  }

  croutine_sched = S;
  NGRTOS_EVENT("Init system %s task: OK.\n", _init.name);
  return h;

err1:
  ng_free(S);
err0:
  return NULL;
}

#endif
