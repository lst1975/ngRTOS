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

/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2010-2015 Intel Corporation
 * Copyright (c) 2007,2008 Kip Macy kmacy@freebsd.org
 * All rights reserved.
 * Derived from FreeBSD's bufring.h
 * Used as BSD-3 Licensed with permission from Kip Macy.
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/* FreeRTOS includes. */
#include "ng_defs.h"
#include "ng_align.h"
#include "ng_ring.h"
#include "ng_rand.h"
#include "mem/ng_mem.h"

static inline void ngrtos_free(void *ptr)
{
  ng_free(ptr);
}
static inline void *ngrtos_malloc(uint32_t sz)
{
  return ng_malloc(sz);
}

typedef uint64_t ngrtos_uintptr_t;
typedef int32_t ngrtos_ssize_t;
typedef uint32_t ngrtos_uint64_t;
typedef uint64_t unaligned_uint64_t;

#define __ATOMIC_RELAXED 0
#define __ATOMIC_ACQUIRE 1
#define __rte_always_inline inline
/**
 * definition to mark a variable or function parameter as used so
 * as to avoid a compiler warning
 */
#define RTE_SET_USED(x) (void)(x)

#define rte_int128_t 0

#define rte_smp_wmb() __DMB()
#define rte_smp_rmb() __DMB()

/** enqueue/dequeue behavior types */
#define RTE_RING_QUEUE_FIXED    0 /** Enq/Deq a fixed number of items from a ring */
#define RTE_RING_QUEUE_VARIABLE 1 /** Enq/Deq as many items as possible from ring */

/**
 * structures to hold a pair of head/tail values and other metadata.
 * Depending on sync_type format of that structure might be different,
 * but offset for *sync_type* and *tail* values should remain the same.
 */
struct rte_ring_headtail {
  volatile rte_ring_size_t head;      /**< prod/consumer head. */
  volatile rte_ring_size_t tail;      /**< prod/consumer tail. */
};

/**
 * An RTE ring structure.
 *
 * The producer and the consumer have a head and a tail index. The particularity
 * of these index is that they are not between 0 and size(ring). These indexes
 * are between 0 and 2^32, and we mask their value when we access the ring[]
 * field. Thanks to this assumption, we can do subtractions between 2 index
 * values in a modulo-32bit base: that's why the overflow of the indexes is not
 * a problem.
 */
struct rte_ring {
  struct rte_ring_headtail prod; /** Ring producer status. */
  struct rte_ring_headtail cons; /** Ring consumer status. */
  rte_ring_size_t size;          /**< Size of ring. */
  rte_ring_size_t mask;          /**< Mask (size-1) of ring. */
  uint8_t *buf;                  /**< Memzone, if any, containing the rte_ring */
  rte_ring_size_t prod_sync_type:4;     /**< sync type of prod */
  rte_ring_size_t cons_sync_type:4;     /**< sync type of cons */
  rte_ring_size_t flags:8;              /**< Flags supplied at creation. */
};

#define RING_F_SP_ENQ 0x0001 /**< The default enqueue is "single-producer". */
#define RING_F_SC_DEQ 0x0002 /**< The default dequeue is "single-consumer". */

/**
 * Ring is to hold exactly requested number of entries.
 * Without this flag set, the ring size requested must be a power of 2, and the
 * usable space will be that size - 1. With the flag, the requested size will
 * be rounded up to the next power of two, but the usable space will be exactly
 * that requested. Worst case, if a power-of-2 size is requested, half the
 * ring space will be wasted.
 */
#define RING_F_EXACT_SZ   0x0004
#define RTE_RING_SZ_MASK  (0x7fffU) /**< Ring size mask */

/*
 * Atomic exclusive load from addr, it returns the 32-bit content of
 * *addr while making it 'monitored', when it is written by someone
 * else, the 'monitored' state is cleared and an event is generated
 * implicitly to exit WFE.
 */
#if __config_UseRingMinSize
#define __RTE_ARM_LOAD_EXC(src, dst, order) { RTE_SET_USED(order);  dst = __LDREXH(src); }
#else
#define __RTE_ARM_LOAD_EXC(src, dst, order) { RTE_SET_USED(order);  dst = __LDREXW(src); }
#endif

/* Send an event to quit WFE. */
#define __RTE_ARM_SEVL() __SEV()

/* Put processor into low power WFE(Wait For Event) state. */
#define __RTE_ARM_WFE() __WFE()

/* Between load and load. there might be cpu reorder in weak model
 * (powerpc/arm).
 * There are 2 choices for the users
 * 1.use rmb() memory barrier
 * 2.use one-direction load_acquire/store_release barrier
 * It depends on performance test results.
 */
