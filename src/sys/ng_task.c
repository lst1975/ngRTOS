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
 *                              https://github.com/lst1975/ngRTOS
 **************************************************************************************
 */
#include "ng_arch.h"
#include "ng_defs.h"
#include "ng_port.h"
#include "ng_list.h"
#include "ng_limits.h"
#include "ng_prio.h"
#include "ng_asm.h"
#include "ng_align.h"
#include "ng_task.h"
#include "ng_system.h"
#include "ng_rtos.h"
#include "mem/ng_mem.h"
#include "ng_tq.h"
#include "ng_semphr.h"
#include "ng_mutex.h"
#include "ng_atomic.h"

#define TH_STATE_READY    0
#define TH_STATE_PENDING  1
#define TH_STATE_DEAD     2
#define TH_STATE_MAX      3

#define TH_FLAG_RUNNING   0x01
#define TH_FLAG_MUTEX     0x02
#define TH_FLAG_SEMPHR    0x04
#define TH_FLAG_CRITICAL  0x08
#define TH_FLAG_SPALLOC   0x10
#define TH_FLAG_THALLOC   0x20
#define TH_FLAG_WAIT      0x40

#define SP_INIT_BYTE 0xC8U

typedef struct ng_context ng_context_s;
typedef struct ng_sched    ng_sched_s;
typedef struct ng_task_tcb ng_task_s;

struct ng_sched {
  ng_priority_list_t ready;
  ng_list_s   pending;
  ng_list_s   dead;
  ngAtomTypeT count;
};

#if configENABLE_TASK_STATS
struct ng_task_stat {
  ngrtos_couter_t ncalls;
  ngrtos_couter_t npends;
  ngrtos_couter_t nwaits;
  ngrtos_couter_t ngiveups;
};
typedef struct ng_task_stat ng_task_stat_s;
#endif

struct ng_context{
  uint8_t   *_sp_top; /* MUST BE the first member */
  ng_list_s  _state_entry;
  uint8_t   *_sp_ptr;  
  uint32_t   _flag:8;
  uint32_t   _state:2;
  uint32_t   _priority:6;
  uint32_t   _switched:1;
  uint32_t   _sp_size:15;
  void      *_arg;
  ng_tq_s   *_tq;
#if configENABLE_TASK_STATS
  ng_task_stat_s  _stat;
#endif
  ng_list_s  _tq_entry;
};

struct ng_task_tcb {
  ng_context_s     t_ctx;     /* MUST BE the first member */
  const char      *t_name;
  ng_task_func_f   t_finish;
  ngTickTypeT      t_runtime;
  ng_tfunc_s      *t_tfunc;
  ngAtomTypeT      t_refcnt;
  ng_timer_s       t_timer;
#if defined(__configUsePrintf)
  void            *t_stdch;
#endif
};
#define t_sp_top      t_ctx._sp_top  
#define t_sp_ptr      t_ctx._sp_ptr 
#define t_sp_size     t_ctx._sp_size
#define t_state_entry t_ctx._state_entry
#define t_flag        t_ctx._flag
#define t_state       t_ctx._state
#define t_priority    t_ctx._priority
#define t_arg         t_ctx._arg
#define t_stat        t_ctx._stat
#define t_tq          t_ctx._tq
#define t_switched    t_ctx._switched
#define t_tq_entry    t_ctx._tq_entry

#if !configENABLE_SMALL_KERNEL
static ng_sched_s ng_tasks = {
    .ready    = NG_PRIORITY_LIST_DECL(ng_tasks.ready),
    .dead     = LIST_HEAD_INIT(ng_tasks.dead),
    .pending  = LIST_HEAD_INIT(ng_tasks.pending),
    .count    = 0,
  };
#define __NG_TASKS() (&ng_tasks)
#else
  static ng_sched_s *ng_tasks = NULL;
#define __NG_TASKS() ng_tasks
#endif

static ng_handle_t ng_idle_task    = NULL;
static ng_handle_t ng_timer_task   = NULL;
static ng_sem_s    timer_task_sem_sys;
       ng_sem_s    timer_task_sem_usr;
static ng_string_s timer_task_name = __DFS(ngRTOS_TASK_NAME_TIMER);
volatile ng_task_s *__ng_cur_t     = NULL;
static ngAtomTypeT ng_sched_inited = 0;
ngAtomTypeT ng_scheduler_stop      = ngrtos_FALSE;
ng_timer_stat_s tostat;

#define H2TCB(h) ((ng_task_s*)(h))

static ng_list_s timer_list = LIST_HEAD_INIT(timer_list);
/* [T] Due + needs process context */
ng_list_s timeout_proc = LIST_HEAD_INIT(timeout_proc);
ng_mutex_s timeout_mutex;

static void __ngrtos_wakeup_timer_add(ng_timer_s *timer);

