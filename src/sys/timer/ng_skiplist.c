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

/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2014 Intel Corporation
 */
#include "ng_config.h"

#if configEnable_TIMER_SKIP
#include "ng_defs.h"
#include "ng_utils.h"
#include "ng_task.h"
#include "ng_mcopy.h"
#include "ng_ring.h"
#include "ng_limits.h"
#include "ng_system.h"
#include "ng_rand.h"
#include "ng_mutex.h"
#include "ng_semphr.h"
#include "mem/ng_mem.h"

#define default_data_id 0
#define RTE_MAX_DATA_ELS 1
#define __ATOMIC_RELEASE 0
#define rte_get_timer_cycles() ____ng_get_systicks()
#define LCORE_ID_ANY (-1)
#define RTE_LIBRTE_TIMER_DEBUG 1
#define RTE_MAX_LCORE 1
#define rte_lcore_id() 0
#define rte_get_next_lcore(a,b,c) RTE_MAX_LCORE

#define __atomic_load_n(ptr, t) __LDREXW(ptr)
#define __atomic_load_n16(ptr, t) __LDREXH(ptr)
#define __atomic_store_n(ptr, v, t) __STREXW((v), (ptr));
#define __atomic_compare_exchange_n(ptr, _old, _new, n, t1, t2) \
  __arch_atomic32_cmpxchg_ptr(ptr, _old, _new)

#define rte_spinlock_unlock(x) __ng_sys_enable_irq(*(x))
#define rte_spinlock_lock(x) (*(x)) = __ng_sys_disable_irq()
#define rte_spinlock_init(x) (void)(x)

typedef ngIrqTypeT rte_spinlock_t;

/**
 * Timer type: Periodic or single (one-shot).
 */
enum rte_timer_type {
  SINGLE = (!NG_TIMER_FLAG_Periodic),
  PERIODICAL = NG_TIMER_FLAG_Periodic
};
typedef enum rte_timer_type rte_timer_type_e;

#ifdef RTE_LIBRTE_TIMER_DEBUG
/**
 * A structure that stores the timer statistics (per-lcore).
 */
struct ng_timer_debug_stats {
  u_llong reset;   /**< Number of success calls to rte_timer_reset(). */
  u_llong stop;    /**< Number of success calls to rte_timer_stop(). */
  u_llong manage;  /**< Number of calls to rte_timer_manage(). */
  u_llong pending; /**< Number of pending/running timers. */
};
typedef struct ng_timer_debug_stats ng_timer_debug_stats_s;
#endif

/**
 * Callback function type for timer expiry.
 */
typedef ng_timer_cb_f rte_timer_cb_t;

#define MAX_SKIPLIST_DEPTH 10

#ifdef __cplusplus
/**
 * A C++ static initializer for a timer structure.
 */
#define RTE_TIMER_INITIALIZER {             \
  0,                                      \
  {NULL},                                 \
  {{RTE_TIMER_STOP, RTE_TIMER_NO_OWNER}}, \
  0,                                      \
  NULL,                                   \
  NULL,                                   \
  }
#else
/**
 * A static initializer for a timer structure.
 */
#define RTE_TIMER_INITIALIZER {                      \
    ._status = {{                         \
      .state = RTE_TIMER_STOP,     \
      .owner = RTE_TIMER_NO_OWNER, \
    }},                                  \
  }
#endif

/**
 * Allocate a timer data instance in shared memory to track a set of pending
 * timer lists.
 *
 * @param id_ptr
 *   Pointer to variable into which to write the identifier of the allocated
 *   timer data instance.
 *
 * @return
 *   - 0: Success
 *   - -ENOSPC: maximum number of timer data instances already allocated
 */
ng_result_e rte_timer_data_alloc(uint32_t *id_ptr);

/**
 * Deallocate a timer data instance.
 *
 * @param id
 *   Identifier of the timer data instance to deallocate.
 *
 * @return
 *   - 0: Success
 *   - -EINVAL: invalid timer data instance identifier
 */
ng_result_e rte_timer_data_dealloc(uint32_t id);

/**
 * Initialize the timer library.
 *
 * Initializes internal variables (list, locks and so on) for the RTE
 * timer library.
 *
 * @note
 *   This function must be called in every process before using the library.
 *
 * @return
 *   - 0: Success
 *   - -ENOMEM: Unable to allocate memory needed to initialize timer
 *      subsystem
 *   - -EALREADY: timer subsystem was already initialized. Not an error.
 */
ng_result_e rte_timer_subsystem_init(void);

/**
 * Free timer subsystem resources.
 */
void rte_timer_subsystem_finalize(void);

/**
 * Initialize a timer handle.
 *
 * The rte_timer_init() function initializes the timer handle *tim*
 * for use. No operations can be performed on a timer before it is
 * initialized.
 *
 * @param tim
 *   The timer to initialize.
 */
void rte_timer_init(ng_timer_s *tim);

/**
 * Reset and start the timer associated with the timer handle.
 *
 * The rte_timer_reset() function resets and starts the timer
 * associated with the timer handle *tim*. When the timer expires after
 * *ticks* HPET cycles, the function specified by *fct* will be called
 * with the argument *arg* on core *tim_lcore*.
 *
 * If the timer associated with the timer handle is already running
 * (in the RUNNING state), the function will fail. The user has to check
 * the return value of the function to see if there is a chance that the
 * timer is in the RUNNING state.
 *
 * If the timer is being configured on another core (the CONFIG state),
 * it will also fail.
 *
 * If the timer is pending or stopped, it will be rescheduled with the
 * new parameters.
 *
 * @param tim
 *   The timer handle.
 * @param ticks
 *   The number of cycles (see rte_get_hpet_hz()) before the callback
 *   function is called.
 * @param type
 *   The type can be either:
 *   - PERIODICAL: The timer is automatically reloaded after execution
 *     (returns to the PENDING state)
 *   - SINGLE: The timer is one-shot, that is, the timer goes to a
 *     STOPPED state after execution.
 * @param tim_lcore
 *   The ID of the lcore where the timer callback function has to be
 *   executed. If tim_lcore is LCORE_ID_ANY, the timer library will
 *   launch it on a different core for each call (round-robin).
 * @param fct
 *   The callback function of the timer.
 * @param arg
 *   The user argument of the callback function.
 * @return
 *   - 0: Success; the timer is scheduled.
 *   - (-1): Timer is in the RUNNING or CONFIG state.
 */
ng_result_e
rte_timer_reset(ng_timer_s *tim, ngTickTypeT ticks,
        rte_timer_type_e type, unsigned tim_lcore,
        rte_timer_cb_t fct, void *arg);

/**
 * Loop until rte_timer_reset() succeeds.
 *
 * Reset and start the timer associated with the timer handle. Always
 * succeed. See rte_timer_reset() for details.
 *
 * @param tim
 *   The timer handle.
 * @param ticks
 *   The number of cycles (see rte_get_hpet_hz()) before the callback
 *   function is called.
 * @param type
 *   The type can be either:
 *   - PERIODICAL: The timer is automatically reloaded after execution
 *     (returns to the PENDING state)
 *   - SINGLE: The timer is one-shot, that is, the timer goes to a
 *     STOPPED state after execution.
 * @param tim_lcore
 *   The ID of the lcore where the timer callback function has to be
 *   executed. If tim_lcore is LCORE_ID_ANY, the timer library will
 *   launch it on a different core for each call (round-robin).
 * @param fct
 *   The callback function of the timer.
 * @param arg
 *   The user argument of the callback function.
 *
 * @note
 *   This API should not be called inside a timer's callback function to
 *   reset another timer; doing so could hang in certain scenarios. Instead,
 *   the rte_timer_reset() API can be called directly and its return code
 *   can be checked for success or failure.
 */