static __rte_always_inline void
rte_wait_until_equal(volatile rte_ring_size_t *addr, 
  rte_ring_size_t expected, int memorder)
{
  rte_ring_size_t value;

  __RTE_ARM_LOAD_EXC(addr, value, memorder)
  if (value != expected) 
  {
    __RTE_ARM_SEVL();
    do {
      __RTE_ARM_WFE();
      __RTE_ARM_LOAD_EXC(addr, value, memorder)
    } while (value != expected);
  }
}
    
static __rte_always_inline void
__rte_ring_update_tail(struct rte_ring_headtail *ht, rte_ring_size_t old_val,
    rte_ring_size_t new_val, uint16_t single, uint16_t enqueue)
{
  if (enqueue)
    rte_smp_wmb();
  else
    rte_smp_rmb();
  /*
   * If there are other enqueues/dequeues in progress that preceded us,
   * we need to wait for them to complete
   */
  if (!single)
    rte_wait_until_equal(&ht->tail, old_val, __ATOMIC_RELAXED);

  ht->tail = new_val;
}

/**
 * Atomic compare and set.
 *
 * (atomic) equivalent to:
 *   if (*dst == exp)
 *     *dst = src (all 32-bit words)
 *
 * @param dst
 *   The destination location into which the value will be written.
 * @param exp
 *   The expected value.
 * @param src
 *   The new value.
 * @return
 *   Non-zero on success; 0 on failure.
 */
static __rte_always_inline int 
__arch_atomic_cmpxchg(volatile rte_ring_size_t *ptr, 
  rte_ring_size_t _old, rte_ring_size_t _new)
{
  rte_ring_size_t val;

#if __config_UseRingMinSize
  val = __LDREXH(ptr);
#else
  val = __LDREXW(ptr);
#endif
  if (likely(val == _old))
  {
#if __config_UseRingMinSize
    return !__STREXH(_new, ptr);
#else    
    return !__STREXW(_new, ptr);
#endif
  }
  else
  {
    return 0;
  }
}

static __rte_always_inline int 
arch_atomic_cmpxchg(volatile rte_ring_size_t *ptr, 
  rte_ring_size_t _old, rte_ring_size_t _new)
{
  int ret;
  ngIrqTypeT b;
  b = __ng_sys_disable_irq();
  ret = __arch_atomic_cmpxchg(ptr,_old,_new);
  __ng_sys_enable_irq(b);
  return ret;
}
#define rte_atomic_cmpset arch_atomic_cmpxchg

/**
 * @internal This function updates the producer head for enqueue
 *
 * @param r
 *   A pointer to the ring structure
 * @param is_sp
 *   Indicates whether multi-producer path is needed or not
 * @param n
 *   The number of elements we will want to enqueue, i.e. how far should the
 *   head be moved
 * @param behavior
 *   RTE_RING_QUEUE_FIXED:    Enqueue a fixed number of items from a ring
 *   RTE_RING_QUEUE_VARIABLE: Enqueue as many items as possible from ring
 * @param old_head
 *   Returns head value as it was before the move, i.e. where enqueue starts
 * @param new_head
 *   Returns the current/new head value i.e. where enqueue finishes
 * @param free_entries
 *   Returns the amount of free space in the ring BEFORE head was moved
 * @return
 *   Actual number of objects enqueued.
 *   If behavior == RTE_RING_QUEUE_FIXED, this will be 0 or n only.
 */
static __rte_always_inline unsigned int
__rte_ring_move_prod_head(struct rte_ring *r, rte_ring_size_t n, 
    uint16_t is_sp, uint16_t behavior, rte_ring_size_t *old_head, 
    rte_ring_size_t *new_head, rte_ring_size_t *free_entries)
{
  const rte_ring_size_t capacity = r->size - 1; /**< Usable size of ring */
  unsigned int max;
  int success;

  rte_prefetch0(&r->prod.head);
  max = n;

  do {
    /* Reset n to the initial burst count */
    n = max;

    *old_head = r->prod.head;

    /* add rmb barrier to avoid load/load reorder in weak
     * memory model. It is noop on x86
     */
    rte_smp_rmb();

    /*
     *  The subtraction is done between two unsigned 32bits value
     * (the result is always modulo 32 bits even if we have
     * *old_head > cons_tail). So 'free_entries' is always between 0
     * and capacity (which is < size).
     */
    *free_entries = (capacity + r->cons.tail - *old_head);

    /* check that we have enough room in ring */
    if (unlikely(n > *free_entries))
      n = (behavior == RTE_RING_QUEUE_FIXED) ?
          0 : *free_entries;

    if (n == 0)
      return 0;

    *new_head = *old_head + n;
    if (is_sp)
      r->prod.head = *new_head, success = 1;
    else
    {
      success = rte_atomic_cmpset(&r->prod.head,
          *old_head, *new_head);
    }
  } 
  while (unlikely(!success));
  return n;
}