__WEAK void __ng_task_timer(void *arg)
{
  while (ngrtos_TRUE)
  {
    ngIrqTypeT b;
    ng_timer_s *t = NULL;

    ngrtos_sem_wait(&timer_task_sem_sys, 100);

    ngrtos_mutex_enter(&timeout_mutex);
    __DMB();
    b = __ng_sys_disable_irq();
    while (!list_empty(&timeout_proc))
    {
      t = list_first_entry(&timeout_proc,ng_timer_s,link);
      list_del(&t->link);
      t->inque = ngrtos_FALSE;
      NGRTOS_ASSERT(t->status == RTE_TIMER_RUNNING);
      __ng_sys_enable_irq(b);
      __DMB();

      t->f(t, t->arg);

      b = __ng_sys_disable_irq();
      t->refcnt--;
      if (t->periodic)
      {
        __ngrtos_wakeup_timer_add(t);
      }
      else
      {
        t->status = RTE_TIMER_STOP;
      }
    }
    __ng_sys_enable_irq(b);
    __DMB();
    ngrtos_mutex_leave(&timeout_mutex);
  }
}

__WEAK ng_result_e ngrtos_timer_subsystem_startup(void)
{
  return NG_result_ok;
}

__WEAK void ngrtos_timer_subsystem_destroy(void)
{
}

static ng_result_e 
ngrtos_system_timer_init(void)
{
  ng_result_e r;
  ng_task_init_s _init = {
      .name      = ngRTOS_TASK_NAME_TIMER,
      .func      = __ng_task_timer, 
      .finish    = NULL,
      .arg       = NULL,
      ._sp       = NULL,
      .sp_size   = ngRTOS_MINIMAL_STACK_SIZE<<3,
      .priority  = ngRTOS_PRIO_LOWEST,
      .ctrl      = 0
    };

  r = ngrtos_timer_subsystem_startup();
  if (r != NG_result_ok)
  {
    NGRTOS_ERROR("Failed to init timer subsystem.\n");
    goto err0;
  }

  ngrtos_mutex_init(&timeout_mutex, &timer_task_name);
  
  NGRTOS_EVENT("Starting to init timer scheduler.\n");
  r = ngrtos_sem_init(&timer_task_sem_sys, &timer_task_name);
  if (r != NG_result_ok)
  {
    NGRTOS_ERROR("Failed to init timer system timer task semaphr.\n");
    goto err1;
  }
  r = ngrtos_sem_init(&timer_task_sem_usr, &timer_task_name);
  if (r != NG_result_ok)
  {
    NGRTOS_ERROR("Failed to init timer user timer task semaphr.\n");
    goto err2;
  }
  ng_timer_task = ngrtos_task_create(&_init);
  if (ng_timer_task == NULL)
  {
    NGRTOS_ERROR("Failed to init system %s task.\n", _init.name);
    r = NG_result_failed;
    goto err3;
  }
  NGRTOS_EVENT("Init system %s task: OK.\n", _init.name);
  NGRTOS_EVENT("Init timer scheduler: OK.\n");
  return NG_result_ok;
  
err3:
  ngrtos_sem_destroy(&timer_task_sem_usr);
err2:
  ngrtos_sem_destroy(&timer_task_sem_sys);
err1:
  ngrtos_timer_subsystem_destroy();
err0:
  NGRTOS_EVENT("Init timer scheduler: Failed.\n");
  return r;
}

static void 
ngrtos_wakeup_timer_reset(ng_timer_s *timer, 
  ng_time_t ms)
{
  timer->interval = ng_ms2ticks(ms);
  timer->status   = RTE_TIMER_STOP;
}

static void 
ngrtos_wakeup_timer_set(ng_timer_s *timer, 
  ng_time_t ms, ng_timer_cb_f cb, void *arg, int flag)
{
  timer->interval = ng_ms2ticks(ms);
  timer->expire   = 0;
  timer->f        = cb;
  timer->arg      = arg;
  timer->status   = RTE_TIMER_STOP;
  timer->periodic = ngRTOS_BIT_TEST(flag, NG_TIMER_FLAG_Periodic);
  timer->sched    = ngRTOS_BIT_TEST(flag, NG_TIMER_FLAG_From_sched);
  timer->refcnt   = 0;
  timer->inque    = ngrtos_FALSE;
  __list_poison_entry(&timer->link);
}

static void 
__ngrtos_wakeup_timer_add(ng_timer_s *timer)
{
  timer->expire = ____ng_get_systicks() + ng_ms2ticks(timer->interval);
  timer->status = RTE_TIMER_PENDING;

#define __CMP(t,e,m) (ngrtos_difftime((t)->m, (e)->m) >= 0)
  list_cmp_add_before(timer,  
            ng_timer_s, 
            &timer_list, 
            link, 
            __CMP,
            expire);
#undef __CMP
  timer->refcnt++;
  timer->inque = ngrtos_TRUE;
}

static void 
__ngrtos_wakeup_timer_del(ng_timer_s *timer)
{
  timer->status = RTE_TIMER_STOP;
  if (timer->inque)  
  {
    list_del(&timer->link);
    timer->inque = ngrtos_FALSE;
    timer->refcnt--;
  }
}

