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
 * Copyright (c) 2010-2020 Intel Corporation
 * Copyright (c) 2007-2009 Kip Macy kmacy@freebsd.org
 * All rights reserved.
 * Derived from FreeBSD's bufring.h
 * Used as BSD-3 Licensed with permission from Kip Macy.
 */

#ifndef __ngRTOS_RING_H__
#define __ngRTOS_RING_H__

typedef uint32_t rte_ring_size_t;

#define __config_UseRingMinSize 0
#define unlikely(x) (x)
#define likely(x) (x)

/** prod/cons sync types */
enum rte_ring_sync_type {
	RTE_RING_SYNC_MT,     /**< multi-thread safe (default mode) */
	RTE_RING_SYNC_ST,     /**< single thread only */
};

unsigned int rte_ring_sp_enqueue_elem(void *r, void *obj, rte_ring_size_t n);
unsigned int rte_ring_sp_enqueue_burst_elem(void *r, const void *obj_table, 
  rte_ring_size_t n, rte_ring_size_t *free_space);
unsigned int rte_ring_mp_enqueue_burst_elem(void *r, const void *obj_table,
    rte_ring_size_t n, rte_ring_size_t *free_space);

unsigned int rte_ring_sc_dequeue_elem(void *r, void *obj_p, rte_ring_size_t n);
unsigned int rte_ring_sc_dequeue_burst_elem(void *r, void *obj_table, 
  rte_ring_size_t n, rte_ring_size_t *available);


void *rte_ring_init(uint8_t *buf, rte_ring_size_t count, uint8_t flags);

#endif