void
rte_timer_reset_sync(ng_timer_s *tim, ngTickTypeT ticks,
         rte_timer_type_e type, unsigned tim_lcore,
         rte_timer_cb_t fct, void *arg);

/**
 * Stop a timer.
 *
 * The rte_timer_stop() function stops the timer associated with the
 * timer handle *tim*. It may fail if the timer is currently running or
 * being configured.
 *
 * If the timer is pending or stopped (for instance, already expired),
 * the function will succeed. The timer handle tim must have been
 * initialized using rte_timer_init(), otherwise, undefined behavior
 * will occur.
 *
 * This function can be called safely from a timer callback. If it
 * succeeds, the timer is not referenced anymore by the timer library
 * and the timer structure can be freed (even in the callback
 * function).
 *
 * @param tim
 *   The timer handle.
 * @return
 *   - 0: Success; the timer is stopped.
 *   - (-1): The timer is in the RUNNING or CONFIG state.
 */
ng_result_e rte_timer_stop(ng_timer_s *tim);

/**
 * Loop until rte_timer_stop() succeeds.
 *
 * After a call to this function, the timer identified by *tim* is
 * stopped. See rte_timer_stop() for details.
 *
 * @param tim
 *   The timer handle.
 *
 * @note
 *   This API should not be called inside a timer's callback function to
 *   stop another timer; doing so could hang in certain scenarios. Instead, the
 *   rte_timer_stop() API can be called directly and its return code can
 *   be checked for success or failure.
 */
void rte_timer_stop_sync(ng_timer_s *tim);

/**
 * Test if a timer is pending.
 *
 * The rte_timer_pending() function tests the PENDING status
 * of the timer handle *tim*. A PENDING timer is one that has been
 * scheduled and whose function has not yet been called.
 *
 * @param tim
 *   The timer handle.
 * @return
 *   - 0: The timer is not pending.
 *   - 1: The timer is pending.
 */
int rte_timer_pending(ng_timer_s *tim);

/**
 * @warning
 * @b EXPERIMENTAL: this API may change without prior notice
 *
 * Time until the next timer on the current lcore
 * This function gives the ticks until the next timer will be active.
 *
 * @return
 *   - -EINVAL: invalid timer data instance identifier
 *   - -ENOENT: no timer pending
 *   - 0: a timer is pending and will run at next rte_timer_manage()
 *   - >0: ticks until the next timer is ready
 */
int64_t rte_timer_next_ticks(void);

/**
 * Manage the timer list and execute callback functions.
 *
 * This function must be called periodically from EAL lcores
 * main_loop(). It browses the list of pending timers and runs all
 * timers that are expired.
 *
 * The precision of the timer depends on the call frequency of this
 * function. However, the more often the function is called, the more
 * CPU resources it will use.
 *
 * @return
 *   - 0: Success
 *   - -EINVAL: timer subsystem not yet initialized
 */
ng_timer_s *rte_timer_manage(void);

/**
 * Dump statistics about timers.
 *
 * @param f
 *   A pointer to a file for output
 * @return
 *   - 0: Success
 *   - -EINVAL: timer subsystem not yet initialized
 */
ng_result_e rte_timer_dump_stats(void);

/**
 * This function is the same as rte_timer_reset(), except that it allows a
 * caller to specify the rte_timer_data instance containing the list to which
 * the timer should be added.
 *
 * @see rte_timer_reset()
 *
 * @param timer_data_id
 *   An identifier indicating which instance of timer data should be used for
 *   this operation.
 * @param tim
 *   The timer handle.
 * @param ticks
 *   The number of cycles (see rte_get_hpet_hz()) before the callback
 *   function is called.
 * @param type
 *   The type can be either:
 *   - PERIODICAL: The timer is automatically reloaded after execution
 *     (returns to the PENDING state)
 *   - SINGLE: The timer is one-shot, that is, the timer goes to a
 *     STOPPED state after execution.
 * @param tim_lcore
 *   The ID of the lcore where the timer callback function has to be
 *   executed. If tim_lcore is LCORE_ID_ANY, the timer library will
 *   launch it on a different core for each call (round-robin).
 * @param fct
 *   The callback function of the timer. This parameter can be NULL if (and
 *   only if) rte_timer_alt_manage() will be used to manage this timer.
 * @param arg
 *   The user argument of the callback function.
 * @return
 *   - 0: Success; the timer is scheduled.
 *   - (-1): Timer is in the RUNNING or CONFIG state.
 *   - -EINVAL: invalid timer_data_id
 */
ng_result_e
rte_timer_alt_reset(uint32_t timer_data_id, ng_timer_s *tim,
        ngTickTypeT ticks, rte_timer_type_e type,
        unsigned int tim_lcore, rte_timer_cb_t fct, void *arg);

/**
 * This function is the same as rte_timer_stop(), except that it allows a
 * caller to specify the rte_timer_data instance containing the list from which
 * this timer should be removed.
 *
 * @see rte_timer_stop()
 *
 * @param timer_data_id
 *   An identifier indicating which instance of timer data should be used for
 *   this operation.
 * @param tim
 *   The timer handle.
 * @return
 *   - 0: Success; the timer is stopped.
 *   - (-1): The timer is in the RUNNING or CONFIG state.
 *   - -EINVAL: invalid timer_data_id
 */
ng_result_e
rte_timer_alt_stop(uint32_t timer_data_id, ng_timer_s *tim);

/**
 * Callback function type for rte_timer_alt_manage().
 */
typedef void (*rte_timer_alt_manage_cb_t)(ng_timer_s *tim);

/**
 * Manage a set of timer lists and execute the specified callback function for
 * all expired timers. This function is similar to rte_timer_manage(), except
 * that it allows a caller to specify the timer_data instance that should
 * be operated on, as well as a set of lcore IDs identifying which timer lists
 * should be processed.  Callback functions of individual timers are ignored.
 *
 * @see rte_timer_manage()
 *
 * @param timer_data_id
 *   An identifier indicating which instance of timer data should be used for
 *   this operation.
 * @param poll_lcores
 *   An array of lcore ids identifying the timer lists that should be processed.
 *   NULL is allowed - if NULL, the timer list corresponding to the lcore
 *   calling this routine is processed (same as rte_timer_manage()).
 * @param n_poll_lcores
 *   The size of the poll_lcores array. If 'poll_lcores' is NULL, this parameter
 *   is ignored.
 * @param f
 *   The callback function which should be called for all expired timers.
 * @return
 *   - 0: success
 *   - -EINVAL: invalid timer_data_id
 */
ng_result_e
rte_timer_alt_manage(uint32_t timer_data_id, unsigned int *poll_lcores,
         int n_poll_lcores, rte_timer_alt_manage_cb_t f);

/**
 * Callback function type for rte_timer_stop_all().
 */
typedef ng_result_e (*rte_timer_stop_all_cb_t)(ng_timer_s *tim, void *arg);