void 
ngrtos_timer_reset(ng_timer_s *timer, 
  ng_time_t ms)
{
  ngrtos_mutex_enter(&timeout_mutex);
  ngrtos_wakeup_timer_reset(timer, ms);
  ngrtos_mutex_leave(&timeout_mutex);
}
__WEAK void 
ngrtos_timer_set(ng_timer_s *timer, ng_time_t ms, 
  ng_timer_cb_f cb, void *arg, int flag)
{
  ngrtos_mutex_enter(&timeout_mutex);
  ngRTOS_BIT_UNSET(flag,NG_TIMER_FLAG_From_sched);
  ngrtos_wakeup_timer_set(timer,ms,cb,arg,flag);
  ngrtos_mutex_leave(&timeout_mutex);
}
__WEAK void 
ngrtos_timer_add(ng_timer_s *timer, ng_time_t ms)
{
  ngIrqTypeT b;

  ngrtos_mutex_enter(&timeout_mutex);
  __DMB();
  b = __ng_sys_disable_irq();
  ngrtos_wakeup_timer_reset(timer, ms);
  __ngrtos_wakeup_timer_add(timer);
  __ng_sys_enable_irq(b);
  __DMB();
  ngrtos_mutex_leave(&timeout_mutex);
}
__WEAK void 
ngrtos_timer_del(ng_timer_s *timer)
{
  ngIrqTypeT b;
  ngrtos_mutex_enter(&timeout_mutex);
  __DMB();
  b = __ng_sys_disable_irq();
  __ngrtos_wakeup_timer_del(timer);
  __ng_sys_enable_irq(b);
  __DMB();
  ngrtos_mutex_leave(&timeout_mutex);
}

static ng_bool_t 
__ngrtos_task_resume_internal(ng_handle_t h);

ng_handle_t
__ngrtos_get_curh(void)
{
  return (ng_handle_t)__ng_cur_t;
}

void *
__ngrtos_get_task_arg(void)
{
  return (void *)__ng_cur_t->t_arg;
}

__STATIC_FORCEINLINE ng_task_s *
__ngrtos_get_curt(void)
{
  return (ng_task_s *)__ng_cur_t;
}

__STATIC_FORCEINLINE ng_bool_t
__ngrtos_is_idletask(ng_task_s *t)
{
  return t == NULL ? 
    __ngrtos_get_curt() == (ng_task_s *)ng_idle_task :
    t == (ng_task_s *)ng_idle_task;
}

__ng_STATIC_FORCEINLINE ng_bool_t
__ngrtos_is_curtask(volatile ng_task_s *t)
{
  return t == __ng_cur_t;
}

__STATIC_FORCEINLINE ng_bool_t
__ngrtos_task_isrunning(ng_task_s *t)
{
  return ngRTOS_BIT_TEST(t->t_flag, TH_FLAG_RUNNING);
}
__STATIC_FORCEINLINE ng_bool_t
__ngrtos_task_ispending(ng_task_s *t)
{
  return t->t_state == TH_STATE_PENDING;
}
ng_bool_t
__ngrtos_task_iswaiting(ng_task_s *t)
{
  return ngRTOS_BIT_TEST(t->t_flag, TH_FLAG_WAIT);
}
/*-----------------------------------------------------------*/

__ng_STATIC_FORCEINLINE void 
ngrtos_task_schedin(volatile ng_task_s *t)
{
  ngRTOS_BIT_SET(t->t_flag, TH_FLAG_RUNNING);
  t->t_runtime = ____ng_get_systicks();
#if configENABLE_TASK_STATS
  t->t_stat.ncalls++;
#endif
}

__ng_STATIC_FORCEINLINE void 
ngrtos_task_schedout(volatile ng_task_s *t)
{
  ngRTOS_BIT_UNSET(t->t_flag, TH_FLAG_RUNNING);
}

/**************************************************************/
static inline void
__ngrtos_task_enque(ng_task_s *t)
{
  ng_sched_s *sh = __NG_TASKS();
  if (t->t_state == TH_STATE_DEAD)
    list_add_tail(&t->t_state_entry, &sh->dead);
  else if (t->t_state == TH_STATE_PENDING)
    list_add_tail(&t->t_state_entry, &sh->pending);
  else
    ng_pri_enque(&sh->ready, &t->t_state_entry, t->t_priority);
}

static inline void
__ngrtos_task_deque(ng_task_s *t)
{
  list_del(&t->t_state_entry);
}

static inline void
__ngrtos_task_enque_ready(ng_task_s *t)
{
  ng_sched_s *sh = __NG_TASKS();
  ng_pri_enque(&sh->ready, &t->t_state_entry, t->t_priority);
}

static inline void
ngrtos_task_enque_ready(ng_task_s *t)
{
  ngIrqTypeT b = __ng_sys_disable_irq();
  NGRTOS_ASSERT(t->t_state == TH_STATE_READY);
  __ngrtos_task_enque_ready(t);
  __ng_sys_enable_irq(b);
}

static inline void
__ngrtos_task_deque_ready(ng_task_s *t)
{
  ng_sched_s *sh = __NG_TASKS();
  ng_pri_deque(&sh->ready, &t->t_state_entry, t->t_priority);
}

