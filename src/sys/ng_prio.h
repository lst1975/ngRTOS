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

#ifndef __NGRTOS_PRIORITY_H__
#define __NGRTOS_PRIORITY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ng_list.h"

struct __priority_list {
  ng_list_s head;
  ng_list_s link;
  ngAtomTypeT priority;
};
typedef struct __priority_list __priority_list_t;

struct ng_priority_list{
  __priority_list_t list[ngRTOS_PRIO_MAX];
  ng_list_s all;
  ngAtomTypeT count;
};
typedef struct ng_priority_list ng_priority_list_t;

#define TH_PRI_CMP_GE(t, e) (t) <= (e)
#define TH_PRI_CMP_GT(t, e) (t) < (e)
#define __task_pri_is_TgtE(t,e,m) TH_PRI_CMP_GT((e)->m, (t)->m)
#define __task_pri_is_TgeE(t,e,m) TH_PRI_CMP_GE((e)->m, (t)->m)

#if !configENABLE_SMALL_KERNEL
#define __DECE(a,i) \
  [i] = { \
   .head      = LIST_HEAD_INIT((a).list[i].head), \
   .link      = LIST_ENTRY_INIT(), \
   .priority  = i \
  }

#if ngRTOS_PRIO_MAX == 8
#define NG_PRIORITY_LIST_DECL(a) { \
    .list  = { __DECE(a,0), __DECE(a,1), __DECE(a,2), __DECE(a,3), \
               __DECE(a,4), __DECE(a,5), __DECE(a,6), __DECE(a,7) }, \
    .all   = LIST_HEAD_INIT((a).all), \
    .count = 0, \
  }
#elif ngRTOS_PRIO_MAX == 7 
#define NG_PRIORITY_LIST_DECL(a) { \
    .list  = { __DECE(a,0), __DECE(a,1), __DECE(a,2), __DECE(a,3), \
               __DECE(a,4), __DECE(a,5), __DECE(a,6) }, \
    .all   = LIST_HEAD_INIT((a).all), \
    .count = 0, \
  }
#elif ngRTOS_PRIO_MAX == 6 
#define NG_PRIORITY_LIST_DECL(a) { \
    .list  = { __DECE(a,0), __DECE(a,1), __DECE(a,2), __DECE(a,3), \
               __DECE(a,4), __DECE(a,5) }, \
    .all   = LIST_HEAD_INIT((a).all), \
    .count = 0, \
  }
#elif ngRTOS_PRIO_MAX == 5 
#define NG_PRIORITY_LIST_DECL(a) { \
    .list  = { __DECE(a,0), __DECE(a,1), __DECE(a,2), __DECE(a,3), \
               __DECE(a,4) }, \
    .all   = LIST_HEAD_INIT((a).all), \
    .count = 0, \
  }
#elif ngRTOS_PRIO_MAX == 4 
#define NG_PRIORITY_LIST_DECL(a) { \
    .list  = { __DECE(a,0), __DECE(a,1), __DECE(a,2), __DECE(a,3) }, \
    .all   = LIST_HEAD_INIT((a).all), \
    .count = 0, \
  }
#elif ngRTOS_PRIO_MAX == 3 
#define NG_PRIORITY_LIST_DECL(a) { \
    .list  = { __DECE(a,0), __DECE(a,1), __DECE(a,2) }, \
    .all   = LIST_HEAD_INIT((a).all), \
    .count = 0, \
  }
#elif ngRTOS_PRIO_MAX == 2 
#define NG_PRIORITY_LIST_DECL(a) { \
    .list  = { __DECE(a,0), __DECE(a,1) }, \
    .all   = LIST_HEAD_INIT((a).all), \
    .count = 0, \
  }
#elif ngRTOS_PRIO_MAX == 1 
#define NG_PRIORITY_LIST_DECL(a) { \
    .list  = { __DECE(a,0) }, \
    .all   = LIST_HEAD_INIT((a).all), \
    .count = 0, \
  }
#endif
#endif

static inline void
ng_pri_init(ng_priority_list_t *list)
{
  for (uint8_t i = 0; i < ngRTOS_PRIO_MAX; i++)
  {
    __priority_list_t *p = &list->list[i];
    INIT_LIST_HEAD(&p->head);
    __list_poison_entry(&p->link);
    p->priority = i;
  }
  INIT_LIST_HEAD(&list->all);
  list->count = 0;
}
      
static inline void
ng_pri_enque(ng_priority_list_t *list, 
  ng_list_s *e, uint8_t pri)
{
  __priority_list_t *p;

  p = &list->list[pri&ngRTOS_PRIO_MASK];
  if (list_empty(&p->head))
  {
    list_cmp_add_before(p,  
              __priority_list_t, 
              &list->all, 
              link, 
              __task_pri_is_TgtE,
              priority);
  }
  
  list_add_tail(e, &p->head);
  list->count++;
}

static inline void 
__ng_pri_deque(ng_priority_list_t *list, 
  __priority_list_t *p, ng_list_s *e)
{
  list_del(e);
  list->count--;
  if (list_empty(&p->head))
    list_del(&p->link);
}

static inline void 
ng_pri_deque(ng_priority_list_t *list, 
  ng_list_s *e, uint8_t pri)
{
  __priority_list_t *p;
  NGRTOS_ASSERT(!list_empty(&list->all));
  p = &list->list[pri&ngRTOS_PRIO_MASK];
  __ng_pri_deque(list, p, e);
}

static inline ng_list_s * 
ng_pri_get(ng_priority_list_t *list)
{
  ng_list_s *e;
  __priority_list_t *p;

  if (list_empty(&list->all))
    return NULL;

  p = list_first_entry(&list->all,__priority_list_t,link);
  e = list_first(&p->head);
  __ng_pri_deque(list, p, e);
  
  return e;
}

static inline ngAtomTypeT
ng_pri_first(ng_priority_list_t *list)
{
  __priority_list_t *p;
  p = list_first_entry(&list->all,__priority_list_t,link);
  return p->priority;
}

static inline ng_bool_t
ng_pri_empty(ng_priority_list_t *list)
{
  return !list->count;
}

#endif /* _RTE_TIMER_H_ */