/**
 * @internal This function updates the consumer head for dequeue
 *
 * @param r
 *   A pointer to the ring structure
 * @param is_sc
 *   Indicates whether multi-consumer path is needed or not
 * @param n
 *   The number of elements we will want to dequeue, i.e. how far should the
 *   head be moved
 * @param behavior
 *   RTE_RING_QUEUE_FIXED:    Dequeue a fixed number of items from a ring
 *   RTE_RING_QUEUE_VARIABLE: Dequeue as many items as possible from ring
 * @param old_head
 *   Returns head value as it was before the move, i.e. where dequeue starts
 * @param new_head
 *   Returns the current/new head value i.e. where dequeue finishes
 * @param entries
 *   Returns the number of entries in the ring BEFORE head was moved
 * @return
 *   - Actual number of objects dequeued.
 *     If behavior == RTE_RING_QUEUE_FIXED, this will be 0 or n only.
 */
static __rte_always_inline unsigned int
__rte_ring_move_cons_head(struct rte_ring *r, rte_ring_size_t n, 
    uint16_t is_sc, uint16_t behavior, rte_ring_size_t *old_head, 
    rte_ring_size_t *new_head, rte_ring_size_t *entries)
{
  unsigned int max;
  int success;

  rte_prefetch0(&r->cons.head);
  max = n;
  
  /* move cons.head atomically */
  do {
    /* Restore n as it may change every loop */
    n = max;

    *old_head = r->cons.head;

    /* add rmb barrier to avoid load/load reorder in weak
     * memory model. It is noop on x86
     */
    rte_smp_rmb();

    /* The subtraction is done between two unsigned 32bits value
     * (the result is always modulo 32 bits even if we have
     * cons_head > prod_tail). So 'entries' is always between 0
     * and size(ring)-1.
     */
    *entries = (r->prod.tail - *old_head);

    /* Set the actual entries for dequeue */
    if (n > *entries)
      n = (behavior == RTE_RING_QUEUE_FIXED) ? 0 : *entries;

    if (unlikely(n == 0))
      return 0;

    *new_head = *old_head + n;
    if (is_sc) 
    {
      r->cons.head = *new_head;
      rte_smp_rmb();
      success = 1;
    } 
    else 
    {
      success = rte_atomic_cmpset(&r->cons.head, *old_head,
          *new_head);
    }
  } 
  while (unlikely(!success));
  
  return n;
}
    
/* the actual enqueue of elements on the ring.
 * Placed here since identical code needed in both
 * single and multi producer enqueue functions.
 */
static __rte_always_inline void
__rte_ring_enqueue_elems(struct rte_ring *r, rte_ring_size_t prod_head,
    const void *obj_table, rte_ring_size_t nr_num)
{
  rte_ring_size_t idx, sz;
  const uint8_t *buf = (const uint8_t *)obj_table;
  
  /* Normalize to uint32_t */
  idx = prod_head & r->mask;
  if (likely(idx + nr_num < r->size)) {
    ng_memcpy(&r->buf[idx], buf, nr_num);
  } 
  else 
  {
    sz = r->size - idx;
    if (sz) ng_memcpy(&r->buf[idx], buf, sz);
    /* Start at the beginning */
    ng_memcpy(&r->buf[0], &buf[sz], nr_num - sz);
  }
}

/* the actual dequeue of elements from the ring.
 * Placed here since identical code needed in both
 * single and multi producer enqueue functions.
 */
static __rte_always_inline void
__rte_ring_dequeue_elems(struct rte_ring *r, rte_ring_size_t cons_head,
    void *obj_table, rte_ring_size_t nr_num)
{
  rte_ring_size_t idx;
  uint8_t *buf = (uint8_t *)obj_table;
  
  /* Normalize to uint32_t */
  idx = cons_head & r->mask;
  if (likely(idx + nr_num < r->size)) {
    ng_memcpy(buf, &r->buf[idx], nr_num);
  } 
  else 
  {
    rte_ring_size_t sz = r->size - idx;
    if (sz) ng_memcpy(buf, &r->buf[idx], sz);
    /* Start at the beginning */
    ng_memcpy(&buf[sz], &r->buf[0], nr_num - sz);
  }
}