static inline ng_task_s *
__ngrtos_task_get_ready(void)
{
  ng_sched_s *sh = __NG_TASKS();
  ng_list_s *e = ng_pri_get(&sh->ready);
  ng_task_s *t = list_entry(e,ng_task_s,t_state_entry);
  return t;
}

static inline ng_task_s *
ngrtos_task_get_ready(void)
{
  ngIrqTypeT b = __ng_sys_disable_irq();
  ng_task_s *t = __ngrtos_task_get_ready();
  __ng_sys_enable_irq(b);
  return t;
}
/*************************************************************
  called by tick interrupt, lowest IRQ priority
**************************************************************/
static void 
ngrtos_timer_cb(ng_timer_s *timer, void *arg)
{
  ng_task_s	*t = (ng_task_s *)arg;
  __ngrtos_task_resume_internal(t);
}

/* Called by SYS_TICK interrupt handler, do some small
   works and teturn as soon as possible, complicated 
   or unpredictable time-consuming TIMER_CALLBACK works 
   were done by the timer task woke up. */
static void ngrtos_timer_check(void)
{
  ng_bool_t has_task_timer = ngrtos_FALSE;
  ngTickTypeT ticks = ____ng_get_systicks();
  ng_timer_s *t;

  ngrtos_mutex_enter(&timeout_mutex);
  __DMB();

  /* The SysTick runs at the lowest interrupt priority, 
   * so when this interrupt executes all interrupts must 
   * be unmasked.  There is therefore no need to save and 
   * then restore the interrupt mask value as its value 
   * is already known. */
  __ng_sys_disable_irq_minpri();
  while (!list_empty(&timer_list))
  {
    t = list_first_entry(&timer_list,ng_timer_s,link);
    if (ngrtos_difftime(t->expire, ticks) > 0)
      break;
    list_del(&t->link);
    t->inque = ngrtos_FALSE;
    NGRTOS_ASSERT(t->status == RTE_TIMER_PENDING)
    t->status = RTE_TIMER_RUNNING;
    __DMB();

    if (t->sched)
    {
      t->f(t, t->arg);

      if (t->periodic)
      {
        __ngrtos_wakeup_timer_add(t);
      }
      else
      {
        t->refcnt--;
        t->status = RTE_TIMER_STOP;
      }
    }
    else
    {
      list_add_tail(&t->link, &timeout_proc);
    }
  }

  has_task_timer = !list_empty(&timeout_proc);
  __ng_sys_enable_irq_minpri();

  __DMB();
  ngrtos_mutex_leave(&timeout_mutex);

  if (has_task_timer)
    ngrtos_sem_wake(&timer_task_sem_sys);
}

__WEAK void
ngrtos_timer_adjust_ticks(int adj)
{
  int new_ticks;
  ngIrqTypeT x;
  ng_timer_s *to;
  ng_list_s *head = &timer_list;

  /* adjusting the monotonic clock backwards would 
   * be a Bad Thing 
   */
  if (adj <= 0)
    return;

  ngrtos_mutex_enter(&timeout_mutex);

  new_ticks = ____ng_get_systicks() + adj;

  /* maybe conflick with function 
         timeout_hardclock_update(),
         timeout_adjust_ticks(),
         oftclock_process_tick_timeout(),
         __timeout_softclock()
     which will operate timeout_wheel BUCKETS or timeout_todo list */
  x = __ng_sys_disable_irq();
  list_for_each_entry(to,ng_timer_s,head,to_list)
  {
    /* when moving a timeout forward need to reinsert it */
    if (ngrtos_difftime(to->to_time, ____ng_get_systicks()) < adj)
      to->to_time = new_ticks;
  }
  __ng_sys_enable_irq(x);
  ____ng_set_systicks(new_ticks);
  ngrtos_mutex_leave(&timeout_mutex);
}

static void __ngrtos_switch_context(void)
{
  volatile ng_task_s *t = __ng_cur_t;
  
  t->t_switched = ngrtos_TRUE;
  __ngrtos_task_enque((ng_task_s *)t);
  ngrtos_task_schedout(t);
  
  /* Select a new task to run */
  __ng_cur_t = t = __ngrtos_task_get_ready();
  NGRTOS_ASSERT(t != NULL);
  ngrtos_task_schedin(t);
}

