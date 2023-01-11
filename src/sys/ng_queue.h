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

/*  $NetBSD: queue.h,v 1.18 1999/01/29 01:05:03 tv Exp $  */

/*
 * Copyright (c) 1991, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *  @(#)queue.h  8.5 (Berkeley) 8/20/94
 */

#ifndef  __ngRTOS_QUEUE_H__
#define __ngRTOS_QUEUE_H__

/*
 * This file defines four types of data structures: lists, simple queues,
 * tail queues, and circular queues.
 *
 * A list is headed by a single forward pointer (or an array of forward
 * pointers for a hash table header). The elements are doubly linked
 * so that an arbitrary element can be removed without a need to
 * traverse the list. New elements can be added to the list before
 * or after an existing element or at the head of the list. A list
 * may only be traversed in the forward direction.
 *
 * A simple queue is headed by a pair of pointers, one the head of the
 * list and the other to the tail of the list. The elements are singly
 * linked to save space, so only elements can only be removed from the
 * head of the list. New elements can be added to the list after
 * an existing element, at the head of the list, or at the end of the
 * list. A simple queue may only be traversed in the forward direction.
 *
 * A tail queue is headed by a pair of pointers, one to the head of the
 * list and the other to the tail of the list. The elements are doubly
 * linked so that an arbitrary element can be removed without a need to
 * traverse the list. New elements can be added to the list before or
 * after an existing element, at the head of the list, or at the end of
 * the list. A tail queue may only be traversed in the forward direction.
 *
 * A circle queue is headed by a pair of pointers, one to the head of the
 * list and the other to the tail of the list. The elements are doubly
 * linked so that an arbitrary element can be removed without a need to
 * traverse the list. New elements can be added to the list before or after
 * an existing element, at the head of the list, or at the end of the list.
 * A circle queue may be traversed in either direction, but has a more
 * complex end of list detection.
 *
 * For details on the use of these macros, see the queue(3) manual page.
 */

#if defined(QUEUE_MACRO_DEBUG)
#define _Q_INVALID ((void *)-1)
#define _Q_INVALIDATE(a) (a) = _Q_INVALID
#else
#define _Q_INVALIDATE(a)
#endif


/*
 * This file defines five types of data structures: singly-linked lists,
 * lists, simple queues, tail queues and XOR simple queues.
 *
 *
 * A singly-linked list is headed by a single forward pointer. The elements
 * are singly linked for minimum space and pointer manipulation overhead at
 * the expense of O(n) removal for arbitrary elements. New elements can be
 * added to the list after an existing element or at the head of the list.
 * Elements being removed from the head of the list should use the explicit
 * macro for this purpose for optimum efficiency. A singly-linked list may
 * only be traversed in the forward direction.  Singly-linked lists are ideal
 * for applications with large datasets and few or no removals or for
 * implementing a LIFO queue.
 *
 * A list is headed by a single forward pointer (or an array of forward
 * pointers for a hash table header). The elements are doubly linked
 * so that an arbitrary element can be removed without a need to
 * traverse the list. New elements can be added to the list before
 * or after an existing element or at the head of the list. A list
 * may only be traversed in the forward direction.
 *
 * A simple queue is headed by a pair of pointers, one to the head of the
 * list and the other to the tail of the list. The elements are singly
 * linked to save space, so elements can only be removed from the
 * head of the list. New elements can be added to the list before or after
 * an existing element, at the head of the list, or at the end of the
 * list. A simple queue may only be traversed in the forward direction.
 *
 * A tail queue is headed by a pair of pointers, one to the head of the
 * list and the other to the tail of the list. The elements are doubly
 * linked so that an arbitrary element can be removed without a need to
 * traverse the list. New elements can be added to the list before or
 * after an existing element, at the head of the list, or at the end of
 * the list. A tail queue may be traversed in either direction.
 *
 * An XOR simple queue is used in the same way as a regular simple queue.
 * The difference is that the head structure also includes a "cookie" that
 * is XOR'd with the queue pointer (first, last or next) to generate the
 * real pointer value.
 *
 * For details on the use of these macros, see the queue(3) manual page.
 */

