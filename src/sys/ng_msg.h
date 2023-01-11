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

#ifndef __ngRTOS_MSG_H__
#define __ngRTOS_MSG_H__

#include "ng_list.h"
#include "ng_semphr.h"

typedef struct ng_msg ng_msg_s;
typedef struct ng_msg_list ng_msg_list_s;

struct ng_msg{
  ng_list_s link;
  uint32_t type:8;
  uint32_t id:19;
  uint32_t reserved:1;
  uint32_t ret:4;
};

#define NG_MSG_RET_DATA_READY 0
#define NG_MSG_RET_FAILED     1
#define NG_MSG_RET_CLOSED     2

struct ng_msg_list{
  ng_list_s   head;
  ng_list_s   todo;
  ng_sem_s    semphr;
  ngAtomTypeT count;
};

static inline void
ng_msg_list_add_one(ng_msg_list_s *h, ng_msg_s *m)
{
  ngIrqTypeT b = __ng_sys_disable_irq();
  list_add_tail(&m->link, &h->head);
  h->count++;
  __ng_sys_enable_irq(b);

  ngrtos_sem_wake(&h->semphr);
}

static inline void
ng_msg_list_add_more(ng_msg_list_s *h, 
  ng_list_s *l, u_long n, u_long size)
{
  ngIrqTypeT b = __ng_sys_disable_irq();
  list_splice_tail(l, &h->head);
  h->count += n;
  __ng_sys_enable_irq(b);

  ngrtos_sem_wake(&h->semphr);
}

static ng_msg_s *
ng_msg_list_get(ng_msg_list_s *h, int ms)
{
  while (ngrtos_TRUE)
  {
    ng_msg_s *m;
    ngIrqTypeT b;

    if (!list_empty(&h->todo))
    {
      m = list_first_entry(&h->todo,ng_msg_s,link);
      return m;
    }

    b = __ng_sys_disable_irq();
    list_splice_init(&h->head, &h->todo);
    h->count = 0;
    __ng_sys_enable_irq(b);
    
    if (list_empty(&h->todo))
    {
      ngrtos_sem_wait(&h->semphr, ms);
    }
  }
}

static ng_result_e
ng_msg_list_init(ng_msg_list_s *h, const ng_string_s *name)
{
  INIT_LIST_HEAD(&h->head);
  INIT_LIST_HEAD(&h->todo);
  h->count = 0;
  return ngrtos_sem_init(&h->semphr, name);
}

#endif /* __LINUX_SEMAPHORE_H */
