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
#ifndef __ngRTOS_LWQ_H__
#define __ngRTOS_LWQ_H__

#ifdef __cplusplus
extern "C" {
#endif

struct ng_lwq {
	ng_list_s head;
	uint32_t  length:31;
	uint32_t  stop:1;
};
typedef struct ng_lwq ng_lwq_s;

void lwq_enqueue(ng_lfq_s *, ng_list_s *);
int lwq_dequeue(ng_lfq_s *, ng_list_s *);

__STATIC_INLINE void
lwq_init(ng_lwq_s *q)
{
  INIT_LIST_HEAD(&q->head);
  q->length = 0;
  q->stop   = ngrtos_FALSE;
}

__STATIC_INLINE void
lwq_stop(ng_lwq_s *q)
{
  ngIrqTypeT x;
  
  x = __ng_sys_disable_irq();
  q->stop   = ngrtos_TRUE;
  __ng_sys_enable_irq(x);
}

__STATIC_FORCEINLINE void
lwq_enqueue(ng_lwq_s *q, ng_list_s *l, int n)
{
  ngIrqTypeT x;
  
  x = __ng_sys_disable_irq();
  if (q->stop)
  {
    __ng_sys_enable_irq(x);
    return;
  }
  list_splice_tail(l, &q->head);
  q->length += n;
  __ng_sys_enable_irq(x);
}

__STATIC_FORCEINLINE void
lwq_dequeue(ng_lwq_s *q, ng_list_s *h)
{
  ngIrqTypeT x;

  x = __ng_sys_disable_irq();
  if (list_empty(&q->head))
  {
    __ng_sys_enable_irq(x);
    return;
  }
  list_splice_init(&q->head,h);
  q->length = 0;
  __ng_sys_enable_irq(x);
}

#ifdef __cplusplus
}
#endif

#endif