#if defined(QUEUE_MACRO_DEBUG)
#define _Q_INVALID ((void *)-1)
#define _Q_INVALIDATE(a) (a) = _Q_INVALID
#else
#define _Q_INVALIDATE(a)
#endif

/*
 * Singly-linked List definitions.
 */
#define bsd_SLIST_HEAD(name, type)            \
struct name {                \
  struct type *slh_first;  /* first element */      \
}

#define bsd_SLIST_HEAD_INITIALIZER(head)          \
  { NULL }

#define bsd_SLIST_ENTRY(type)            \
struct {                \
  struct type *sle_next;  /* next element */      \
}

/*
 * Singly-linked List access methods.
 */
#define bsd_SLIST_FIRST(head)  ((head)->slh_first)
#define bsd_SLIST_END(head)    NULL
#define bsd_SLIST_EMPTY(head)  (bsd_SLIST_FIRST(head) == bsd_SLIST_END(head))
#define bsd_SLIST_NEXT(elm, field)  ((elm)->field.sle_next)

#define bsd_SLIST_FOREACH(var, head, field)          \
  for((var) = bsd_SLIST_FIRST(head);          \
      (var) != bsd_SLIST_END(head);          \
      (var) = bsd_SLIST_NEXT(var, field))

#define bsd_SLIST_FOREACH_SAFE(var, head, field, tvar)      \
  for ((var) = bsd_SLIST_FIRST(head);        \
      (var) && ((tvar) = bsd_SLIST_NEXT(var, field), 1);    \
      (var) = (tvar))

/*
 * Singly-linked List functions.
 */
#define bsd_SLIST_INIT(head) {            \
  bsd_SLIST_FIRST(head) = bsd_SLIST_END(head);        \
}

#define bsd_SLIST_INSERT_AFTER(slistelm, elm, field) do {      \
  (elm)->field.sle_next = (slistelm)->field.sle_next;    \
  (slistelm)->field.sle_next = (elm);        \
} while (0)

#define bsd_SLIST_INSERT_HEAD(head, elm, field) do {      \
  (elm)->field.sle_next = (head)->slh_first;      \
  (head)->slh_first = (elm);          \
} while (0)

#define bsd_SLIST_REMOVE_AFTER(elm, field) do {        \
  (elm)->field.sle_next = (elm)->field.sle_next->field.sle_next;  \
} while (0)

#define bsd_SLIST_REMOVE_HEAD(head, field) do {        \
  (head)->slh_first = (head)->slh_first->field.sle_next;    \
} while (0)

#define bsd_SLIST_REMOVE(head, elm, type, field) do {      \
  if ((head)->slh_first == (elm)) {        \
    bsd_SLIST_REMOVE_HEAD((head), field);      \
  } else {              \
    struct type *curelm = (head)->slh_first;    \
                  \
    while (curelm->field.sle_next != (elm))      \
      curelm = curelm->field.sle_next;    \
    curelm->field.sle_next =        \
        curelm->field.sle_next->field.sle_next;    \
  }                \
  _Q_INVALIDATE((elm)->field.sle_next);        \
} while (0)

/*
 * List definitions.
 */
#define bsd_LIST_HEAD(name, type)            \
struct name {                \
  struct type *lh_first;  /* first element */      \
}

#define bsd_LIST_HEAD_INITIALIZER(head)          \
  { NULL }

#define bsd_LIST_ENTRY(type)            \
struct {                \
  struct type *le_next;  /* next element */      \
  struct type **le_prev;  /* address of previous next element */  \
}

/*
 * List access methods.
 */
#define bsd_LIST_FIRST(head)    ((head)->lh_first)
#define bsd_LIST_END(head)      NULL
#define bsd_LIST_EMPTY(head)    (bsd_LIST_FIRST(head) == bsd_LIST_END(head))
#define bsd_LIST_NEXT(elm, field)    ((elm)->field.le_next)

#define bsd_LIST_FOREACH(var, head, field)          \
  for((var) = bsd_LIST_FIRST(head);          \
      (var)!= bsd_LIST_END(head);          \
      (var) = bsd_LIST_NEXT(var, field))