ng_handle_t
ngrtos_task_create(ng_task_init_s *_init)
{
  ng_task_s *t;
  uint8_t *sp;

  const char *name      = _init->name; 
  ng_task_func_f func   = _init->func;
  ng_task_func_f finish = _init->finish;
  void *arg             = _init->arg;
  uint8_t *_sp          = _init->_sp;
  size_t sp_size        = _init->sp_size;
  int16_t priority      = _init->priority;

  NGRTOS_ASSERT(priority < ngRTOS_PRIO_MAX);
  NGRTOS_ASSERT(sp_size > 0);

  sp_size = RTE_ALIGN(sp_size, RTE_CACHE_LINE_SIZE, size_t);
  t = (ng_task_s *)ng_malloc(sizeof(*t));
  if (t == NULL) 
  {
    NGRTOS_ERROR("Out of memory! Failed to malloc thread.\n");
    goto err0;
  }
  ngRTOS_BIT_SET(t->t_flag, TH_FLAG_THALLOC);
  
  if (_sp == NULL)
  {
    sp = (uint8_t *)ng_malloc(sp_size);
    if (sp == NULL) 
    {
      NGRTOS_ERROR("Out of memory! Failed to malloc stack, "
        "size:%d.\n", sp_size);
      goto err1;
    }
    ngRTOS_BIT_SET(t->t_flag, TH_FLAG_SPALLOC);
  }
  else
  {
    sp = _sp;
    ngRTOS_BIT_UNSET(t->t_flag, TH_FLAG_SPALLOC);
  }
  
#if !configENABLE_SPEED_FIRST
  ng_memset(sp, SP_INIT_BYTE, sp_size);
#endif

  t->t_sp_ptr   = sp;
  t->t_sp_size  = sp_size;
  t->t_sp_top   = __ng_stack_init(sp 
#if __ngRTOS_STACK_DECREASE  
      + sp_size, 
#endif
      func, arg);
  t->t_tq       = NULL;
  t->t_state    = TH_STATE_READY;
  t->t_flag     = TH_FLAG_RUNNING;
  t->t_runtime  = 0;
  t->t_arg      = arg;
  t->t_finish   = finish;
  t->t_refcnt   = 0;
  
#if configENABLE_TASK_STATS
  t->t_stat.ncalls   = 0;
  t->t_stat.npends   = 0;
  t->t_stat.nwaits   = 0;
  t->t_stat.ngiveups = 0;
#endif
  
#if defined(__configUsePrintf)
  t->t_stdch         = NULL;
#endif
  
  t->t_name          = name;
  t->t_priority      = priority;
  
  __list_poison_entry(&t->t_state_entry);
  __list_poison_entry(&t->t_tq_entry);
  
  ngrtos_wakeup_timer_set(&t->t_timer, 
                   NG_TIME_MAX, 
                   ngrtos_timer_cb, 
                   t, 
                   NG_TIMER_FLAG_From_sched);
  
  ngrtos_task_enque_ready(t);

  __NG_TASKS()->count++;

  return t;
  
err1:
  if (_sp != NULL)
    ng_free(sp);
err0:
  ng_free(t);
  
  NGRTOS_ERROR("Failed to create task %s.\n", name);
  return NULL;
}

__STATIC_FORCEINLINE void __task_free(ng_task_s *t)
{
  ngIrqTypeT b;

  b = __ng_sys_disable_irq();
  if (--t->t_refcnt)
  {
    __ng_sys_enable_irq(b);
    ng_free(t);
  }
  else
  {
    __ng_sys_enable_irq(b);
  }
}

/* release: put thread back on free list */
void 
ngrtos_task_destroy(ng_task_s *t)
{
  ngIrqTypeT b;

  if (t == NULL)
    return;
  
  NGRTOS_WARN("Possible channel leak in %s.\n", 
     t->t_name);
  NGRTOS_ASSERT(t->t_state == TH_STATE_DEAD);
  
  b = __ng_sys_disable_irq();
  NGRTOS_ASSERT(!__ngrtos_task_isrunning(t));
  NGRTOS_ASSERT(!__ngrtos_task_iswaiting(t));
  NGRTOS_ASSERT(t->t_tq == NULL);
  NGRTOS_ASSERT(t->t_timer.status == RTE_TIMER_STOP);
  __ngrtos_task_deque(t);
  __ng_sys_enable_irq(b);

  if (t->t_finish)
  {
    t->t_finish(t->t_arg);
  }
  if (ngRTOS_BIT_TEST(t->t_flag, TH_FLAG_SPALLOC))
    ng_free(t->t_sp_ptr);
  if (ngRTOS_BIT_TEST(t->t_flag, TH_FLAG_THALLOC))
    __task_free(t);

  __NG_TASKS()->count--;
}

static int
ngrtos_task_execute(ng_task_s *t)
{
  ng_sched_s *sh = __NG_TASKS();
  t->t_runtime = ____ng_get_systicks();

  /*
   * See if something tromped on the end of the stack.
   */
#if configENABLE_TASK_STATS
  t->t_stat.ncalls++;
#endif

  if (!__ngrtos_is_curtask(t))
  {
    t->t_state = TH_STATE_READY;
    __ngrtos_task_enque_ready(t);
  }

  return TH_PRI_CMP_GT(ng_pri_first(&sh->ready), 
             t->t_priority);
}

#if __NG_CTX_SWITCH_WAIT
static inline void 
____ngrtos_task_wait_switch(ng_task_s *t) 
{
  if (!IS_IRQ())
  {
    ng_bool_t switched;

    __DMB();
    __ISB();
    do 
    {
      ngIrqTypeT b;
      b   = __ng_sys_disable_irq();
      switched = t->t_switched;
      __ng_sys_enable_irq(b);
    } while (!switched);
  }
}
#else
#define ____ngrtos_task_wait_switch(t) (void)(0)
#endif

static inline void 
____ngrtos_task_start_switch(ng_task_s *t, 
  ngIrqTypeT b) 
{
  t->t_switched = ngrtos_FALSE;
  __ng_sys_enable_irq(b);
  __DMB();
  ngrtos_mutex_leave(&timeout_mutex);

  __DMB();
  __ng_sys_yield();
  ____ngrtos_task_wait_switch(t);
}