/**
 * @internal Enqueue several objects on the ring
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of objects.
 * @param esize
 *   The size of ring element, in bytes. It must be a multiple of 4.
 *   This must be the same value used while creating the ring. Otherwise
 *   the results are undefined.
 * @param n
 *   The number of objects to add in the ring from the obj_table.
 * @param behavior
 *   RTE_RING_QUEUE_FIXED:    Enqueue a fixed number of items from a ring
 *   RTE_RING_QUEUE_VARIABLE: Enqueue as many items as possible from ring
 * @param is_sp
 *   Indicates whether to use single producer or multi-producer head update
 * @param free_space
 *   returns the amount of space after the enqueue operation has finished
 * @return
 *   Actual number of objects enqueued.
 *   If behavior == RTE_RING_QUEUE_FIXED, this will be 0 or n only.
 */
static __rte_always_inline unsigned int
__rte_ring_do_enqueue_elem(struct rte_ring *r, const void *obj_table,
    rte_ring_size_t n, uint16_t behavior, uint16_t is_sp, rte_ring_size_t *free_space)
{
  rte_ring_size_t prod_head, prod_next;
  rte_ring_size_t free_entries;

  n = __rte_ring_move_prod_head(r, n, is_sp, behavior,
      &prod_head, &prod_next, &free_entries);
  if (n == 0)
    goto end;

  __rte_ring_enqueue_elems(r, prod_head, obj_table, n);

  __rte_ring_update_tail(&r->prod, prod_head, prod_next, is_sp, 1);
end:
  if (free_space != NULL)
    *free_space = free_entries - n;
  return n;
}

/**
 * @internal Dequeue several objects from the ring
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of objects.
 * @param esize
 *   The size of ring element, in bytes. It must be a multiple of 4.
 *   This must be the same value used while creating the ring. Otherwise
 *   the results are undefined.
 * @param n
 *   The number of objects to pull from the ring.
 * @param behavior
 *   RTE_RING_QUEUE_FIXED:    Dequeue a fixed number of items from a ring
 *   RTE_RING_QUEUE_VARIABLE: Dequeue as many items as possible from ring
 * @param is_sc
 *   Indicates whether to use single consumer or multi-consumer head update
 * @param available
 *   returns the number of remaining ring entries after the dequeue has finished
 * @return
 *   - Actual number of objects dequeued.
 *     If behavior == RTE_RING_QUEUE_FIXED, this will be 0 or n only.
 */
static __rte_always_inline unsigned int
__rte_ring_do_dequeue_elem(struct rte_ring *r, void *obj_table,
    rte_ring_size_t n, uint16_t behavior, uint16_t is_sc, rte_ring_size_t *available)
{
  rte_ring_size_t cons_head, cons_next;
  rte_ring_size_t entries;

  n = __rte_ring_move_cons_head(r, n, is_sc, behavior,
      &cons_head, &cons_next, &entries);
  if (n == 0)
    goto end;

  __rte_ring_dequeue_elems(r, cons_head, obj_table, n);

  __rte_ring_update_tail(&r->cons, cons_head, cons_next, is_sc, 0);

end:
  if (available != NULL)
    *available = entries - n;
  return n;
}

/**
 * Calculate the memory size needed for a ring with given element size
 *
 * This function returns the number of bytes needed for a ring, given
 * the number of elements in it and the size of the element. This value
 * is the sum of the size of the structure rte_ring and the size of the
 * memory needed for storing the elements. The value is aligned to a cache
 * line size.
 *
 * @param esize
 *   The size of ring element, in bytes. It must be a multiple of 4.
 * @param count
 *   The number of elements in the ring (must be a power of 2).
 * @return
 *   - The memory size needed for the ring on success.
 *   - -EINVAL - esize is not a multiple of 4 or count provided is not a
 *     power of 2.
 */
/* true if x is a power of 2 */
#define __IS_POWEROF2(x) ((((x)-1) & (x)) == 0)

/* by default set head/tail distance as 1/8 of ring capacity */
#define HTD_MAX_DEF  8

#define RTE_MD_RING 0

#define RTE_ERR(tx, s, ...) NGRTOS_ERROR(s, ##__VA_ARGS__)
#define RTE_ASSERT(x, ...) NGRTOS_ASSERT(x, ##__VA_ARGS__)

#define NGRTOS_OK      0
#define NGRTOS_EINVAL   1
#define NGRTOS_ENOENT  2
#define NGRTOS_ENOBUFS 3

/**
 * Enqueue several objects on the ring (multi-producers safe).
 *
 * This function uses a "compare and set" instruction to move the
 * producer index atomically.
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of objects.
 * @param esize
 *   The size of ring element, in bytes. It must be a multiple of 4.
 *   This must be the same value used while creating the ring. Otherwise
 *   the results are undefined.
 * @param n
 *   The number of objects to add in the ring from the obj_table.
 * @param free_space
 *   if non-NULL, returns the amount of space in the ring after the
 *   enqueue operation has finished.
 * @return
 *   The number of objects enqueued, either 0 or n
 */