#define LIST_FOREACH_SAFE(var, head, field, tvar)      \
  for ((var) = bsd_LIST_FIRST(head);        \
      (var) && ((tvar) = bsd_LIST_NEXT(var, field), 1);    \
      (var) = (tvar))

/*
 * List functions.
 */
#define bsd_LIST_INIT(head) do {            \
  bsd_LIST_FIRST(head) = bsd_LIST_END(head);        \
} while (0)

#define bsd_LIST_INSERT_AFTER(listelm, elm, field) do {      \
  if (((elm)->field.le_next = (listelm)->field.le_next) != NULL)  \
    (listelm)->field.le_next->field.le_prev =    \
        &(elm)->field.le_next;        \
  (listelm)->field.le_next = (elm);        \
  (elm)->field.le_prev = &(listelm)->field.le_next;    \
} while (0)

#define bsd_LIST_INSERT_BEFORE(listelm, elm, field) do {      \
  (elm)->field.le_prev = (listelm)->field.le_prev;    \
  (elm)->field.le_next = (listelm);        \
  *(listelm)->field.le_prev = (elm);        \
  (listelm)->field.le_prev = &(elm)->field.le_next;    \
} while (0)

#define bsd_LIST_INSERT_HEAD(head, elm, field) do {        \
  if (((elm)->field.le_next = (head)->lh_first) != NULL)    \
    (head)->lh_first->field.le_prev = &(elm)->field.le_next;\
  (head)->lh_first = (elm);          \
  (elm)->field.le_prev = &(head)->lh_first;      \
} while (0)

#define bsd_LIST_REMOVE(elm, field) do {          \
  if ((elm)->field.le_next != NULL)        \
    (elm)->field.le_next->field.le_prev =      \
        (elm)->field.le_prev;        \
  *(elm)->field.le_prev = (elm)->field.le_next;      \
  _Q_INVALIDATE((elm)->field.le_prev);        \
  _Q_INVALIDATE((elm)->field.le_next);        \
} while (0)

#define bsd_LIST_REPLACE(elm, elm2, field) do {        \
  if (((elm2)->field.le_next = (elm)->field.le_next) != NULL)  \
    (elm2)->field.le_next->field.le_prev =      \
        &(elm2)->field.le_next;        \
  (elm2)->field.le_prev = (elm)->field.le_prev;      \
  *(elm2)->field.le_prev = (elm2);        \
  _Q_INVALIDATE((elm)->field.le_prev);        \
  _Q_INVALIDATE((elm)->field.le_next);        \
} while (0)

/*
 * Simple queue definitions.
 */
#define SIMPLEQ_HEAD(name, type)          \
struct name {                \
  struct type *sqh_first;  /* first element */      \
  struct type **sqh_last;  /* addr of last next element */    \
}

#define SIMPLEQ_HEAD_INITIALIZER(head)          \
  { NULL, &(head).sqh_first }

#define SIMPLEQ_ENTRY(type)            \
struct {                \
  struct type *sqe_next;  /* next element */      \
}

/*
 * Simple queue access methods.
 */
#define SIMPLEQ_FIRST(head)      ((head)->sqh_first)
#define SIMPLEQ_END(head)         NULL
#define SIMPLEQ_EMPTY(head)      (SIMPLEQ_FIRST(head) == SIMPLEQ_END(head))
#define SIMPLEQ_NEXT(elm, field) ((elm)->field.sqe_next)

#define SIMPLEQ_FOREACH(var, head, field)        \
  for((var) = SIMPLEQ_FIRST(head);        \
      (var) != SIMPLEQ_END(head);          \
      (var) = SIMPLEQ_NEXT(var, field))

#define SIMPLEQ_FOREACH_SAFE(var, head, field, tvar)      \
  for ((var) = SIMPLEQ_FIRST(head);        \
      (var) && ((tvar) = SIMPLEQ_NEXT(var, field), 1);    \
      (var) = (tvar))

/*
 * Simple queue functions.
 */
