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

/*  $OpenBSD: kern_timeout.c,v 1.85 2021/06/19 02:05:33 cheloha Exp $  */
/*
 * Copyright (c) 2001 Thomas Nordin <nordin@openbsd.org>
 * Copyright (c) 2000-2001 Artur Grabowski <art@openbsd.org>
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 *
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL  DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "ng_config.h"

#if configEnable_TIMER_WHEEL

#include "ng_defs.h"
#include "ng_task.h"
#include "ng_system.h"
#include "ng_mutex.h"
#include "ng_semphr.h"
#include "ng_snprintf.h"
#include "ng_list.h"

#define nitems(x) sizeof(x)/sizeof((x)[0])
#define _Q_INVALIDATE(x) (void)(x)

/*
 * Locks used to protect global variables in this file:
 *
 *  I  immutable after initialization
 *  T  timeout_mutex
 */
extern ng_mutex_s timeout_mutex;
#define mtx_enter(x) ngrtos_mutex_enter(x)
#define mtx_leave(x) ngrtos_mutex_leave(x)

/*
 * Timeouts are kept in a hierarchical timing wheel. The to_time is the value
 * of the global variable "ticks" when the timeout should be called. There are
 * four levels with WHEELSIZE buckets each.
 */
#define WHEELCOUNT      4
#define WHEELSIZE       256
#define WHEELSIZE_SHIFT 8
#define WHEELMASK       255
#define WHEELBITS       8
#define WHEELBITS_SHIFT 3
#define BUCKETS (WHEELCOUNT << WHEELSIZE_SHIFT)

static ng_list_s timeout_wheel[BUCKETS];  /* [T] Tick-based timeouts */
static ng_list_s timeout_new=             /* [T] New, unscheduled timeouts */
                  LIST_HEAD_INIT(timeout_new);             
static ng_list_s timeout_todo=            /* [T] Due or needs rescheduling */
                  LIST_HEAD_INIT(timeout_todo);             
extern ng_list_s timeout_proc;            /* [T] Due + needs process context */
extern ng_timer_stat_s tostat;            /* [T] statistics and totals */
extern ng_sem_s  timer_task_sem_usr;

#define MASKWHEEL(wheel, time) \
  (((time) >> ((wheel)<<WHEELBITS_SHIFT)) & WHEELMASK)

#define BUCKET(rel, abs)            \
    (timeout_wheel[              \
  ((rel) <= (1 << (2 << WHEELBITS_SHIFT)))          \
      ? ((rel) <= (1 << WHEELBITS))        \
    ? MASKWHEEL(0, (abs))          \
    : MASKWHEEL(1, (abs)) + WHEELSIZE      \
      : ((rel) <= (1 << (3<<WHEELBITS_SHIFT)))        \
    ? MASKWHEEL(2, (abs)) + (2<<WHEELSIZE_SHIFT)      \
    : MASKWHEEL(3, (abs)) + (3<<WHEELSIZE_SHIFT)])

#define MOVEBUCKET(wheel, time)            \
    list_splice_init( \
      &timeout_wheel[MASKWHEEL((wheel), (time)) + ((wheel)<<WHEELSIZE_SHIFT)], \
      &timeout_todo)

/*
 * Some of the "math" in here is a bit tricky.
 *
 * We have to beware of wrapping ints.
 * We use the fact that any element added to the queue must be added with a
 * positive time. That means that any element `to' on the queue cannot be
 * scheduled to timeout further in time than INT_MAX, but to->to_time can
 * be positive or negative so comparing it with anything is dangerous.
 * The only way we can use the to->to_time value in any predictable way
 * is when we calculate how far in the future `to' will timeout -
 * "to->to_time - ticks". The result will always be positive for future
 * timeouts and 0 or negative for due timeouts.
 */

ng_result_e
ngrtos_timer_subsystem_startup(void)
{
  for (int b = 0; b < nitems(timeout_wheel); b++)
    INIT_LIST_HEAD(&timeout_wheel[b]);
  return NG_result_ok;
}
void
ngrtos_timer_subsystem_destroy(void)
{
}

void
ngrtos_timer_set(ng_timer_s *to, ng_time_t ms, ng_timer_cb_f fn, 
  void *arg, int flags)
{
  ngRTOS_BIT_UNSET(flags, NG_TIMER_FLAG_From_sched);
  to->to_func  = fn;
  to->to_arg   = arg;
  to->to_flags = RTE_TIMER_STOP;
  to->interval = ng_ms2ticks(ms);
  to->expire   = 0;
  to->periodic = ngRTOS_BIT_TEST(flags, NG_TIMER_FLAG_Periodic);
  to->sched    = ngrtos_FALSE;
  to->refcnt   = 0;
  to->inque    = ngrtos_FALSE;
  __list_poison_entry(&to->to_list);
}