static __rte_always_inline unsigned int
rte_ring_mp_enqueue_bulk_elem(struct rte_ring *r, const void *obj_table,
    rte_ring_size_t n, rte_ring_size_t *free_space)
{
  return __rte_ring_do_enqueue_elem(r, obj_table, n,
      RTE_RING_QUEUE_FIXED, RTE_RING_SYNC_MT, free_space);
}

/**
 * Enqueue several objects on a ring
 *
 * @warning This API is NOT multi-producers safe
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of objects.
 * @param esize
 *   The size of ring element, in bytes. It must be a multiple of 4.
 *   This must be the same value used while creating the ring. Otherwise
 *   the results are undefined.
 * @param n
 *   The number of objects to add in the ring from the obj_table.
 * @param free_space
 *   if non-NULL, returns the amount of space in the ring after the
 *   enqueue operation has finished.
 * @return
 *   The number of objects enqueued, either 0 or n
 */
static __rte_always_inline unsigned int
rte_ring_sp_enqueue_bulk_elem(struct rte_ring *r, const void *obj_table,
    rte_ring_size_t n, rte_ring_size_t *free_space)
{
  return __rte_ring_do_enqueue_elem(r, obj_table, n,
      RTE_RING_QUEUE_FIXED, RTE_RING_SYNC_ST, free_space);
}

/**
 * Enqueue several objects on a ring.
 *
 * This function calls the multi-producer or the single-producer
 * version depending on the default behavior that was specified at
 * ring creation time (see flags).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of objects.
 * @param esize
 *   The size of ring element, in bytes. It must be a multiple of 4.
 *   This must be the same value used while creating the ring. Otherwise
 *   the results are undefined.
 * @param n
 *   The number of objects to add in the ring from the obj_table.
 * @param free_space
 *   if non-NULL, returns the amount of space in the ring after the
 *   enqueue operation has finished.
 * @return
 *   The number of objects enqueued, either 0 or n
 */
static __rte_always_inline unsigned int
rte_ring_enqueue_bulk_elem(struct rte_ring *r, const void *obj_table,
    rte_ring_size_t n, rte_ring_size_t *free_space)
{
  switch (r->prod_sync_type) {
  case RTE_RING_SYNC_MT:
    return rte_ring_mp_enqueue_bulk_elem(r, obj_table, n, free_space);
  case RTE_RING_SYNC_ST:
    return rte_ring_sp_enqueue_bulk_elem(r, obj_table, n, free_space);
  }

  /* valid ring should never reach this point */
  RTE_ASSERT(0);
  if (free_space != NULL)
    *free_space = 0;
  return 0;
}

/**
 * Enqueue one object on a ring (multi-producers safe).
 *
 * This function uses a "compare and set" instruction to move the
 * producer index atomically.
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj
 *   A pointer to the object to be added.
 * @param esize
 *   The size of ring element, in bytes. It must be a multiple of 4.
 *   This must be the same value used while creating the ring. Otherwise
 *   the results are undefined.
 * @return
 *   - 0: Success; objects enqueued.
 *   - -ENOBUFS: Not enough room in the ring to enqueue; no object is enqueued.
 */
int
rte_ring_mp_enqueue_elem(struct rte_ring *r, void *obj, rte_ring_size_t n)
{
  return rte_ring_mp_enqueue_bulk_elem(r, obj, n, NULL);
}

/**
 * Enqueue one object on a ring
 *
 * @warning This API is NOT multi-producers safe
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj
 *   A pointer to the object to be added.
 * @param esize
 *   The size of ring element, in bytes. It must be a multiple of 4.
 *   This must be the same value used while creating the ring. Otherwise
 *   the results are undefined.
 * @return
 *   - 0: Success; objects enqueued.
 *   - -ENOBUFS: Not enough room in the ring to enqueue; no object is enqueued.
 */
unsigned int
rte_ring_sp_enqueue_elem(void *r, void *obj, rte_ring_size_t n)
{
  return rte_ring_sp_enqueue_bulk_elem((struct rte_ring *)r, obj, n, NULL);
}

/**
 * Enqueue one object on a ring.
 *
 * This function calls the multi-producer or the single-producer
 * version, depending on the default behaviour that was specified at
 * ring creation time (see flags).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj
 *   A pointer to the object to be added.
 * @param esize
 *   The size of ring element, in bytes. It must be a multiple of 4.
 *   This must be the same value used while creating the ring. Otherwise
 *   the results are undefined.
 * @return
 *   - 0: Success; objects enqueued.
 *   - -ENOBUFS: Not enough room in the ring to enqueue; no object is enqueued.
 */