/**
 * Walk the pending timer lists for the specified lcore IDs, and for each timer
 * that is encountered, stop it and call the specified callback function to
 * process it further.
 *
 * @param timer_data_id
 *   An identifier indicating which instance of timer data should be used for
 *   this operation.
 * @param walk_lcores
 *   An array of lcore ids identifying the timer lists that should be processed.
 * @param nb_walk_lcores
 *   The size of the walk_lcores array.
 * @param f
 *   The callback function which should be called for each timers. Can be NULL.
 * @param f_arg
 *   An arbitrary argument that will be passed to f, if it is called.
 * @return
 *   - 0: success
 *   - EINVAL: invalid timer_data_id
 */
ng_result_e
rte_timer_stop_all(uint32_t timer_data_id, unsigned int *walk_lcores,
       int nb_walk_lcores, rte_timer_stop_all_cb_t f, void *f_arg);

/**
 * This function is the same as rte_timer_dump_stats(), except that it allows
 * the caller to specify the rte_timer_data instance that should be used.
 *
 * @see rte_timer_dump_stats()
 *
 * @param timer_data_id
 *   An identifier indicating which instance of timer data should be used for
 *   this operation.
 * @param f
 *   A pointer to a file for output
 * @return
 *   - 0: success
 *   - -EINVAL: invalid timer_data_id
 */
ng_result_e
rte_timer_alt_dump_stats(uint32_t timer_data_id);

/*  
Function:bool __atomic_compare_exchange_n(type *ptr, type *expected, type desired, 
  bool weak, int success_memorder, int failure_memorder)

This compares the contents of *ptr with the contents of *expected. If equal, the operation is 
a read-modify-write operation that writes desired into *ptr. If they are not equal, the operation 
is a read and the current contents of *ptr are written into *expected.

If desiredis written into *ptr then true is returned£¬Otherwise, false is returned.
*/
static inline int __arch_atomic32_cmpxchg_ptr(volatile uint32_t *ptr, 
  uint32_t *_old, uint32_t _new)
{
  ng_bool_t success = ngrtos_FALSE;
  uint32_t val;            
  ngIrqTypeT b;

  b = __ng_sys_disable_irq();
  val = __LDREXW((uint32_t*)ptr); 
  if (val != *_old)
  {
    while(__STREXW(val, (uint32_t*)_old) == 0U){};
    __ng_sys_enable_irq(b);
    return ngrtos_FALSE;
  }
  else
  {
    success = __STREXW(_new,(uint32_t*)ptr) != 0U;
  }

  __ng_sys_enable_irq(b);
  return success;
}
    
/**
 * Per-lcore info for timers.
 */
struct priv_timer {
  ng_timer_s pending_head;  /**< dummy timer instance to head up list */
  rte_spinlock_t list_lock;       /**< lock to protect list access */

  /** per-core variable that true if a timer was updated on this
   *  core since last reset of the variable */
  uint32_t updated:1;
  
  /** track the current depth of the skiplist */
  uint32_t curr_skiplist_depth:5;
  
  uint32_t prev_lcore:8;              /**< used for lcore round robin */
  uint32_t count;

  /** running timer on this lcore now */
  ng_timer_s *running_tim;

#ifdef RTE_LIBRTE_TIMER_DEBUG
  /** per-lcore statistics */
  ng_timer_debug_stats_s stats;
#endif
};
typedef struct priv_timer ng_priv_timer_s;

#define FL_ALLOCATED  (1 << 0)
struct ng_timer_data {
  ng_priv_timer_s priv_timer[RTE_MAX_LCORE];
  uint8_t internal_flags;
};
typedef struct ng_timer_data ng_timer_data_s;

/* when debug is enabled, store some statistics */
#ifdef RTE_LIBRTE_TIMER_DEBUG
#define __TIMER_STAT_ADD(priv_timer, name, n) do {      \
    unsigned __lcore_id = rte_lcore_id();      \
    if (__lcore_id < RTE_MAX_LCORE)        \
      priv_timer[__lcore_id].stats.name += (n);  \
  } while(0)
#else
#define __TIMER_STAT_ADD(priv_timer, name, n) do {} while (0)
#endif

static ng_timer_data_s *rte_timer_data_arr;
static ng_bool_t rte_timer_subsystem_initialized = ngrtos_FALSE;

static inline int
timer_data_valid(uint32_t id)
{
  return rte_timer_data_arr &&
    (rte_timer_data_arr[id].internal_flags & FL_ALLOCATED);
}

/* validate ID and retrieve timer data pointer, or return error value */
#define TIMER_DATA_VALID_GET_OR_ERR_RET(id, timer_data, retval) do {  \
  if (id >= RTE_MAX_DATA_ELS || !timer_data_valid(id))    \
    return retval;            \
  timer_data = &rte_timer_data_arr[id];        \
} while (0)

ng_result_e
rte_timer_data_alloc(uint32_t *id_ptr)
{
  int i;
  ng_timer_data_s *data;

  if (!rte_timer_subsystem_initialized)
    return NG_result_nmem;
  
  for (i = 0; i < RTE_MAX_DATA_ELS; i++) {
    data = &rte_timer_data_arr[i];
    if (!(data->internal_flags & FL_ALLOCATED)) {
      data->internal_flags |= FL_ALLOCATED;

      if (id_ptr)
        *id_ptr = i;

      return NG_result_ok;
    }
  }

  return NG_result_overflow;
}

ng_result_e
rte_timer_data_dealloc(uint32_t id)
{
  ng_timer_data_s *timer_data;
  TIMER_DATA_VALID_GET_OR_ERR_RET(id, timer_data, NG_result_inval);

  timer_data->internal_flags &= ~(FL_ALLOCATED);

  return NG_result_ok;
}

/* Init the timer library. Allocate an array of timer data structs in shared
 * memory, and allocate the zeroth entry for use with original timer
 * APIs. Since the intersection of the sets of lcore ids in primary and
 * secondary processes should be empty, the zeroth entry can be shared by
 * multiple processes.
 */
ng_result_e
rte_timer_subsystem_init(void)
{
  ng_timer_data_s *data;
  const size_t data_arr_size =
      RTE_MAX_DATA_ELS * sizeof(*rte_timer_data_arr);

  if (rte_timer_subsystem_initialized) 
  {
    return NG_result_esys;
  }

  rte_timer_data_arr = ng_malloc(data_arr_size);
  if (rte_timer_data_arr == NULL)
  {
    NGRTOS_ERROR("Failed to malloc rte_timer_data_arr.\n");
    return NG_result_nmem;
  }
  
  for (int i = 0; i < RTE_MAX_DATA_ELS; i++) 
  {
    data = &rte_timer_data_arr[i];

    for (int lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) 
    {
      rte_spinlock_init(&data->priv_timer[lcore_id].list_lock);
      data->priv_timer[lcore_id].prev_lcore = lcore_id;
      data->priv_timer[lcore_id].count = 0;
    }
  }

  rte_timer_data_arr[default_data_id].internal_flags |= FL_ALLOCATED;
  rte_timer_subsystem_initialized = ngrtos_TRUE;
  return NG_result_ok;
}