void ngrtos_task_delete(ng_handle_t h)
{
  ngIrqTypeT b;
  ng_task_s *t = H2TCB(h);

  NGRTOS_ASSERT(!__ngrtos_is_idletask(t));
  
  ngrtos_mutex_enter(&timeout_mutex);
  __DMB();
  b = __ng_sys_disable_irq();
  if (t->t_state == TH_STATE_DEAD)
  {
    __ng_sys_enable_irq(b);
    __DMB();
    ngrtos_mutex_leave(&timeout_mutex);
    return;
  }

  if (__ngrtos_is_curtask(t))
  {
    NGRTOS_ASSERT(__ngrtos_task_isrunning(t));
    NGRTOS_ASSERT(!__ngrtos_task_iswaiting(t));
    NGRTOS_ASSERT(t->t_state == TH_STATE_READY);
    ____ngrtos_task_start_switch(t, b);
  }
  else
  {
    if (t->t_tq != NULL)
    {
      ngrtos_tq_del(&t->t_tq_entry);
      t->t_tq = NULL;
    }
    
    NGRTOS_ASSERT(__ngrtos_task_ispending(t)
      || t->t_state == TH_STATE_READY);
    
    if (__ngrtos_task_iswaiting(t))
    {
      __ngrtos_wakeup_timer_del(&t->t_timer);
      ngRTOS_BIT_UNSET(t->t_flag, TH_FLAG_WAIT);
    }
    __ngrtos_task_deque(t);
    t->t_state = TH_STATE_DEAD;
    __ngrtos_task_enque(t);
    __ng_sys_enable_irq(b);
    __DMB();
    ngrtos_mutex_leave(&timeout_mutex);
  }
}

/*
 * process_resume
 *
 * The process is voluntarily relinquishing the processor.  
 */
static ng_bool_t
__ngrtos_task_resume_internal(ng_handle_t h)
{
  ng_task_s *t = H2TCB(h);

  NGRTOS_ASSERT(!__ngrtos_is_idletask(t));

  if (t->t_state == TH_STATE_READY
    || t->t_state == TH_STATE_DEAD)
  {
    NGRTOS_ASSERT(!__ngrtos_task_iswaiting(t));
    NGRTOS_ASSERT(!__ngrtos_task_ispending(t));
    NGRTOS_ASSERT(t->t_tq == NULL);
    NGRTOS_ASSERT(t->t_timer.status == RTE_TIMER_STOP);
    return ngrtos_FALSE;
  }
  
  NGRTOS_ASSERT(t->t_state == TH_STATE_PENDING);
  if (__ngrtos_task_iswaiting(t))
  {
    __ngrtos_wakeup_timer_del(&t->t_timer);
    ngRTOS_BIT_UNSET(t->t_flag, TH_FLAG_WAIT);
  }
  if (t->t_tq != NULL)
  {
    ngrtos_tq_del(&t->t_tq_entry);
    t->t_tq = NULL;
  }
  __ngrtos_task_deque(t);
  
  return ngrtos_task_execute(t);
}

void
ngrtos_task_resume(ng_handle_t h)
{
  ngIrqTypeT b;

  ngrtos_mutex_enter(&timeout_mutex);
  __DMB();
  b = __ng_sys_disable_irq();
  if (__ngrtos_task_resume_internal(h))
  {
    ____ngrtos_task_start_switch(H2TCB(h), b);
  }
  else
  {
    __ng_sys_enable_irq(b);
    __DMB();
    ngrtos_mutex_leave(&timeout_mutex);
  }
}

/*********************************************************
 * ngrtos_task_suspend
 * ngrtos_suspend
 * ng_os_delay
 *
 * The process is voluntarily giveup CPU.  
 **********************************************************/
void
ngrtos_task_suspend(ng_task_s *t)
{
  ngIrqTypeT b;

  b = __ng_sys_disable_irq();
  NGRTOS_ASSERT(!__ngrtos_is_idletask(t));
  if (t->t_state == TH_STATE_PENDING
    || t->t_state == TH_STATE_DEAD)
  {
    __ng_sys_enable_irq(b);
    return;
  }
  NGRTOS_ASSERT(t->t_state == TH_STATE_READY);
  
  if (__ngrtos_is_curtask(t))
  {
#if configENABLE_TASK_STATS
    t->t_stat.ngiveups++;
#endif
    NGRTOS_ASSERT(!__ngrtos_task_iswaiting(t));
    NGRTOS_ASSERT(!__ngrtos_task_ispending(t));
    NGRTOS_ASSERT(t->t_tq == NULL);
    NGRTOS_ASSERT(t->t_timer.status == RTE_TIMER_STOP);
    t->t_state = TH_STATE_PENDING;
    ____ngrtos_task_start_switch(t, b);
  }
  else
  {
#if configENABLE_TASK_STATS
    t->t_stat.npends++;
#endif
    __ngrtos_task_deque_ready(t);
    t->t_state = TH_STATE_PENDING;
    __ngrtos_task_enque(t);
    __ng_sys_enable_irq(b);
  }
}