int
rte_ring_enqueue_elem(struct rte_ring *r, void *obj, rte_ring_size_t n)
{
  return rte_ring_enqueue_bulk_elem(r, obj, n, NULL);
}

/**
 * Dequeue several objects from a ring (multi-consumers safe).
 *
 * This function uses a "compare and set" instruction to move the
 * consumer index atomically.
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of objects that will be filled.
 * @param esize
 *   The size of ring element, in bytes. It must be a multiple of 4.
 *   This must be the same value used while creating the ring. Otherwise
 *   the results are undefined.
 * @param n
 *   The number of objects to dequeue from the ring to the obj_table.
 * @param available
 *   If non-NULL, returns the number of remaining ring entries after the
 *   dequeue has finished.
 * @return
 *   The number of objects dequeued, either 0 or n
 */
static __rte_always_inline unsigned int
rte_ring_mc_dequeue_bulk_elem(struct rte_ring *r, void *obj_table,
    rte_ring_size_t n, rte_ring_size_t *available)
{
  return __rte_ring_do_dequeue_elem(r, obj_table, n,
      RTE_RING_QUEUE_FIXED, RTE_RING_SYNC_MT, available);
}

/**
 * Dequeue several objects from a ring (NOT multi-consumers safe).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of objects that will be filled.
 * @param esize
 *   The size of ring element, in bytes. It must be a multiple of 4.
 *   This must be the same value used while creating the ring. Otherwise
 *   the results are undefined.
 * @param n
 *   The number of objects to dequeue from the ring to the obj_table,
 *   must be strictly positive.
 * @param available
 *   If non-NULL, returns the number of remaining ring entries after the
 *   dequeue has finished.
 * @return
 *   The number of objects dequeued, either 0 or n
 */
static __rte_always_inline unsigned int
rte_ring_sc_dequeue_bulk_elem(struct rte_ring *r, void *obj_table,
    rte_ring_size_t n, rte_ring_size_t *available)
{
  return __rte_ring_do_dequeue_elem(r, obj_table, n,
      RTE_RING_QUEUE_FIXED, RTE_RING_SYNC_ST, available);
}

/**
 * Dequeue several objects from a ring.
 *
 * This function calls the multi-consumers or the single-consumer
 * version, depending on the default behaviour that was specified at
 * ring creation time (see flags).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of objects that will be filled.
 * @param esize
 *   The size of ring element, in bytes. It must be a multiple of 4.
 *   This must be the same value used while creating the ring. Otherwise
 *   the results are undefined.
 * @param n
 *   The number of objects to dequeue from the ring to the obj_table.
 * @param available
 *   If non-NULL, returns the number of remaining ring entries after the
 *   dequeue has finished.
 * @return
 *   The number of objects dequeued, either 0 or n
 */
static __rte_always_inline unsigned int
rte_ring_dequeue_bulk_elem(struct rte_ring *r, void *obj_table,
    rte_ring_size_t n, rte_ring_size_t *available)
{
  switch (r->cons_sync_type) {
  case RTE_RING_SYNC_MT:
    return rte_ring_mc_dequeue_bulk_elem(r, obj_table, n, available);
  case RTE_RING_SYNC_ST:
    return rte_ring_sc_dequeue_bulk_elem(r, obj_table, n, available);
  }

  /* valid ring should never reach this point */
  RTE_ASSERT(0);
  if (available != NULL)
    *available = 0;
  return 0;
}

/**
 * Dequeue one object from a ring (multi-consumers safe).
 *
 * This function uses a "compare and set" instruction to move the
 * consumer index atomically.
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_p
 *   A pointer to the object that will be filled.
 * @param esize
 *   The size of ring element, in bytes. It must be a multiple of 4.
 *   This must be the same value used while creating the ring. Otherwise
 *   the results are undefined.
 * @return
 *   - 0: Success; objects dequeued.
 *   - -ENOENT: Not enough entries in the ring to dequeue; no object is
 *     dequeued.
 */
int
rte_ring_mc_dequeue_elem(struct rte_ring *r, void *obj_p, rte_ring_size_t n)
{
  return rte_ring_mc_dequeue_bulk_elem(r, obj_p, n, NULL);
}

/**
 * Dequeue one object from a ring (NOT multi-consumers safe).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_p
 *   A pointer to the object that will be filled.
 * @param esize
 *   The size of ring element, in bytes. It must be a multiple of 4.
 *   This must be the same value used while creating the ring. Otherwise
 *   the results are undefined.
 * @return
 *   - 0: Success; objects dequeued.
 *   - -ENOENT: Not enough entries in the ring to dequeue, no object is
 *     dequeued.
 */