void
rte_timer_subsystem_free(void)
{
  rte_timer_subsystem_initialized = ngrtos_FALSE;
  for (int i = 0; i < RTE_MAX_DATA_ELS; i++) 
  {
    ng_timer_data_s *data;
    data = &rte_timer_data_arr[i];
    for (int lcore_id = 0; lcore_id < RTE_MAX_LCORE;
         lcore_id++) {
      NGRTOS_ASSERT(data->priv_timer[lcore_id].count == 0);
    }
  }
  ng_free(rte_timer_data_arr);
  rte_timer_data_arr = NULL;
}

/* Initialize the timer handle tim for use */
void
rte_timer_init(ng_timer_s *tim)
{
  rte_timer_status_u status;

  status.state = RTE_TIMER_STOP;
  status.owner = RTE_TIMER_NO_OWNER;
  __atomic_store_n(&tim->_status.u32, status.u32, __ATOMIC_RELAXED);
}

/*
 * if timer is pending or stopped (or running on the same core than
 * us), mark timer as configuring, and on success return the previous
 * status of the timer
 */
static ng_result_e
timer_set_config_state(ng_timer_s *tim,
           rte_timer_status_u *ret_prev_status,
           ng_priv_timer_s *priv_timer)
{
  rte_timer_status_u prev_status, status;
  int success = 0;
  unsigned lcore_id;

  lcore_id = rte_lcore_id();

  /* wait that the timer is in correct status before update,
   * and mark it as being configured */
  prev_status.u32 = __atomic_load_n(&tim->_status.u32, __ATOMIC_RELAXED);

  while (success == 0)
  {
    /* timer is running on another core
     * or ready to run on local core, exit
     */
    if (prev_status.state == RTE_TIMER_RUNNING &&
        (prev_status.owner != (uint16_t)lcore_id ||
         tim != priv_timer[lcore_id].running_tim))
      return NG_result_failed;

    /* timer is being configured on another core */
    if (prev_status.state == RTE_TIMER_CONFIG)
      return NG_result_failed;

    /* here, we know that timer is stopped or pending,
     * mark it atomically as being configured */
    status.state = RTE_TIMER_CONFIG;
    status.owner = (int16_t)lcore_id;
    /* CONFIG states are acting as locked states. If the
     * timer is in CONFIG state, the state cannot be changed
     * by other threads. So, we should use ACQUIRE here.
     */
    success = __atomic_compare_exchange_n(&tim->_status.u32,
                &prev_status.u32,
                status.u32, 
                0,
                __ATOMIC_ACQUIRE,
                __ATOMIC_RELAXED);
  }

  ret_prev_status->u32 = prev_status.u32;
  return NG_result_ok;
}

/*
 * if timer is pending, mark timer as running
 */
static int
timer_set_running_state(ng_timer_s *tim)
{
  rte_timer_status_u prev_status, status;
  unsigned lcore_id = rte_lcore_id();
  int success = 0;

  /* wait that the timer is in correct status before update,
   * and mark it as running */
  prev_status.u32 = __atomic_load_n(&tim->_status.u32, __ATOMIC_RELAXED);

  while (success == 0) 
  {
    /* timer is not pending anymore */
    if (prev_status.state != RTE_TIMER_PENDING)
      return -1;

    /* we know that the timer will be pending at this point
     * mark it atomically as being running
     */
    status.state = RTE_TIMER_RUNNING;
    status.owner = (int16_t)lcore_id;
    /* RUNNING states are acting as locked states. If the
     * timer is in RUNNING state, the state cannot be changed
     * by other threads. So, we should use ACQUIRE here.
     */
    success = __atomic_compare_exchange_n(&tim->_status.u32,
                &prev_status.u32,
                status.u32, 0,
                __ATOMIC_ACQUIRE,
                __ATOMIC_RELAXED);
  }

  return 0;
}

/*
 * Return a skiplist level for a new entry.
 * This probabilistically gives a level with p=1/4 that an entry at level n
 * will also appear at level n+1.
 */
static uint32_t
timer_get_skiplist_level(unsigned curr_depth)
{
#ifdef RTE_LIBRTE_TIMER_DEBUG
  static uint32_t i, count = 0;
  static uint32_t levels[MAX_SKIPLIST_DEPTH] = {0};
#endif

  /* probability value is 1/4, i.e. all at level 0, 1 in 4 is at level 1,
   * 1 in 16 at level 2, 1 in 64 at level 3, etc. Calculated using lowest
   * bit position of a (pseudo)random number.
   */
  uint32_t rand = isaac_RNG_Rand() & (ng_UINT32_MAX - 1);
  uint32_t level = rand == 0 ? MAX_SKIPLIST_DEPTH : (rte_bsf32(rand)-1) >> 1;

  /* limit the levels used to one above our current level, so we don't,
   * for instance, have a level 0 and a level 7 without anything between
   */
  if (level > curr_depth)
    level = curr_depth;
  if (level >= MAX_SKIPLIST_DEPTH)
    level = MAX_SKIPLIST_DEPTH-1;
#ifdef RTE_LIBRTE_TIMER_DEBUG
  count ++;
  levels[level]++;
  if (count % 10000 == 0)
    for (i = 0; i < MAX_SKIPLIST_DEPTH; i++)
      NGRTOS_DEBUG("Level %u: %u\n", (unsigned)i, (unsigned)levels[i]);
#endif
  return level;
}

/*
 * For a given time value, get the entries at each level which
 * are <= that time value.
 */
static void
timer_get_prev_entries(ngTickTypeT time_val, unsigned tim_lcore,
           ng_timer_s **prev, ng_priv_timer_s *priv_timer)
{
  unsigned lvl = priv_timer[tim_lcore].curr_skiplist_depth;
  prev[lvl] = &priv_timer[tim_lcore].pending_head;
  while(lvl != 0) {
    lvl--;
    prev[lvl] = prev[lvl+1];
    while (prev[lvl]->sl_next[lvl] &&
        prev[lvl]->sl_next[lvl]->expire <= time_val)
      prev[lvl] = prev[lvl]->sl_next[lvl];
  }
}

/*
 * Given a timer node in the skiplist, find the previous entries for it at
 * all skiplist levels.
 */
static void
timer_get_prev_entries_for_node(ng_timer_s *tim, unsigned tim_lcore,
        ng_timer_s **prev,
        ng_priv_timer_s *priv_timer)
{
  int i;

  /* to get a specific entry in the list, look for just lower than the time
   * values, and then increment on each level individually if necessary
   */
  timer_get_prev_entries(tim->expire - 1, tim_lcore, prev, priv_timer);
  for (i = priv_timer[tim_lcore].curr_skiplist_depth - 1; i >= 0; i--) {
    while (prev[i]->sl_next[i] != NULL &&
        prev[i]->sl_next[i] != tim &&
        prev[i]->sl_next[i]->expire <= tim->expire)
      prev[i] = prev[i]->sl_next[i];
  }
}

/* call with lock held as necessary
 * add in list
 * timer must be in config state
 * timer must not be in a list
 */