#define SIMPLEQ_INIT(head) do {            \
  (head)->sqh_first = NULL;          \
  (head)->sqh_last = &(head)->sqh_first;        \
} while (0)

#define SIMPLEQ_INSERT_HEAD(head, elm, field) do {      \
  if (((elm)->field.sqe_next = (head)->sqh_first) == NULL)  \
    (head)->sqh_last = &(elm)->field.sqe_next;    \
  (head)->sqh_first = (elm);          \
} while (0)

#define SIMPLEQ_INSERT_TAIL(head, elm, field) do {      \
  (elm)->field.sqe_next = NULL;          \
  *(head)->sqh_last = (elm);          \
  (head)->sqh_last = &(elm)->field.sqe_next;      \
} while (0)

#define SIMPLEQ_INSERT_AFTER(head, listelm, elm, field) do {    \
  if (((elm)->field.sqe_next = (listelm)->field.sqe_next) == NULL)\
    (head)->sqh_last = &(elm)->field.sqe_next;    \
  (listelm)->field.sqe_next = (elm);        \
} while (0)

#define SIMPLEQ_REMOVE_HEAD(head, field) do {      \
  if (((head)->sqh_first = (head)->sqh_first->field.sqe_next) == NULL) \
    (head)->sqh_last = &(head)->sqh_first;      \
} while (0)

#define SIMPLEQ_REMOVE_AFTER(head, elm, field) do {      \
  if (((elm)->field.sqe_next = (elm)->field.sqe_next->field.sqe_next) \
      == NULL)              \
    (head)->sqh_last = &(elm)->field.sqe_next;    \
} while (0)

#define SIMPLEQ_CONCAT(head1, head2) do {        \
  if (!SIMPLEQ_EMPTY((head2))) {          \
    *(head1)->sqh_last = (head2)->sqh_first;    \
    (head1)->sqh_last = (head2)->sqh_last;      \
    SIMPLEQ_INIT((head2));          \
  }                \
} while (0)

/*
 * XOR Simple queue definitions.
 */
#define XSIMPLEQ_HEAD(name, type)          \
struct name {                \
  struct type *sqx_first;  /* first element */      \
  struct type **sqx_last;  /* addr of last next element */    \
  unsigned long sqx_cookie;          \
}

#define XSIMPLEQ_ENTRY(type)            \
struct {                \
  struct type *sqx_next;  /* next element */      \
}

/*
 * XOR Simple queue access methods.
 */
#define XSIMPLEQ_XOR(head, ptr)      ((__typeof(ptr))((head)->sqx_cookie ^ \
          (unsigned long)(ptr)))
#define XSIMPLEQ_FIRST(head)      XSIMPLEQ_XOR(head, ((head)->sqx_first))
#define XSIMPLEQ_END(head)      NULL
#define XSIMPLEQ_EMPTY(head)      (XSIMPLEQ_FIRST(head) == XSIMPLEQ_END(head))
#define XSIMPLEQ_NEXT(head, elm, field)    XSIMPLEQ_XOR(head, ((elm)->field.sqx_next))


#define XSIMPLEQ_FOREACH(var, head, field)        \
  for ((var) = XSIMPLEQ_FIRST(head);        \
      (var) != XSIMPLEQ_END(head);        \
      (var) = XSIMPLEQ_NEXT(head, var, field))

#define XSIMPLEQ_FOREACH_SAFE(var, head, field, tvar)      \
  for ((var) = XSIMPLEQ_FIRST(head);        \
      (var) && ((tvar) = XSIMPLEQ_NEXT(head, var, field), 1);  \
      (var) = (tvar))

/*
 * XOR Simple queue functions.
 */
#define XSIMPLEQ_INIT(head) do {          \
  arc4random_buf(&(head)->sqx_cookie, sizeof((head)->sqx_cookie)); \
  (head)->sqx_first = XSIMPLEQ_XOR(head, NULL);      \
  (head)->sqx_last = XSIMPLEQ_XOR(head, &(head)->sqx_first);  \
} while (0)