unsigned int
rte_ring_sc_dequeue_elem(void *r, void *obj_p, rte_ring_size_t n)
{
  return rte_ring_sc_dequeue_bulk_elem((struct rte_ring *)r, obj_p, n, NULL);
}

/**
 * Dequeue one object from a ring.
 *
 * This function calls the multi-consumers or the single-consumer
 * version depending on the default behaviour that was specified at
 * ring creation time (see flags).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_p
 *   A pointer to the object that will be filled.
 * @param esize
 *   The size of ring element, in bytes. It must be a multiple of 4.
 *   This must be the same value used while creating the ring. Otherwise
 *   the results are undefined.
 * @return
 *   - 0: Success, objects dequeued.
 *   - -ENOENT: Not enough entries in the ring to dequeue, no object is
 *     dequeued.
 */
int
rte_ring_dequeue_elem(struct rte_ring *r, void *obj_p, rte_ring_size_t n)
{
  return rte_ring_dequeue_bulk_elem(r, obj_p, n, NULL);
}

/**
 * Enqueue several objects on the ring (multi-producers safe).
 *
 * This function uses a "compare and set" instruction to move the
 * producer index atomically.
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of objects.
 * @param esize
 *   The size of ring element, in bytes. It must be a multiple of 4.
 *   This must be the same value used while creating the ring. Otherwise
 *   the results are undefined.
 * @param n
 *   The number of objects to add in the ring from the obj_table.
 * @param free_space
 *   if non-NULL, returns the amount of space in the ring after the
 *   enqueue operation has finished.
 * @return
 *   - n: Actual number of objects enqueued.
 */
unsigned int
rte_ring_mp_enqueue_burst_elem(void *r, const void *obj_table,
    rte_ring_size_t n, rte_ring_size_t *free_space)
{
  return __rte_ring_do_enqueue_elem((struct rte_ring *)r, obj_table, n,
      RTE_RING_QUEUE_VARIABLE, RTE_RING_SYNC_MT, free_space);
}

/**
 * Enqueue several objects on a ring
 *
 * @warning This API is NOT multi-producers safe
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of objects.
 * @param esize
 *   The size of ring element, in bytes. It must be a multiple of 4.
 *   This must be the same value used while creating the ring. Otherwise
 *   the results are undefined.
 * @param n
 *   The number of objects to add in the ring from the obj_table.
 * @param free_space
 *   if non-NULL, returns the amount of space in the ring after the
 *   enqueue operation has finished.
 * @return
 *   - n: Actual number of objects enqueued.
 */
unsigned int
rte_ring_sp_enqueue_burst_elem(void *r, const void *obj_table,
    rte_ring_size_t n, rte_ring_size_t *free_space)
{
  return __rte_ring_do_enqueue_elem((struct rte_ring *)r, obj_table, n,
      RTE_RING_QUEUE_VARIABLE, RTE_RING_SYNC_ST, free_space);
}

/**
 * Enqueue several objects on a ring.
 *
 * This function calls the multi-producer or the single-producer
 * version depending on the default behavior that was specified at
 * ring creation time (see flags).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of objects.
 * @param esize
 *   The size of ring element, in bytes. It must be a multiple of 4.
 *   This must be the same value used while creating the ring. Otherwise
 *   the results are undefined.
 * @param n
 *   The number of objects to add in the ring from the obj_table.
 * @param free_space
 *   if non-NULL, returns the amount of space in the ring after the
 *   enqueue operation has finished.
 * @return
 *   - n: Actual number of objects enqueued.
 */
unsigned int
rte_ring_enqueue_burst_elem(struct rte_ring *r, const void *obj_table,
    rte_ring_size_t n, rte_ring_size_t *free_space)
{
  switch (r->prod_sync_type) {
  case RTE_RING_SYNC_MT:
    return rte_ring_mp_enqueue_burst_elem(r, obj_table, n,
      free_space);
  case RTE_RING_SYNC_ST:
    return rte_ring_sp_enqueue_burst_elem(r, obj_table, n,
      free_space);
  }

  /* valid ring should never reach this point */
  RTE_ASSERT(0);
  if (free_space != NULL)
    *free_space = 0;
  return 0;
}