static void
timer_add(ng_timer_s *tim, unsigned int tim_lcore,
    ng_priv_timer_s *priv_timer)
{
  unsigned lvl;
  ng_timer_s *prev[MAX_SKIPLIST_DEPTH+1];

  /* find where exactly this element goes in the list of elements
   * for each depth. */
  timer_get_prev_entries(tim->expire, tim_lcore, prev, priv_timer);

  /* now assign it a new level and add at that level */
  const unsigned tim_level = timer_get_skiplist_level(
      priv_timer[tim_lcore].curr_skiplist_depth);
  if (tim_level == priv_timer[tim_lcore].curr_skiplist_depth)
    priv_timer[tim_lcore].curr_skiplist_depth++;

  lvl = tim_level;
  while (lvl > 0) {
    tim->sl_next[lvl] = prev[lvl]->sl_next[lvl];
    prev[lvl]->sl_next[lvl] = tim;
    lvl--;
  }
  tim->sl_next[0] = prev[0]->sl_next[0];
  prev[0]->sl_next[0] = tim;

  /* save the lowest list entry into the expire field of the dummy hdr
   * NOTE: this is not atomic on 32-bit*/
  priv_timer[tim_lcore].count++;
  priv_timer[tim_lcore].pending_head.expire = priv_timer[tim_lcore].pending_head.sl_next[0]->expire;
}

/*
 * del from list, lock if needed
 * timer must be in config state
 * timer must be in a list
 */
static void
timer_del(ng_timer_s *tim, rte_timer_status_u prev_status,
    int local_is_locked, ng_priv_timer_s *priv_timer)
{
  unsigned lcore_id = rte_lcore_id();
  unsigned prev_owner = prev_status.owner;
  int i;
  ng_timer_s *prev[MAX_SKIPLIST_DEPTH+1];

  /* if timer needs is pending another core, we need to lock the
   * list; if it is on local core, we need to lock if we are not
   * called from rte_timer_manage() */
  if (prev_owner != lcore_id || !local_is_locked)
    rte_spinlock_lock(&priv_timer[prev_owner].list_lock);

  /* save the lowest list entry into the expire field of the dummy hdr.
   * NOTE: this is not atomic on 32-bit */
  if (tim == priv_timer[prev_owner].pending_head.sl_next[0])
    priv_timer[prev_owner].pending_head.expire =
        ((tim->sl_next[0] == NULL) ? 0 : tim->sl_next[0]->expire);

  /* adjust pointers from previous entries to point past this */
  timer_get_prev_entries_for_node(tim, prev_owner, prev, priv_timer);
  for (i = priv_timer[prev_owner].curr_skiplist_depth - 1; i >= 0; i--) {
    if (prev[i]->sl_next[i] == tim)
      prev[i]->sl_next[i] = tim->sl_next[i];
  }

  /* in case we deleted last entry at a level, adjust down max level */
  for (i = priv_timer[prev_owner].curr_skiplist_depth - 1; i >= 0; i--)
    if (priv_timer[prev_owner].pending_head.sl_next[i] == NULL)
      priv_timer[prev_owner].curr_skiplist_depth --;
    else
      break;

  tim->refcnt--;
  priv_timer[prev_owner].count--;
  if (prev_owner != lcore_id || !local_is_locked)
    rte_spinlock_unlock(&priv_timer[prev_owner].list_lock);
}

/* Reset and start the timer associated with the timer handle (private func) */
static ng_result_e
__rte_timer_reset(ng_timer_s *tim, 
      ngTickTypeT expire,
      ngTickTypeT period, 
      unsigned tim_lcore,
      rte_timer_cb_t fct, 
      void *arg,
      int local_is_locked,
      ng_timer_data_s *timer_data)
{
  rte_timer_status_u prev_status, status;
  ng_result_e ret;
  unsigned lcore_id = rte_lcore_id();
  ng_priv_timer_s *priv_timer = timer_data->priv_timer;

  /* round robin for tim_lcore */
  if (tim_lcore == (unsigned)LCORE_ID_ANY) 
  {
    if (lcore_id < RTE_MAX_LCORE) 
    {
      /* EAL thread with valid lcore_id */
      tim_lcore = rte_get_next_lcore(
        priv_timer[lcore_id].prev_lcore,
        0, 1);
      priv_timer[lcore_id].prev_lcore = tim_lcore;
    } else
      /* non-EAL thread do not run rte_timer_manage(),
       * so schedule the timer on the first enabled lcore. */
      tim_lcore = rte_get_next_lcore(LCORE_ID_ANY, 0, 1);
  }

  /* wait that the timer is in correct status before update,
   * and mark it as being configured */
  ret = timer_set_config_state(tim, &prev_status, priv_timer);
  if (ret != NG_result_ok)
    return ret;

  __TIMER_STAT_ADD(priv_timer, reset, 1);
  if (prev_status.state == RTE_TIMER_RUNNING &&
      lcore_id < RTE_MAX_LCORE) 
  {
    priv_timer[lcore_id].updated = 1;
  }

  /* remove it from list */
  if (prev_status.state == RTE_TIMER_PENDING) 
  {
    timer_del(tim, prev_status, local_is_locked, priv_timer);
    __TIMER_STAT_ADD(priv_timer, pending, -1);
  }

  tim->periodic = (period != 0);
  tim->interval = period;
  tim->expire = expire;
  tim->f = fct;
  tim->arg = arg;

  /* if timer needs to be scheduled on another core, we need to
   * lock the destination list; if it is on local core, we need to lock if
   * we are not called from rte_timer_manage()
   */
  if (tim_lcore != lcore_id || !local_is_locked)
    rte_spinlock_lock(&priv_timer[tim_lcore].list_lock);

  __TIMER_STAT_ADD(priv_timer, pending, 1);
  timer_add(tim, tim_lcore, priv_timer);

  /* update state: as we are in CONFIG state, only us can modify
   * the state so we don't need to use cmpset() here */
  status.state = RTE_TIMER_PENDING;
  status.owner = (int16_t)tim_lcore;
  /* The "RELEASE" ordering guarantees the memory operations above
   * the status update are observed before the update by all threads
   */
  __atomic_store_n(&tim->_status.u32, status.u32, __ATOMIC_RELEASE);

  if (tim_lcore != lcore_id || !local_is_locked)
    rte_spinlock_unlock(&priv_timer[tim_lcore].list_lock);

  return NG_result_ok;
}

/* Reset and start the timer associated with the timer handle tim */
ng_result_e
rte_timer_reset(ng_timer_s *tim, ngTickTypeT ticks,
          rte_timer_type_e type, unsigned int tim_lcore,
          rte_timer_cb_t fct, void *arg)
{
  return rte_timer_alt_reset(default_data_id, tim, ticks, type,
           tim_lcore, fct, arg);
}

ng_result_e
rte_timer_alt_reset(uint32_t timer_data_id, ng_timer_s *tim,
          ngTickTypeT ticks, rte_timer_type_e type,
          unsigned int tim_lcore, rte_timer_cb_t fct, void *arg)
{
	uint64_t period;
  ng_timer_data_s *timer_data;
  ngTickTypeT cur_time = rte_get_timer_cycles();

  TIMER_DATA_VALID_GET_OR_ERR_RET(timer_data_id, timer_data, NG_result_inval);

	if (type == PERIODICAL)
		period = ticks;
	else
		period = 0;
  return __rte_timer_reset(tim, cur_time + ticks, period, tim_lcore,
         fct, arg, 0, timer_data);
}

/* loop until rte_timer_reset() succeed */
void
rte_timer_reset_sync(ng_timer_s *tim, ngTickTypeT ticks,
         rte_timer_type_e type, unsigned tim_lcore,
         rte_timer_cb_t fct, void *arg)
{
  while (rte_timer_reset(tim, ticks, type, tim_lcore,
             fct, arg) != 0)
    kos_task_switch2other(NULL);
}

