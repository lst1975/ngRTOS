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

#ifndef __ngRTOS_CRITICAL_H__
#define __ngRTOS_CRITICAL_H__

#ifdef __cplusplus
    extern "C" {
#endif

struct ng_critical{
  ngAtomTypeT nesting;
  ngIrqTypeT previrq;
  ngIrqTypeT basepri;
};
typedef struct ng_critical ng_critical_s;
#define NG_CRITICAL_DECL_INIT { \
  .nesting = 0, \
}

void ng_critical_section_init(ng_critical_s *c)
{
  c->nesting = 0;
}
/*-----------------------------------------------------------*/

void ng_critical_section_enter(ng_critical_s *c)
{
  ngIrqTypeT basepri = __ng_sys_disable_irq();
  ngIrqTypeT curirq = __ng_get_Irqn();
  
	/* Only assert if the critical nesting count is 1 to 
	   protect against recursive calls if the assert 
	   function also uses a critical section. 
	 */
	__ng_sys_check_recursive_critical();
  c->nesting++;
  if (c->nesting == 1)
  {
    c->previrq = curirq;
    c->basepri = basepri;
  }
  else
  {
    NGRTOS_ASSERT(c->previrq == curirq);
  }
}
/*-----------------------------------------------------------*/

void ng_critical_section_leave(ng_critical_s *c)
{
  ngIrqTypeT curirq = __ng_get_Irqn();

	__ng_sys_check_recursive_critical();
  NGRTOS_ASSERT(c->nesting);
  NGRTOS_ASSERT(c->previrq == curirq);

  c->nesting--;
  if (!c->nesting)
    __ng_sys_enable_irq(c->basepri);
}

#ifdef __cplusplus
    }
#endif

#endif /* __ngRTOS_CRITICAL_H__ */

