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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ngRTOS_TASK_H__
#define __ngRTOS_TASK_H__

#ifdef __cplusplus
 extern "C" {
#endif

#include "ng_defs.h"
#include "ng_list.h"
#include "ng_tq.h"
#include "ng_utils.h"

struct ng_task_init{
  const char    *name;
  ng_task_func_f func;
  ng_task_func_f finish;
  void          *arg;
  uint8_t       *_sp;
  uint32_t       sp_size:22;
  uint32_t       priority:4;
  uint32_t       ctrl:6;
};

typedef struct ng_task_init ng_task_init_s; 

/**
 * Defines the priority used by the idle task.  This must not be modified.
 *
 * \ingroup TaskUtils
 */
#define tskIDLE_PRIORITY    ((ng_BASE_TYPE_T)0x0U)
#define tskCONF_PRIORITY    ((ng_BASE_TYPE_T)0x0U)

#define NGRTOS_TCTRL_COROUTINE 0x01

ng_result_e ng_scheduler_start(void);
void ngrtos_increase_ticks(void);
ng_result_e ngrtos_scheder_init(void);
void ngrtos_scheder_deinit(void);
ng_handle_t ngrtos_task_create(ng_task_init_s *_init);

ng_handle_t __ngrtos_get_curh(void);
void ngrtos_task_resume(ng_handle_t _t);
void ngrtos_task_delete(ng_handle_t h);
void ngrtos_suspend(void);
#if configEnable_COROUTINE  
void __ngrtos_switch_context_for_coroutine(void);
#endif

void ngrtos_switch_context(void);
void *__ngrtos_get_task_arg(void);
void ng_os_delay(ng_time_t ms);

/* Converts a time in milliseconds to a time in ticks.  This macro can be
 * overridden by a macro of the same name defined in FreeRTOSConfig.h in case the
 * definition here is not suitable for your application. */
__STATIC_FORCEINLINE ngTickTypeT ng_ms2ticks(ng_time_t ms)
{
#if ngRTOS_TICK_RATE_HZ == 1000UL
  return (ngTickTypeT)(ms);
#else
  return ((ngTickTypeT)(((ngTickTypeT)(ms)*(ngTickTypeT)ngRTOS_TICK_RATE_HZ)
                            / (ngTickTypeT)1000U));
#endif
}
__STATIC_FORCEINLINE ng_time_t ng_ticks2ms(ngTickTypeT ticks)
{
#if ngRTOS_TICK_RATE_HZ == 1000UL
  return (ng_time_t)(ticks);
#else
  return ((ngTickTypeT)(ticks))*((ngTickTypeT)1000U/
      (ngTickTypeT)((ngTickTypeT)ngRTOS_TICK_RATE_HZ));
#endif
}
__STATIC_FORCEINLINE ng_time_t ng_ticks2ns(ngTickTypeT ticks)
{
#if ngRTOS_TICK_RATE_HZ == 1000UL
  return (ng_time_t)(ticks*1000);
#else
  return ((ngTickTypeT)(ticks))*((ngTickTypeT)1000000U/
      (ngTickTypeT)((ngTickTypeT)ngRTOS_TICK_RATE_HZ));
#endif
}

#define NG_TIMER_FLAG_From_sched  0x01
#define NG_TIMER_FLAG_Periodic    0x02
typedef struct ng_timer ng_timer_s;

/**
 * Callback function type for timer expiry.
 */
typedef void (*ng_timer_cb_f)(ng_timer_s *, void *);

#define RTE_TIMER_STOP     0 /**< State: timer is stopped. */
#define RTE_TIMER_PENDING  1 /**< State: timer is scheduled. */
#define RTE_TIMER_RUNNING  2 /**< State: timer function is running. */
#define RTE_TIMER_CONFIG   3 /**< State: timer is being configured. */

#define RTE_TIMER_NO_OWNER -2 /**< Timer has no owner. */

struct ng_timer_stat {
  u_long tos_added;		/* timeout_add*(9) calls */
  u_long tos_cancelled;		/* dequeued during timeout_del*(9) */
  u_long tos_deleted;		/* timeout_del*(9) calls */
  u_long tos_late;		/* run after deadline */
  u_long tos_pending;		/* number currently ONQUEUE */
  u_long tos_readded;		/* timeout_add*(9) + already ONQUEUE */
  u_long tos_rescheduled;	/* bucketed + already SCHEDULED */
  u_long tos_run_softclock;	/* run from softclock() */
  u_long tos_run_thread;	/* run from softclock_thread() */
  u_long tos_scheduled;		/* bucketed during softclock() */
  u_long tos_softclocks;	/* softclock() calls */
  u_long tos_thread_wakeups;	/* wakeups in softclock_thread() */
};
typedef struct ng_timer_stat ng_timer_stat_s;

#if configEnable_TIMER_SKIP
/**
 * Timer status: A union of the state (stopped, pending, running,
 * config) and an owner (the id of the lcore that owns the timer).
 */
union rte_timer_status {
  RTE_STD_C11
  struct {
    uint16_t state;  /**< Stop, pending, running, config. */
    int16_t owner;   /**< The lcore that owns the timer. */
  };
  uint32_t u32;            /**< To atomic-set status + owner. */
};
typedef union rte_timer_status rte_timer_status_u;
#endif

/**
 * A structure describing a timer in RTE.
 */
struct ng_timer
{
  ngTickTypeT expire;     /**< Time when timer to expire, in ticks. */
  ngTickTypeT interval;   /**< Interval when timer expire, in ticks. */
  ng_timer_cb_f f;        /**< Callback function. */
  void *arg;              /**< Argument to callback function. */
  ngAtomTypeT refcnt;
  ng_list_s link;
  uint32_t status:3;      /**< Status of timer. */
  uint32_t periodic:1;    /**< Period of timer (0 if not periodic). */
  uint32_t inque:1;       
  uint32_t sched:1;       
#if configEnable_TIMER_SKIP
  uint32_t dataid:8;       
  uint32_t lcoreid:8;       
  volatile rte_timer_status_u _status; /**< Status of timer. */
  ng_timer_s *sl_next[MAX_SKIPLIST_DEPTH];
#endif
#define to_list  link   /* timeout queue, don't move */
#define to_func  f      /* function to call */
#define to_arg   arg    /* function argument */
#define to_time  expire /* ticks on event */
#define to_flags status /* status of timer */
};

#define NG_SOFT_TIMEOUT_WAIT 20

void ngrtos_timer_set(ng_timer_s *timer, 
  ng_time_t ms, ng_timer_cb_f cb, void *arg, int flag);
void ngrtos_timer_reset(ng_timer_s *timer, 
  ng_time_t ms);
void ngrtos_timer_del(ng_timer_s *timer);
void ngrtos_timer_add(ng_timer_s *timer, ng_time_t ms);
ng_result_e ngrtos_timer_subsystem_startup(void);
void ngrtos_timer_subsystem_destroy(void);
void ngrtos_timer_adjust_ticks(int adj);
void db_show_callout(void);

void __ng_task_timer(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* __ngRTOS_TASK_H__ */