static ng_result_e
__rte_timer_stop(ng_timer_s *tim, int local_is_locked,
     ng_timer_data_s *timer_data)
{
  rte_timer_status_u prev_status, status;
  unsigned lcore_id = rte_lcore_id();
  int ret;
  ng_priv_timer_s *priv_timer = timer_data->priv_timer;

  /* wait that the timer is in correct status before update,
   * and mark it as being configured */
  ret = timer_set_config_state(tim, &prev_status, priv_timer);
  if (ret < 0)
    return NG_result_failed;

  __TIMER_STAT_ADD(priv_timer, stop, 1);
  if (prev_status.state == RTE_TIMER_RUNNING &&
      lcore_id < RTE_MAX_LCORE) 
  {
    priv_timer[lcore_id].updated = 1;
  }

  /* remove it from list */
  if (prev_status.state == RTE_TIMER_PENDING) 
  {
    timer_del(tim, prev_status, local_is_locked, priv_timer);
    __TIMER_STAT_ADD(priv_timer, pending, -1);
  }

  /* mark timer as stopped */
  status.state = RTE_TIMER_STOP;
  status.owner = RTE_TIMER_NO_OWNER;
  /* The "RELEASE" ordering guarantees the memory operations above
   * the status update are observed before the update by all threads
   */
  __atomic_store_n(&tim->_status.u32, status.u32, __ATOMIC_RELEASE);

  return NG_result_ok;
}

/* Stop the timer associated with the timer handle tim */
ng_result_e
rte_timer_stop(ng_timer_s *tim)
{
  return rte_timer_alt_stop(default_data_id, tim);
}

ng_result_e
rte_timer_alt_stop(uint32_t timer_data_id, ng_timer_s *tim)
{
  ng_timer_data_s *timer_data;

  TIMER_DATA_VALID_GET_OR_ERR_RET(timer_data_id, timer_data, NG_result_inval);

  return __rte_timer_stop(tim, 0, timer_data);
}

/* loop until rte_timer_stop() succeed */
void
rte_timer_stop_sync(ng_timer_s *tim)
{
  while (rte_timer_stop(tim) != 0)
    kos_task_switch2other(NULL);
}

/* Test the PENDING status of the timer handle tim */
int
rte_timer_pending(ng_timer_s *tim)
{
  return __atomic_load_n16(&tim->_status.state,
        __ATOMIC_RELAXED) == RTE_TIMER_PENDING;
}

extern ng_list_s  timeout_proc;
extern ng_mutex_s timeout_mutex;
extern ng_sem_s   timer_task_sem_usr;

void __ng_task_timer(void *arg)
{
  ng_list_s *head = &timeout_proc;

  for (;;) 
  {
    ngrtos_sem_wait(&timer_task_sem_usr, NG_SOFT_TIMEOUT_WAIT);

    ngrtos_mutex_enter(&timeout_mutex);
    while (!list_empty(head))
    {
      rte_timer_status_u status;
      ng_timer_s *tim, *next_tim, *run_first_tim;
      int lcore_id, dataid;
      ng_timer_data_s *timer_data;
      ng_priv_timer_s *priv_timer;

      run_first_tim = list_first_entry(head,ng_timer_s,link);
      list_del(&run_first_tim->link);
      NGRTOS_ASSERT(run_first_tim->to_flags == RTE_TIMER_RUNNING);
      run_first_tim->inque = ngrtos_FALSE;
      lcore_id = run_first_tim->lcoreid;
      dataid = run_first_tim->dataid;
      NGRTOS_ASSERT(dataid < RTE_MAX_DATA_ELS);
      NGRTOS_ASSERT(timer_data_valid(dataid));
      timer_data = &rte_timer_data_arr[dataid]; 
      priv_timer = timer_data->priv_timer;
      
      /* now scan expired list and call callbacks */
      for (tim = run_first_tim; tim != NULL; tim = next_tim) 
      {
        next_tim = tim->sl_next[0];
        priv_timer[lcore_id].updated = 0;
        priv_timer[lcore_id].running_tim = tim;
      
        /* execute callback function with list unlocked */
        tim->f(tim, tim->arg);
      
        __TIMER_STAT_ADD(priv_timer, pending, -1);
        /* the timer was stopped or reloaded by the callback
         * function, we have nothing to do here */
        if (priv_timer[lcore_id].updated == 1)
          continue;
      
        if (tim->interval == 0) 
        {
          /* remove from done list and mark timer as stopped */
          status.state = RTE_TIMER_STOP;
          status.owner = RTE_TIMER_NO_OWNER;
          priv_timer[lcore_id].count--;
          rte_spinlock_lock(&priv_timer[lcore_id].list_lock);
          tim->refcnt--;
          rte_spinlock_unlock(&priv_timer[lcore_id].list_lock);
          /* The "RELEASE" ordering guarantees the memory
           * operations above the status update are observed
           * before the update by all threads
           */
          __atomic_store_n(&tim->_status.u32, status.u32,
            __ATOMIC_RELEASE);
        }
        else 
        {
          /* keep it in list and mark timer as pending */
          rte_spinlock_lock(&priv_timer[lcore_id].list_lock);
          status.state = RTE_TIMER_PENDING;
          __TIMER_STAT_ADD(priv_timer, pending, 1);
          status.owner = (int16_t)lcore_id;
          /* The "RELEASE" ordering guarantees the memory
           * operations above the status update are observed
           * before the update by all threads
           */
          __atomic_store_n(&tim->_status.u32, status.u32,
            __ATOMIC_RELEASE);
          __rte_timer_reset(tim, tim->expire + tim->interval,
            tim->interval, lcore_id, tim->f, tim->arg, 1,
            timer_data);
          rte_spinlock_unlock(&priv_timer[lcore_id].list_lock);
        }
      }
      priv_timer[lcore_id].running_tim = NULL;
    }

    ngrtos_mutex_leave(&timeout_mutex);
  }
}