void
ngrtos_suspend(void)
{
  ng_task_s *t;
  ngIrqTypeT b;

  b = __ng_sys_disable_irq();
  t = __ngrtos_get_curt();
  NGRTOS_ASSERT(t != NULL);
  NGRTOS_ASSERT(t->t_state == TH_STATE_READY);
  NGRTOS_ASSERT(__ngrtos_task_isrunning(t));
  NGRTOS_ASSERT(!__ngrtos_is_idletask(t));
  NGRTOS_ASSERT(t->t_timer.inque == ngrtos_FALSE);

#if configENABLE_TASK_STATS
  t->t_stat.ngiveups++;
#endif
  t->t_state = TH_STATE_PENDING;
  ____ngrtos_task_start_switch(t, b);
}

void ng_os_delay(ng_time_t ms)
{
  ngIrqTypeT b;
  ng_task_s *t;

  NGRTOS_ASSERT(!IS_IRQ());

  ngrtos_mutex_enter(&timeout_mutex);
  __DMB();
  b = __ng_sys_disable_irq();
  t = __ngrtos_get_curt();
  NGRTOS_ASSERT(t != NULL);
  NGRTOS_ASSERT(t->t_state == TH_STATE_READY);
  NGRTOS_ASSERT(__ngrtos_task_isrunning(t));
  NGRTOS_ASSERT(!__ngrtos_is_idletask(t));
  NGRTOS_ASSERT(t->t_timer.inque == ngrtos_FALSE);

#if configENABLE_TASK_STATS
  t->t_stat.nwaits++;
#endif
  __ngrtos_wakeup_timer_del(&t->t_timer);
  ngrtos_wakeup_timer_reset(&t->t_timer, ms);
  __ngrtos_wakeup_timer_add(&t->t_timer);
  t->t_state = TH_STATE_PENDING;
  ngRTOS_BIT_SET(t->t_flag, TH_FLAG_WAIT);

  ____ngrtos_task_start_switch(t, b);
}

__WEAK ng_bool_t
timeout_hardclock_update(ngTickTypeT ticks)
{
  NGRTOS_ASSERT(IS_IRQ());

  __ng_irq_priority_sanity_check();
  return ngrtos_FALSE;
}

__STATIC_FORCEINLINE void ng_ticks_process(void)
{
  ng_sched_s *sh = __NG_TASKS();
  ngAtomTypeT hasReadyTask = ngrtos_FALSE;

  NGRTOS_ASSERT(IS_IRQ());

  __ng_irq_priority_sanity_check();

  ngrtos_timer_check();

  if (timeout_hardclock_update(____ng_get_systicks()))
    ngrtos_sem_wake(&timer_task_sem_usr);
    
  /* The SysTick runs at the lowest interrupt priority, 
   * so when this interrupt executes all interrupts must 
   * be unmasked.  There is therefore no need to save and 
   * then restore the interrupt mask value as its value 
   * is already known. */
  __ng_sys_disable_irq_minpri();
  if (!ng_pri_empty(&sh->ready))
  {
    ngAtomTypeT pri;
    ng_task_s *t = __ngrtos_get_curt();
    pri = ng_pri_first(&sh->ready);
#if configENABLE_PREEMPTION && configENABLE_TIME_SLICING
    if (TH_PRI_CMP_GE(pri, t->t_priority))
      hasReadyTask = ngrtos_TRUE;
#elif configENABLE_PREEMPTION      
    if (TH_PRI_CMP_GT(pri, t->t_priority))
      hasReadyTask = ngrtos_TRUE;
#endif      
  }
  __ng_sys_enable_irq_minpri();
  __DMB();

  /* The tick hook gets called at regular intervals, even 
   * if the scheduler is locked. 
   */
#if configENABLE_TICK_HOOK
  ngSystemTickHook();
#endif

  /* A context switch is required.  Context switching is 
   * performed in the PendSV interrupt. Pend the PendSV 
   * interrupt. 
   */
  if (hasReadyTask)
    __ng_sys_yield();
}

static void ng_task_idle(void *arg)
{
  ng_sched_s *sh = __NG_TASKS();
  
  NGRTOS_ASSERT(__ngrtos_is_idletask(NULL));
  ng_sched_inited = ngrtos_TRUE;
  __DMB();
  
  while (ngrtos_TRUE)
  {
    ngIrqTypeT b;
    ng_task_s *t = NULL;

    b = __ng_sys_disable_irq();
    if (!list_empty(&sh->dead))
    {
      t = list_first_entry(&sh->dead,ng_task_s,
             t_state_entry);
      list_del(&t->t_state_entry);
      
      /* WAIT for the timer finished*/
      NGRTOS_ASSERT(t->t_timer.status == RTE_TIMER_STOP);
      NGRTOS_ASSERT(t->t_timer.inque == ngrtos_FALSE);
      if (t->t_timer.refcnt)
      {
        list_add_tail(&t->t_state_entry, &sh->dead);
        __ng_sys_enable_irq(b);
        continue;
      }
    }
    __ng_sys_enable_irq(b);

    ngrtos_task_destroy(t);

#if configENABLE_IDLE_HOOK
    ngSystemIdleHook();
#endif
  }
}