void
ngrtos_timer_add(ng_timer_s *timer, int msecs)
{
  int old_time;
  int to_ticks = ng_ms2ticks(msecs);
  
  NGRTOS_ASSERT(ngRTOS_BIT_TEST(timer->to_flags, RTE_TIMER_STOP));
  NGRTOS_ASSERT(to_ticks >= 0);

  mtx_enter(&timeout_mutex);

  /* Initialize the time here, it won't change. */
  old_time = timer->to_time;
  timer->to_time = to_ticks + ____ng_get_systicks();
  ngRTOS_BIT_UNSET(timer->to_flags, RTE_TIMER_RUNNING);

  /*
   * If this timeout already is scheduled and now is moved
   * earlier, reschedule it now. Otherwise leave it in place
   * and let it be rescheduled later.
   */
  if (timer->inque) 
  {
    if (timer->to_time - ____ng_get_systicks()
      < old_time - ____ng_get_systicks()) 
    {
      list_del(&timer->to_list);
      list_add_tail(&timer->to_list, &timeout_new);
    }
    tostat.tos_readded++;
  } 
  else 
  {
    timer->inque = ngrtos_TRUE;
    list_add_tail(&timer->to_list, &timeout_new);
    timer->refcnt++;
  }
  tostat.tos_added++;
  mtx_leave(&timeout_mutex);
}

void
ngrtos_timer_del(ng_timer_s *to)
{
  mtx_enter(&timeout_mutex);
  if (to->inque) 
  {
    list_del(&to->to_list);
    to->inque = ngrtos_FALSE;
    tostat.tos_cancelled++;
  }
  to->refcnt--;
  to->to_flags = RTE_TIMER_STOP;
  tostat.tos_deleted++;
  mtx_leave(&timeout_mutex);
}

/*
 * This is called from hardclock() on the primary CPU at the start of
 * every tick.
 */
ng_bool_t
timeout_hardclock_update(ngTickTypeT ticks)
{
  ng_bool_t need_softclock;
  
  NGRTOS_ASSERT(IS_IRQ());

  __ng_irq_priority_sanity_check();

  mtx_enter(&timeout_mutex);
  __DMB();

  /* maybe conflick with function  
         timeout_hardclock_update(),
         timeout_adjust_ticks(),
         oftclock_process_tick_timeout(),
         __timeout_softclock()
     which will operate timeout_wheel BUCKETS or timeout_todo list */
  __ng_sys_disable_irq_minpri();
  MOVEBUCKET(0, ticks);
  if (MASKWHEEL(0, ticks) == 0) {
    MOVEBUCKET(1, ticks);
    if (MASKWHEEL(1, ticks) == 0) {
      MOVEBUCKET(2, ticks);
      if (MASKWHEEL(2, ticks) == 0)
        MOVEBUCKET(3, ticks);
    }
  }
  need_softclock = !list_empty(&timeout_todo);
  __ng_sys_enable_irq_minpri();
  __DMB();
  mtx_leave(&timeout_mutex);

  return need_softclock;
}

static void
timeout_run(ng_timer_s *to)
{
  to->inque = ngrtos_FALSE;
  ngRTOS_BIT_SET(to->to_flags, RTE_TIMER_RUNNING);
  to->to_func(to, to->to_arg);
  ngRTOS_BIT_UNSET(to->to_flags, RTE_TIMER_RUNNING);
  to->refcnt--;
}

static void
softclock_process_tick_timeout(ng_timer_s *to, 
  ng_bool_t is_new)
{
  ngTickTypeT ticks = ____ng_get_systicks();
  int delta = ngrtos_difftime(to->to_time, ticks);

  if (delta > 0) 
  {
    ngIrqTypeT b;
    tostat.tos_scheduled++;
    if (!is_new)
      tostat.tos_rescheduled++;

    /* maybe conflick with function  
         timeout_hardclock_update(),
         timeout_adjust_ticks(),
         oftclock_process_tick_timeout(),
         __timeout_softclock()
       which will operate timeout_wheel BUCKETS or timeout_todo list */
    b = __ng_sys_disable_irq();
    list_add_tail(&to->to_list, &BUCKET(delta, to->to_time));
    to->inque = ngrtos_TRUE;
    __ng_sys_enable_irq(b);
    
    return;
  }
  if (!is_new && delta < 0)
  { 
    to->inque = ngrtos_TRUE;
    list_add_tail(&to->to_list, &timeout_proc);
    tostat.tos_late++;
    return;
  }

  timeout_run(to);
  tostat.tos_run_softclock++;
}

/*
 * Timeouts are processed here instead of timeout_hardclock_update()
 * to avoid doing any more work at IPL_CLOCK than absolutely necessary.
 * Down here at IPL_SOFTCLOCK other interrupts can be serviced promptly
 * so the system remains responsive even if there is a surge of timeouts.
 */