#define XSIMPLEQ_INSERT_HEAD(head, elm, field) do {      \
  if (((elm)->field.sqx_next = (head)->sqx_first) ==    \
      XSIMPLEQ_XOR(head, NULL))          \
    (head)->sqx_last = XSIMPLEQ_XOR(head, &(elm)->field.sqx_next); \
  (head)->sqx_first = XSIMPLEQ_XOR(head, (elm));      \
} while (0)

#define XSIMPLEQ_INSERT_TAIL(head, elm, field) do {      \
  (elm)->field.sqx_next = XSIMPLEQ_XOR(head, NULL);    \
  *(XSIMPLEQ_XOR(head, (head)->sqx_last)) = XSIMPLEQ_XOR(head, (elm)); \
  (head)->sqx_last = XSIMPLEQ_XOR(head, &(elm)->field.sqx_next);  \
} while (0)

#define XSIMPLEQ_INSERT_AFTER(head, listelm, elm, field) do {    \
  if (((elm)->field.sqx_next = (listelm)->field.sqx_next) ==  \
      XSIMPLEQ_XOR(head, NULL))          \
    (head)->sqx_last = XSIMPLEQ_XOR(head, &(elm)->field.sqx_next); \
  (listelm)->field.sqx_next = XSIMPLEQ_XOR(head, (elm));    \
} while (0)

#define XSIMPLEQ_REMOVE_HEAD(head, field) do {        \
  if (((head)->sqx_first = XSIMPLEQ_XOR(head,      \
      (head)->sqx_first)->field.sqx_next) == XSIMPLEQ_XOR(head, NULL)) \
    (head)->sqx_last = XSIMPLEQ_XOR(head, &(head)->sqx_first); \
} while (0)

#define XSIMPLEQ_REMOVE_AFTER(head, elm, field) do {      \
  if (((elm)->field.sqx_next = XSIMPLEQ_XOR(head,      \
      (elm)->field.sqx_next)->field.sqx_next)      \
      == XSIMPLEQ_XOR(head, NULL))        \
    (head)->sqx_last =           \
        XSIMPLEQ_XOR(head, &(elm)->field.sqx_next);    \
} while (0)


/*
 * Tail queue definitions.
 */
#define TAILQ_HEAD(name, type)            \
struct name {                \
  struct type *tqh_first;  /* first element */      \
  struct type **tqh_last;  /* addr of last next element */    \
}

#define TAILQ_HEAD_INITIALIZER(head)          \
  { NULL, &(head).tqh_first }

#define TAILQ_ENTRY(type)            \
struct {                \
  struct type *tqe_next;  /* next element */      \
  struct type **tqe_prev;  /* address of previous next element */  \
}

/*
 * Tail queue access methods.
 */
#define TAILQ_FIRST(head)    ((head)->tqh_first)
#define TAILQ_END(head)      NULL
#define TAILQ_NEXT(elm, field)    ((elm)->field.tqe_next)
#define TAILQ_LAST(head, headname)          \
  (*(((struct headname *)((head)->tqh_last))->tqh_last))
/* XXX */
#define TAILQ_PREV(elm, headname, field)        \
  (*(((struct headname *)((elm)->field.tqe_prev))->tqh_last))
#define TAILQ_EMPTY(head)            \
  (TAILQ_FIRST(head) == TAILQ_END(head))

#define TAILQ_FOREACH(var, head, field)          \
  for((var) = TAILQ_FIRST(head);          \
      (var) != TAILQ_END(head);          \
      (var) = TAILQ_NEXT(var, field))

#define TAILQ_FOREACH_SAFE(var, head, field, tvar)      \
  for ((var) = TAILQ_FIRST(head);          \
      (var) != TAILQ_END(head) &&          \
      ((tvar) = TAILQ_NEXT(var, field), 1);      \
      (var) = (tvar))


#define TAILQ_FOREACH_REVERSE(var, head, headname, field)    \
  for((var) = TAILQ_LAST(head, headname);        \
      (var) != TAILQ_END(head);          \
      (var) = TAILQ_PREV(var, headname, field))