ng_result_e ngrtos_scheder_init(void)
{
  ng_sched_s *sh;

  NGRTOS_EVENT("Starting scheduler init.\n");
#if !configENABLE_SMALL_KERNEL
  sh = __NG_TASKS();
#else
  sh = (ng_sched_s *)ng_malloc(sizeof(*sh));
  if (sh == NULL)
  {
    NGRTOS_EVENT("Failed to init scheduler.\n");
    return NG_result_nmem;
  }
  INIT_LIST_HEAD(&sh->dead);
  INIT_LIST_HEAD(&sh->pending);
  sh->count = 0;
  ng_tasks = sh;
#endif
  ng_pri_init(&sh->ready);
  NGRTOS_EVENT("Scheduler init OK.\n");
  return NG_result_ok;
}

void ngrtos_scheder_deinit(void)
{
  NGRTOS_EVENT("Starting scheduler deinit.\n");
#if !configENABLE_SMALL_KERNEL
#else
  if (ng_tasks != NULL)
    ng_free(ng_tasks);
#endif
  NGRTOS_EVENT("Scheduler deinit OK.\n");
}

ng_result_e ng_scheduler_start(void)
{
  ng_result_e ret;

  NGRTOS_EVENT("Starting system scheduler.\n");

  ng_task_init_s _init = {
      .name      = ngRTOS_TASK_NAME_IDLE,
      .func      = ng_task_idle, 
      .finish    = NULL,
      .arg       = NULL,
      ._sp       = NULL,
      .sp_size   = ngRTOS_MINIMAL_STACK_SIZE<<3,
      .priority  = ngRTOS_PRIO_LOWEST,
      .ctrl      = 0
    };
  ng_idle_task = ngrtos_task_create(&_init);
  if (ng_idle_task == NULL)
  {
    NGRTOS_ERROR("Failed to init system %s task.\n", 
      _init.name);
    return NG_result_failed;
  }
  NGRTOS_EVENT("Init system %s task: OK.\n", _init.name);

  ret = ngrtos_system_timer_init();
  if (ret != NG_result_ok)
  {
    return ret;
  }

  if (__ng_scheduler_start() == NG_result_ok)
  {
    NGRTOS_ASSERT(__ngrtos_get_curt() == NULL);
    __ng_cur_t = ngrtos_task_get_ready();
    
    __DMB();
    __ISB();     

    NGRTOS_EVENT("Scheduler init OK.\n");

    /* Start the first task. */
    NGRTOS_EVENT("Starting the first task.\n");
    __ng_start_first_task();
    return NG_result_ok;
  }
  else
  {
    NGRTOS_ERROR("Failed to init system scheduler.\n");
    return NG_result_failed;
  }
}
/*-----------------------------------------------------------*/

void ng_task_scheduler_stop( void )
{
  ng_scheduler_stop = ngrtos_TRUE;
}
/*-----------------------------------------------------------*/

void ng_task_scheduler_resume( void )
{
  ng_scheduler_stop = ngrtos_FALSE;
}
/*-----------------------------------------------------------*/

/* Increment the RTOS tick. */
void ngrtos_increase_ticks(void)
{
  NGRTOS_ASSERT(IS_IRQ());

  if (ng_sched_inited)
  {
    __ng_irq_priority_sanity_check();
    ng_ticks_process();
  }
}
/*-----------------------------------------------------------*/
void ngrtos_switch_context(void)
{
  if (ng_scheduler_stop)
  {
    /* The scheduler is currently suspended - do not allow a 
     * context switch. 
     */
    return;
  }
  else
  {
    __ngrtos_switch_context();
  }
}

void ngrtos_tq_init(ng_tq_s *tq, const ng_string_s *name)
{
  NGRTOS_ASSERT(NULL != tq);
  tq->count        = 0;
  tq->flag         = 0;
  tq->wcnt         = 0;
  tq->name         = name;
  INIT_LIST_HEAD(&tq->waiters);
}

void ngrtos_tq_add(ng_tq_s *tq, ng_handle_t h)
{
  ng_list_s *l = &tq->waiters;
  ng_task_s *e = h == NULL ? __ngrtos_get_curt() : H2TCB(h);
  e->t_tq  = tq;
  tq->wcnt++;
  list_cmp_add_before(
    e, 
    ng_task_s, 
    l, 
    t_tq_entry, 
    __task_pri_is_TgtE,
    t_priority);
}

ng_handle_t ngrtos_tq_get(ng_tq_s *tq)
{
  ng_task_s *t;
  ng_list_s *l = &tq->waiters;

  if (list_empty(l))
    return NULL;
  t = list_first_entry(l,ng_task_s,t_tq_entry);
  list_del(&t->t_state_entry);
  tq->wcnt--;
  t->t_tq  = NULL;
  t->t_refcnt++;
  return t;
}

void ngrtos_tq_del(ng_list_s *l)
{
  list_del(l);
}