/**
 * Dequeue several objects from a ring (multi-consumers safe). When the request
 * objects are more than the available objects, only dequeue the actual number
 * of objects
 *
 * This function uses a "compare and set" instruction to move the
 * consumer index atomically.
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of objects that will be filled.
 * @param esize
 *   The size of ring element, in bytes. It must be a multiple of 4.
 *   This must be the same value used while creating the ring. Otherwise
 *   the results are undefined.
 * @param n
 *   The number of objects to dequeue from the ring to the obj_table.
 * @param available
 *   If non-NULL, returns the number of remaining ring entries after the
 *   dequeue has finished.
 * @return
 *   - n: Actual number of objects dequeued, 0 if ring is empty
 */
unsigned int
rte_ring_mc_dequeue_burst_elem(struct rte_ring *r, void *obj_table,
    rte_ring_size_t n, rte_ring_size_t *available)
{
  return __rte_ring_do_dequeue_elem(r, obj_table, n,
      RTE_RING_QUEUE_VARIABLE, RTE_RING_SYNC_MT, available);
}

/**
 * Dequeue several objects from a ring (NOT multi-consumers safe).When the
 * request objects are more than the available objects, only dequeue the
 * actual number of objects
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of objects that will be filled.
 * @param esize
 *   The size of ring element, in bytes. It must be a multiple of 4.
 *   This must be the same value used while creating the ring. Otherwise
 *   the results are undefined.
 * @param n
 *   The number of objects to dequeue from the ring to the obj_table.
 * @param available
 *   If non-NULL, returns the number of remaining ring entries after the
 *   dequeue has finished.
 * @return
 *   - n: Actual number of objects dequeued, 0 if ring is empty
 */
unsigned int
rte_ring_sc_dequeue_burst_elem(void *r, void *obj_table,
    rte_ring_size_t n, rte_ring_size_t *available)
{
  return __rte_ring_do_dequeue_elem((struct rte_ring *)r, obj_table, n,
      RTE_RING_QUEUE_VARIABLE, RTE_RING_SYNC_ST, available);
}

/*
 * helper function, calculates sync_type values for prod and cons
 * based on input flags. Returns zero at success or negative
 * errno value otherwise.
 */
static int
get_sync_type(struct rte_ring *r, uint8_t flags)
{
  static const uint8_t prod_st_flags = RING_F_SP_ENQ;
  static const uint8_t cons_st_flags = RING_F_SC_DEQ;

  switch (flags & prod_st_flags) {
  case 0:
    r->prod_sync_type = RTE_RING_SYNC_MT;
    break;
  case RING_F_SP_ENQ:
    r->prod_sync_type = RTE_RING_SYNC_ST;
    break;
  default:
    return -NGRTOS_EINVAL;
  }

  switch (flags & cons_st_flags) {
  case 0:
    r->cons_sync_type = RTE_RING_SYNC_MT;
    break;
  case RING_F_SC_DEQ:
    r->cons_sync_type = RTE_RING_SYNC_ST;
    break;
  default:
    return -NGRTOS_EINVAL;
  }

  return 0;
}

void *
rte_ring_init(uint8_t *buf, rte_ring_size_t count, uint8_t flags)
{
  ngrtos_size_t sz;
  struct rte_ring *r;
        
/* true if x is a power of 2 */
  if (!__IS_POWEROF2(count) 
    || count > RTE_RING_SZ_MASK 
    || count < RTE_CACHE_LINE_SIZE) 
  {
    RTE_ERR(RING,
      "Requested size is invalid, must be power of 2, "
      "and not exceed the size limit %u\n",
      RTE_RING_SZ_MASK);
    goto err0;
  }

#if !__config_UseRingMinSize
  sz = RTE_ALIGN(sizeof(*r), RTE_CACHE_LINE_SIZE, ngrtos_size_t);
#else
  sz = sizeof(*r);
#endif
  r = (struct rte_ring *)ngrtos_malloc(sz);
  if (r == NULL)
  {
    RTE_ERR(RING,
      "Failed to allocate memory for rte_ring/%u\n",
      sizeof(*r));
    goto err0;
  }
  
  /* init the ring structure */
  ng_memset(r, 0, sizeof(*r));
  r->flags = flags;

  if (get_sync_type(r, flags))
    goto err1;

  r->size = count;
  r->mask = count - 1;

  if (buf == NULL)
  {
#if !__config_UseRingMinSize
    sz = RTE_ALIGN(count, RTE_CACHE_LINE_SIZE, ngrtos_size_t);
#endif
    buf = (uint8_t *)ngrtos_malloc(sz);
    if (buf == NULL)
    {
      RTE_ERR(RING,
              "Failed to allocate memory for buffer/%u\n",
              count);
      goto err1;
    }
    
  }
  r->buf = buf;
  return r;
  
err1:
  ngrtos_free(r);
err0:  
  return NULL;
}
