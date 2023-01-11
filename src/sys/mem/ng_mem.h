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

#ifndef __NGRTOS_MEM_H__
#define __NGRTOS_MEM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ng_defs.h"
#include "ng_string.h"
#include "ng_mcopy.h"

#define M_DONTWAIT 0
#define M_WAIT     1

ng_result_e ng_mem_init(void);
void ng_mem_destroy(void);

#define TLSF_USE_LOCKS 1
#include "tlsf/ng_tlsf.h"
#define vos_malloc(size) tlsf_malloc(size) 
#define vos_free(ptr) tlsf_free(ptr)
#define vos_realloc(ptr, size) tlsf_realloc(ptr, size)
#define vos_calloc(nelem, elem_size) tlsf_calloc(nelem, elem_size)

#if configEnable_MemPool
#include "peak/ng_vmem.h"
#define ng_malloc(size) peak_malloc(size) 
#define ng_free(ptr) peak_free(ptr)
#define ng_realloc(ptr, size) peak_realloc(ptr, size)
#define ng_calloc(nelem, elem_size) peak_calloc(nelem, elem_size)
#else
#define ng_malloc(size) vos_malloc(size) 
#define ng_free(ptr) vos_free(ptr)
#define ng_realloc(ptr, size) vos_realloc(ptr, size)
#define ng_calloc(nelem, elem_size) vos_calloc(nelem, elem_size)
#endif

static inline void *__ng_malloc(int wait, int size)
{
  void *m;

  do {
    m = ng_malloc(size);
  } while(wait && m == NULL);
  
  return m;
}

/**
 * strdupex - Copies a string into a newly created location.
 * @str: The source string
 *
 * returns the address of the new location
 */
static inline char *vos_strdup_len(const char *str, int *length)
{
  char * tmp;
  size_t len;

  if (str == NULL)
    return NULL;
  
  tmp = NULL;
  
  len = length == NULL || *length == 0 ?
    ng_strlen(str) : *length;
  
  if (len){
    tmp = (char*)vos_malloc(1+ len);
    if (tmp != NULL)
      ng_memcpy(tmp, str, len+1);
  }
  if (length != NULL)
    *length = len;
  return tmp;
}

static inline char *vos_strdup(const char *str)
{
  return vos_strdup_len(str, NULL);
}

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_CONFIG_H */