#define TAILQ_FOREACH_REVERSE_SAFE(var, head, headname, field, tvar)  \
  for ((var) = TAILQ_LAST(head, headname);      \
      (var) != TAILQ_END(head) &&          \
      ((tvar) = TAILQ_PREV(var, headname, field), 1);    \
      (var) = (tvar))

/*
 * Tail queue functions.
 */
#define TAILQ_INIT(head) do {            \
  (head)->tqh_first = NULL;          \
  (head)->tqh_last = &(head)->tqh_first;        \
} while (0)

#define TAILQ_INSERT_HEAD(head, elm, field) do {      \
  if (((elm)->field.tqe_next = (head)->tqh_first) != NULL)  \
    (head)->tqh_first->field.tqe_prev =      \
        &(elm)->field.tqe_next;        \
  else                \
    (head)->tqh_last = &(elm)->field.tqe_next;    \
  (head)->tqh_first = (elm);          \
  (elm)->field.tqe_prev = &(head)->tqh_first;      \
} while (0)

#define TAILQ_INSERT_TAIL(head, elm, field) do {      \
  (elm)->field.tqe_next = NULL;          \
  (elm)->field.tqe_prev = (head)->tqh_last;      \
  *(head)->tqh_last = (elm);          \
  (head)->tqh_last = &(elm)->field.tqe_next;      \
} while (0)

#define TAILQ_INSERT_AFTER(head, listelm, elm, field) do {    \
  if (((elm)->field.tqe_next = (listelm)->field.tqe_next) != NULL)\
    (elm)->field.tqe_next->field.tqe_prev =      \
        &(elm)->field.tqe_next;        \
  else                \
    (head)->tqh_last = &(elm)->field.tqe_next;    \
  (listelm)->field.tqe_next = (elm);        \
  (elm)->field.tqe_prev = &(listelm)->field.tqe_next;    \
} while (0)

#define TAILQ_INSERT_BEFORE(listelm, elm, field) do {      \
  (elm)->field.tqe_prev = (listelm)->field.tqe_prev;    \
  (elm)->field.tqe_next = (listelm);        \
  *(listelm)->field.tqe_prev = (elm);        \
  (listelm)->field.tqe_prev = &(elm)->field.tqe_next;    \
} while (0)

#define TAILQ_REMOVE(head, elm, field) do {        \
  if (((elm)->field.tqe_next) != NULL)        \
    (elm)->field.tqe_next->field.tqe_prev =      \
        (elm)->field.tqe_prev;        \
  else                \
    (head)->tqh_last = (elm)->field.tqe_prev;    \
  *(elm)->field.tqe_prev = (elm)->field.tqe_next;      \
  _Q_INVALIDATE((elm)->field.tqe_prev);        \
  _Q_INVALIDATE((elm)->field.tqe_next);        \
} while (0)

#define TAILQ_REPLACE(head, elm, elm2, field) do {      \
  if (((elm2)->field.tqe_next = (elm)->field.tqe_next) != NULL)  \
    (elm2)->field.tqe_next->field.tqe_prev =    \
        &(elm2)->field.tqe_next;        \
  else                \
    (head)->tqh_last = &(elm2)->field.tqe_next;    \
  (elm2)->field.tqe_prev = (elm)->field.tqe_prev;      \
  *(elm2)->field.tqe_prev = (elm2);        \
  _Q_INVALIDATE((elm)->field.tqe_prev);        \
  _Q_INVALIDATE((elm)->field.tqe_next);        \
} while (0)

#define TAILQ_CONCAT(head1, head2, field) do {        \
  if (!TAILQ_EMPTY(head2)) {          \
    *(head1)->tqh_last = (head2)->tqh_first;    \
    (head2)->tqh_first->field.tqe_prev = (head1)->tqh_last;  \
    (head1)->tqh_last = (head2)->tqh_last;      \
    TAILQ_INIT((head2));          \
  }                \
} while (0)

/*
 * Singly-linked Tail queue declarations.
 */
#define STAILQ_HEAD(name, type)            \
struct name {                \
  struct type *stqh_first;  /* first element */    \
  struct type **stqh_last;  /* addr of last next element */  \
}