/* must be called periodically, run all timer that expired */
static ng_timer_s *
__rte_timer_manage(int dataid, ngTickTypeT cur_time)
{
  ng_timer_s *tim, *next_tim;
  ng_timer_s *run_first_tim, **pprev;
  unsigned lcore_id = rte_lcore_id();
  ng_timer_s *prev[MAX_SKIPLIST_DEPTH + 1];
  int i, ret;
  ng_priv_timer_s *priv_timer;
  ng_timer_data_s *timer_data;

  TIMER_DATA_VALID_GET_OR_ERR_RET(dataid, timer_data, NULL);
  priv_timer = timer_data->priv_timer;
  
  /* timer manager only runs on EAL thread with valid lcore_id */
  NGRTOS_ASSERT(lcore_id < RTE_MAX_LCORE);

  __TIMER_STAT_ADD(priv_timer, manage, 1);
  /* optimize for the case where per-cpu list is empty */
  if (priv_timer[lcore_id].pending_head.sl_next[0] == NULL)
    return NULL;

#ifdef RTE_ARCH_64
  /* on 64-bit the value cached in the pending_head.expired will be
   * updated atomically, so we can consult that for a quick check here
   * outside the lock */
  if (likely(priv_timer[lcore_id].pending_head.expire > cur_time))
    return NULL;
#endif

  /* browse ordered list, add expired timers in 'expired' list */
  rte_spinlock_lock(&priv_timer[lcore_id].list_lock);

  /* if nothing to do just unlock and return */
  if (priv_timer[lcore_id].pending_head.sl_next[0] == NULL ||
      priv_timer[lcore_id].pending_head.sl_next[0]->expire > cur_time) 
  {
    rte_spinlock_unlock(&priv_timer[lcore_id].list_lock);
    return NULL;
  }

  /* save start of list of expired timers */
  tim = priv_timer[lcore_id].pending_head.sl_next[0];

  /* break the existing list at current time point */
  timer_get_prev_entries(cur_time, lcore_id, prev, priv_timer);
  for (i = priv_timer[lcore_id].curr_skiplist_depth -1; i >= 0; i--) 
  {
    if (prev[i] == &priv_timer[lcore_id].pending_head)
      continue;
    priv_timer[lcore_id].pending_head.sl_next[i] =
        prev[i]->sl_next[i];
    if (prev[i]->sl_next[i] == NULL)
      priv_timer[lcore_id].curr_skiplist_depth--;
    prev[i] ->sl_next[i] = NULL;
  }

  /* transition run-list from PENDING to RUNNING */
  run_first_tim = tim;
  pprev = &run_first_tim;

  for ( ; tim != NULL; tim = next_tim) 
  {
    next_tim = tim->sl_next[0];

    ret = timer_set_running_state(tim);
    if (likely(ret == 0)) 
    {
      pprev = &tim->sl_next[0];
    } 
    else 
    {
      /* another core is trying to re-config this one,
       * remove it from local expired list
       */
      *pprev = next_tim;
    }
  }

  /* update the next to expire timer value */
  priv_timer[lcore_id].pending_head.expire =
      (priv_timer[lcore_id].pending_head.sl_next[0] == NULL) ? 0 :
    priv_timer[lcore_id].pending_head.sl_next[0]->expire;

  run_first_tim->dataid = dataid;
  run_first_tim->lcoreid = lcore_id;
  
  rte_spinlock_unlock(&priv_timer[lcore_id].list_lock);

  return run_first_tim;
}

ng_timer_s *
rte_timer_manage(void)
{
  return __rte_timer_manage(default_data_id, rte_get_timer_cycles());
}

ng_bool_t
timeout_hardclock_update(ngTickTypeT ticks)
{
  ng_timer_s *tim;
  
  ngrtos_mutex_enter(&timeout_mutex);
  tim = rte_timer_manage();
  if (tim != NULL)
  {
    ngIrqTypeT x;
    x = __ng_sys_disable_irq();
    list_add_tail(&tim->link, &timeout_proc);
    __ng_sys_enable_irq(x);
  }
  ngrtos_mutex_leave(&timeout_mutex);

  return tim != NULL;
}

/** Number of elements in the array. */
#define  RTE_DIM(a)  (sizeof (a) / sizeof ((a)[0]))

ng_result_e
rte_timer_alt_manage(uint32_t timer_data_id,
         unsigned int *poll_lcores,
         int nb_poll_lcores,
         rte_timer_alt_manage_cb_t f)
{
  unsigned int default_poll_lcores[] = {rte_lcore_id()};
  rte_timer_status_u status;
  ng_timer_s *tim, *next_tim, **pprev;
  ng_timer_s *run_first_tims[RTE_MAX_LCORE];
  unsigned int this_lcore = rte_lcore_id();
  ng_timer_s *prev[MAX_SKIPLIST_DEPTH + 1];
  ngTickTypeT cur_time;
  int i, j, ret;
  int nb_runlists = 0;
  ng_timer_data_s *data;
  ng_priv_timer_s *privp;
  uint32_t poll_lcore;

  TIMER_DATA_VALID_GET_OR_ERR_RET(timer_data_id, data, NG_result_inval);

  /* timer manager only runs on EAL thread with valid lcore_id */
  NGRTOS_ASSERT(this_lcore < RTE_MAX_LCORE);

  __TIMER_STAT_ADD(data->priv_timer, manage, 1);

  if (poll_lcores == NULL) {
    poll_lcores = default_poll_lcores;
    nb_poll_lcores = RTE_DIM(default_poll_lcores);
  }

  for (i = 0; i < nb_poll_lcores; i++) 
  {
    poll_lcore = poll_lcores[i];
    privp = &data->priv_timer[poll_lcore];

    /* optimize for the case where per-cpu list is empty */
    if (privp->pending_head.sl_next[0] == NULL)
      continue;
    cur_time = rte_get_timer_cycles();

#ifdef RTE_ARCH_64
    /* on 64-bit the value cached in the pending_head.expired will
     * be updated atomically, so we can consult that for a quick
     * check here outside the lock
     */
    if (likely(privp->pending_head.expire > cur_time))
      continue;
#endif

    /* browse ordered list, add expired timers in 'expired' list */
    rte_spinlock_lock(&privp->list_lock);

    /* if nothing to do just unlock and return */
    if (privp->pending_head.sl_next[0] == NULL ||
        privp->pending_head.sl_next[0]->expire > cur_time) 
    {
      rte_spinlock_unlock(&privp->list_lock);
      continue;
    }

    /* save start of list of expired timers */
    tim = privp->pending_head.sl_next[0];

    /* break the existing list at current time point */
    timer_get_prev_entries(cur_time, poll_lcore, prev,
               data->priv_timer);
    for (j = privp->curr_skiplist_depth - 1; j >= 0; j--) 
    {
      if (prev[j] == &privp->pending_head)
        continue;
      privp->pending_head.sl_next[j] =
        prev[j]->sl_next[j];
      if (prev[j]->sl_next[j] == NULL)
        privp->curr_skiplist_depth--;

      prev[j]->sl_next[j] = NULL;
    }

    /* transition run-list from PENDING to RUNNING */
    run_first_tims[nb_runlists] = tim;
    pprev = &run_first_tims[nb_runlists];
    nb_runlists++;

    for ( ; tim != NULL; tim = next_tim) 
    {
      next_tim = tim->sl_next[0];

      ret = timer_set_running_state(tim);
      if (likely(ret == 0)) 
      {
        pprev = &tim->sl_next[0];
      } 
      else 
      {
        /* another core is trying to re-config this one,
         * remove it from local expired list
         */
        *pprev = next_tim;
      }
    }

    /* update the next to expire timer value */
    privp->pending_head.expire =
        (privp->pending_head.sl_next[0] == NULL) ? 0 :
      privp->pending_head.sl_next[0]->expire;

    rte_spinlock_unlock(&privp->list_lock);
  }

  /* Now process the run lists */
  while (ngrtos_TRUE) 
  {
    ng_bool_t done = ngrtos_TRUE;
    ngTickTypeT min_expire = ng_TIMET_MAX;
    int min_idx = 0;

    /* Find the next oldest timer to process */
    for (i = 0; i < nb_runlists; i++) 
    {
      tim = run_first_tims[i];

      if (tim != NULL && tim->expire < min_expire) 
      {
        min_expire = tim->expire;
        min_idx = i;
        done = ngrtos_FALSE;
      }
    }

    if (done)
      break;

    tim = run_first_tims[min_idx];

    /* Move down the runlist from which we picked a timer to
     * execute
     */
    run_first_tims[min_idx] = run_first_tims[min_idx]->sl_next[0];

    data->priv_timer[this_lcore].updated = 0;
    data->priv_timer[this_lcore].running_tim = tim;

    /* Call the provided callback function */
    f(tim);

    __TIMER_STAT_ADD(data->priv_timer, pending, -1);

    /* the timer was stopped or reloaded by the callback
     * function, we have nothing to do here
     */
    if (data->priv_timer[this_lcore].updated == ngrtos_TRUE)
      continue;

    if (tim->interval == 0) 
    {
      /* remove from done list and mark timer as stopped */
      status.state = RTE_TIMER_STOP;
      status.owner = RTE_TIMER_NO_OWNER;
      data->priv_timer[this_lcore].count--;
      /* The "RELEASE" ordering guarantees the memory
       * operations above the status update are observed
       * before the update by all threads
       */
      __atomic_store_n(&tim->_status.u32, status.u32,
        __ATOMIC_RELEASE);
    } 
    else 
    {
      /* keep it in list and mark timer as pending */
      rte_spinlock_lock(
        &data->priv_timer[this_lcore].list_lock);
      status.state = RTE_TIMER_PENDING;
      __TIMER_STAT_ADD(data->priv_timer, pending, 1);
      status.owner = (int16_t)this_lcore;
      /* The "RELEASE" ordering guarantees the memory
       * operations above the status update are observed
       * before the update by all threads
       */
      __atomic_store_n(&tim->_status.u32, status.u32,
        __ATOMIC_RELEASE);
      __rte_timer_reset(tim, tim->expire + tim->interval,
        tim->interval, this_lcore, tim->f, tim->arg, 1,
        data);
      rte_spinlock_unlock(
        &data->priv_timer[this_lcore].list_lock);
    }

    data->priv_timer[this_lcore].running_tim = NULL;
  }

  return NG_result_ok;
}

