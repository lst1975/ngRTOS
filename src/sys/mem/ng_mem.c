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

#include "ng_arch.h"
#include "ng_defs.h"
#include "ng_mem.h"

static uint8_t ng_heap_main[ngRTOS_TOTAL_HEAP_SIZE];

struct ng_heap
{
  uint8_t *ptr;
  size_t   size;
};

typedef struct ng_heap ng_heap_s;

static ng_heap_s ng_heap_regions[] = {
  { ng_heap_main, ngRTOS_TOTAL_HEAP_SIZE },
  { NULL,         0                     }
};

ng_result_e 
ng_mem_init(void)
{
  int i;
  ng_heap_s *h;
  ng_result_e ret;

  NGRTOS_EVENT("Starting memory system.\n");

  if (IS_IRQ()) 
  {
    ret = NG_result_eirq;
    goto err0;
  }

  h = ng_heap_regions;
  for (i = 0; h[i].size; i++)
  {
    ret = init_memory_pool(h->size, h->ptr);
    if (NG_result_ok != ret)
    {
      goto err1;
    }
  }

#if configEnable_MemPool
  ret = sys_mem_init();
  if (NG_result_ok != ret)
  {
    goto err1;
  }
#endif  
  NGRTOS_EVENT("Memory system init OK.\n");
  return NG_result_ok;

err1:
  while (i--)
  {
    destroy_memory_pool(h[i].ptr);
  }
err0:
  NGRTOS_ERROR("Failed to init memory system.\n");
  return ret;
}

void 
ng_mem_destroy(void)
{
  if (!IS_IRQ()) 
  {
    int i;
    ng_heap_s *h = ng_heap_regions;

#if configEnable_MemPool
    sys_mem_close(ngrtos_FALSE);
#endif  
    
    for (i = 0; h[i].size; i++)
    {
      destroy_memory_pool(h->ptr);
    }
  }
}