#define STAILQ_HEAD_INITIALIZER(head)          \
  { NULL, &(head).stqh_first }

#define STAILQ_ENTRY(type)            \
struct {                \
  struct type *stqe_next;  /* next element */      \
}

/*
 * Singly-linked Tail queue access methods.
 */
#define STAILQ_FIRST(head)  ((head)->stqh_first)
#define STAILQ_END(head)  NULL
#define STAILQ_EMPTY(head)  (STAILQ_FIRST(head) == STAILQ_END(head))
#define STAILQ_NEXT(elm, field)  ((elm)->field.stqe_next)

#define STAILQ_FOREACH(var, head, field)        \
  for ((var) = STAILQ_FIRST(head);        \
      (var) != STAILQ_END(head);          \
      (var) = STAILQ_NEXT(var, field))

#define STAILQ_FOREACH_SAFE(var, head, field, tvar)      \
  for ((var) = STAILQ_FIRST(head);        \
      (var) && ((tvar) = STAILQ_NEXT(var, field), 1);    \
      (var) = (tvar))

/*
 * Singly-linked Tail queue functions.
 */
#define STAILQ_INIT(head) do {            \
  STAILQ_FIRST((head)) = NULL;          \
  (head)->stqh_last = &STAILQ_FIRST((head));      \
} while (0)

#define STAILQ_INSERT_HEAD(head, elm, field) do {      \
  if ((STAILQ_NEXT((elm), field) = STAILQ_FIRST((head))) == NULL)  \
    (head)->stqh_last = &STAILQ_NEXT((elm), field);    \
  STAILQ_FIRST((head)) = (elm);          \
} while (0)

#define STAILQ_INSERT_TAIL(head, elm, field) do {      \
  STAILQ_NEXT((elm), field) = NULL;        \
  *(head)->stqh_last = (elm);          \
  (head)->stqh_last = &STAILQ_NEXT((elm), field);      \
} while (0)

#define STAILQ_INSERT_AFTER(head, listelm, elm, field) do {    \
  if ((STAILQ_NEXT((elm), field) = STAILQ_NEXT((elm), field)) == NULL)\
    (head)->stqh_last = &STAILQ_NEXT((elm), field);    \
  STAILQ_NEXT((elm), field) = (elm);        \
} while (0)

#define STAILQ_REMOVE_HEAD(head, field) do {                            \
  if ((STAILQ_FIRST((head)) =          \
      STAILQ_NEXT(STAILQ_FIRST((head)), field)) == NULL)    \
    (head)->stqh_last = &STAILQ_FIRST((head));    \
} while (0)

#define STAILQ_REMOVE_AFTER(head, elm, field) do {                      \
  if ((STAILQ_NEXT(elm, field) =          \
      STAILQ_NEXT(STAILQ_NEXT(elm, field), field)) == NULL)  \
    (head)->stqh_last = &STAILQ_NEXT((elm), field);    \
} while (0)

#define STAILQ_REMOVE(head, elm, type, field) do {      \
  if (STAILQ_FIRST((head)) == (elm)) {        \
    STAILQ_REMOVE_HEAD((head), field);      \
  } else {              \
    struct type *curelm = (head)->stqh_first;    \
    while (STAILQ_NEXT(curelm, field) != (elm))    \
      curelm = STAILQ_NEXT(curelm, field);    \
    STAILQ_REMOVE_AFTER(head, curelm, field);    \
  }                \
} while (0)

#define STAILQ_CONCAT(head1, head2) do {        \
  if (!STAILQ_EMPTY((head2))) {          \
    *(head1)->stqh_last = (head2)->stqh_first;    \
    (head1)->stqh_last = (head2)->stqh_last;    \
    STAILQ_INIT((head2));          \
  }                \
} while (0)

#define STAILQ_LAST(head, type, field)          \
  (STAILQ_EMPTY((head)) ?  NULL :          \
          ((struct type *)(void *)        \
    ((char *)((head)->stqh_last) - ngrtos_offsetof(struct type, field))))


#endif /* __ngRTOS_QUEUE_H__ */