static void
__timeout_softclock(void *arg)
{
  ng_timer_s *first_new, *to;
  ngIrqTypeT b;

  first_new = NULL;
  mtx_enter(&timeout_mutex);
  if (!list_empty(&timeout_new))
  { 
    first_new = 
      list_first_entry(&timeout_new,ng_timer_s,to_list); 
  }

  /* maybe conflick with function 
         timeout_hardclock_update(),
         timeout_adjust_ticks(),
         oftclock_process_tick_timeout(),
         __timeout_softclock()
     which will operate timeout_wheel BUCKETS or timeout_todo list */
  b = __ng_sys_disable_irq();
  __DMB();
  list_splice_init(&timeout_new, &timeout_todo);
  while (!list_empty(&timeout_todo)) 
  {
    to = list_first_entry(&timeout_todo,ng_timer_s,to_list); 
    list_del(&to->to_list);
    to->inque = ngrtos_FALSE;
    __ng_sys_enable_irq(b);
    
    softclock_process_tick_timeout(to, to == first_new);
    b = __ng_sys_disable_irq();
  }
  tostat.tos_softclocks++;
  __ng_sys_enable_irq(b);
  __DMB();
  mtx_leave(&timeout_mutex);

  return;
}

void __ng_task_timer(void *arg)
{
  ng_timer_s *to;
  ng_list_s *head = &timeout_proc;

  for (;;) 
  {
    ngrtos_sem_wait(&timer_task_sem_usr, NG_SOFT_TIMEOUT_WAIT);

    __timeout_softclock(arg);
    
    mtx_enter(&timeout_mutex);

    while (!list_empty(head))
    {
      to = list_first_entry(head,ng_timer_s,to_list);
      list_del(&to->to_list);
      NGRTOS_ASSERT(to->to_flags == RTE_TIMER_RUNNING);
      to->inque = ngrtos_FALSE;

      timeout_run(to);

      if (to->periodic)
      {
        mtx_leave(&timeout_mutex);
        ngrtos_timer_add(to, to->interval);
        mtx_enter(&timeout_mutex);
      }
      else
      {
        to->to_flags = RTE_TIMER_STOP;
      }
      tostat.tos_run_thread++;
    }
    tostat.tos_thread_wakeups++;

    mtx_leave(&timeout_mutex);
  }
}

void
ngrtos_timer_adjust_ticks(int adj)
{
  ng_timer_s *to;
  int new_ticks, b;

  /* adjusting the monotonic clock backwards would be a Bad Thing */
  if (adj <= 0)
    return;

  mtx_enter(&timeout_mutex);

  new_ticks = ____ng_get_systicks() + adj;
  for (b = 0; b < BUCKETS; b++) 
  {
    ngIrqTypeT x;
    ng_list_s *head = &timeout_wheel[b];

    /* maybe conflick with function 
           timeout_hardclock_update(),
           timeout_adjust_ticks(),
           oftclock_process_tick_timeout(),
           __timeout_softclock()
       which will operate timeout_wheel BUCKETS or timeout_todo list */
    x = __ng_sys_disable_irq();
    while (!list_empty(head))
    {
      to = list_first_entry(head,ng_timer_s,to_list);
      /* when moving a timeout forward need to reinsert it */
      if (ngrtos_difftime(to->to_time, ____ng_get_systicks()) < adj)
        to->to_time = new_ticks;
      list_del(&to->to_list);
      list_add_tail(&to->to_list, &timeout_todo);
    }
    __ng_sys_enable_irq(x);
  }
  ____ng_set_systicks(new_ticks);
  mtx_leave(&timeout_mutex);
}

void
db_show_timeout(ng_timer_s *to, ng_list_s *bucket)
{
  ng_list_s *wheel;
  char *where;
  int width = sizeof(long) * 2;

  if (bucket == &timeout_new)
    where = "new";
  else if (bucket == &timeout_todo)
    where = "softint";
  else if (bucket == &timeout_proc)
    where = "thread";
  else 
  {
    char buf[8];
    wheel = timeout_wheel;
    ng_snprintf(buf, sizeof(buf), "%3ld/%1ld",
        (bucket - wheel) % WHEELSIZE,
        (bucket - wheel) / WHEELSIZE);
    where = buf;
  }
  NGRTOS_PRINT("%20d  %8s  %7s  0x%0*lx\n",
      to->to_time - ____ng_get_systicks(), 
      "ticks", where, width, 
      (u_long)to->to_arg);
  
  NGRTOS_UNUSED(where);
  NGRTOS_UNUSED(width);
}

void
db_show_callout_bucket(ng_list_s *bucket)
{
  ng_timer_s *p;

  list_for_each_entry(p, ng_timer_s, bucket, to_list)
  { 
    db_show_timeout(p, bucket);
  }
}

void
db_show_callout(void)
{
  int width = sizeof(long) * 2 + 2;
  int b;

  NGRTOS_PRINT("%20s  %8s\n", "lastscan", "clock");
  NGRTOS_PRINT("%20d  %8s\n", ____ng_get_systicks(), "ticks");
  NGRTOS_PRINT("\n");  
  NGRTOS_PRINT("%20s  %8s  %7s  %*s  %s\n",
      "remaining", "clock", "wheel", width, "arg", "func");
  db_show_callout_bucket(&timeout_new);
  db_show_callout_bucket(&timeout_todo);
  db_show_callout_bucket(&timeout_proc);
  for (b = 0; b < nitems(timeout_wheel); b++)
    db_show_callout_bucket(&timeout_wheel[b]);
  NGRTOS_UNUSED(width);
}

#endif