/* Walk pending lists, stopping timers and calling 
 * user-specified function 
 */
ng_result_e
rte_timer_stop_all(uint32_t timer_data_id, 
  unsigned int *walk_lcores,
  int nb_walk_lcores, 
  rte_timer_stop_all_cb_t f, 
  void *f_arg)
{
  int i;
  uint32_t walk_lcore;
  ng_priv_timer_s *priv_timer;
  ng_timer_s *tim, *next_tim;
  ng_timer_data_s *timer_data;

  TIMER_DATA_VALID_GET_OR_ERR_RET(timer_data_id, 
    timer_data, NG_result_inval);

  for (i = 0; i < nb_walk_lcores; i++) 
  {
    walk_lcore = walk_lcores[i];
    priv_timer = &timer_data->priv_timer[walk_lcore];

    rte_spinlock_lock(&priv_timer->list_lock);

    for (tim = priv_timer->pending_head.sl_next[0];
         tim != NULL;
         tim = next_tim) 
    {
      next_tim = tim->sl_next[0];

      /* Call timer_stop with lock held */
      __rte_timer_stop(tim, 1, timer_data);

      if (f)
        f(tim, f_arg);
    }

    rte_spinlock_unlock(&priv_timer->list_lock);
  }

  return NG_result_ok;
}

int64_t
rte_timer_next_ticks(void)
{
  unsigned int lcore_id = rte_lcore_id();
  ng_timer_data_s *timer_data;
  ng_priv_timer_s *priv_timer;
  const ng_timer_s *tm;
  ngTickTypeT cur_time;
  int64_t left = -NG_result_nent;

  TIMER_DATA_VALID_GET_OR_ERR_RET(default_data_id, 
    timer_data, -NG_result_inval);

  priv_timer = timer_data->priv_timer;
  cur_time = rte_get_timer_cycles();

  rte_spinlock_lock(&priv_timer[lcore_id].list_lock);
  tm = priv_timer[lcore_id].pending_head.sl_next[0];
  if (tm) 
  {
    left = tm->expire - cur_time;
    if (left < 0)
      left = 0;
  }
  rte_spinlock_unlock(&priv_timer[lcore_id].list_lock);

  return left;
}

/* dump statistics about timers */
static void
__rte_timer_dump_stats(ng_timer_data_s *timer_data)
{
#ifdef RTE_LIBRTE_TIMER_DEBUG
  ng_timer_debug_stats_s sum;
  unsigned lcore_id;
  ng_priv_timer_s *priv_timer = timer_data->priv_timer;

  ng_memset(&sum, 0, sizeof(sum));
  for (lcore_id = 0; lcore_id < RTE_MAX_LCORE; lcore_id++) {
    sum.reset += priv_timer[lcore_id].stats.reset;
    sum.stop += priv_timer[lcore_id].stats.stop;
    sum.manage += priv_timer[lcore_id].stats.manage;
    sum.pending += priv_timer[lcore_id].stats.pending;
  }
  NGRTOS_PRINT("Timer statistics:\n");
  NGRTOS_PRINT("  reset = %"__PrxULL"\n", sum.reset);
  NGRTOS_PRINT("  stop = %"__PrxULL"\n", sum.stop);
  NGRTOS_PRINT("  manage = %"__PrxULL"\n", sum.manage);
  NGRTOS_PRINT("  pending = %"__PrxULL"\n", sum.pending);
#else
  NGRTOS_UNUSED(timer_data);
  NGRTOS_PRINT("No timer statistics, RTE_LIBRTE_TIMER_DEBUG is disabled\n");
#endif
}

ng_result_e
rte_timer_alt_dump_stats(uint32_t timer_data_id)
{
  ng_timer_data_s *timer_data;

  TIMER_DATA_VALID_GET_OR_ERR_RET(timer_data_id, timer_data, NG_result_inval);

  __rte_timer_dump_stats(timer_data);

  return NG_result_ok;
}

ng_result_e
rte_timer_dump_stats(void)
{
  return rte_timer_alt_dump_stats(default_data_id);
}

ng_result_e
ngrtos_timer_subsystem_startup(void)
{
  return rte_timer_subsystem_init();
}

void
ngrtos_timer_subsystem_destroy(void)
{
  rte_timer_subsystem_free();
}

void
ngrtos_timer_set(ng_timer_s *to, ng_time_t ms, ng_timer_cb_f fn, 
  void *arg, int flags)
{
  ngRTOS_BIT_UNSET(flags, NG_TIMER_FLAG_From_sched);
  rte_timer_init(to);
  to->f        = fn;
  to->arg      = arg;
  to->interval = ng_ms2ticks(ms);
  to->periodic = ngRTOS_BIT_TEST(flags, NG_TIMER_FLAG_Periodic);
  to->refcnt   = 0;
  to->inque    = ngrtos_FALSE;
  __list_poison_entry(&to->link);
}

void
ngrtos_timer_add(ng_timer_s *timer, int ms)
{
  rte_timer_type_e type;
  
  ngrtos_mutex_enter(&timeout_mutex);

  if (timer->periodic)
    type = PERIODICAL;
  else
    type = SINGLE;
  rte_timer_reset(timer, ng_ms2ticks(ms), type, 
    rte_lcore_id(), timer->f, timer->arg);
  timer->refcnt++;
  ngrtos_mutex_leave(&timeout_mutex);
}

void
ngrtos_timer_del(ng_timer_s *to)
{
  ngrtos_mutex_enter(&timeout_mutex);
  rte_timer_stop(to);
  ngrtos_mutex_leave(&timeout_mutex);
}

#endif
