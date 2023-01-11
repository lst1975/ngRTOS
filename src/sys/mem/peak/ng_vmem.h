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

/*
 * Copyright (c) 1999-2010.  Beijing China
 * All rights reserved to Songtao Liu
 * email: songtaoliu@sohu.com
 */
 
#ifndef __LIBX_VMEM_H__
#define __LIBX_VMEM_H__

#if configEnable_MemPool

#define __USE_KOS_DEBUG__ 1
#define MEM_FROM_SYSTEM  100
#define __inline inline
#define DO_NOTHING (void)(0)

#define __ALIGNBYTES (sizeof(void*))
#define __ROUNDUP(p, t, b)  (((t)(p) + ((b)-1)) & ~((b)-1))

#define M_FREE    0  /* should be on free list */
#define M_TEMP      -1

#define M_ZERO 0x01
#define M_NOWAIT 0x02

#ifndef KASSERT
#define KASSERT(x) NGRTOS_ASSERT(x)
#endif /* KASSERT */

#ifndef min
#define min(a,b) (((a)<(b)) ? (a) : (b))
#endif /* min */

#ifndef max
#define max(a,b) (((a)<(b)) ? (b) : (a))
#endif /* max */

ng_result_e __sys_mem_init(void );
void __sys_mem_close(ng_bool_t quiet);

#if __USE_KOS_DEBUG__
void *__sys_calloc(size_t , size_t , const char *, int );
void *__sys_malloc(size_t , int, int, const char *, int );
void __sys_free(void *, int, const char *, int );
void *__sys_realloc(void *, size_t, const char *, int);
char *__sys_strdup_len(const char * str, int *length
  , const char * file, int line);
char *__sys_strdup(const char * str, const char * file, int line);
#else
void *__sys_malloc(size_t , int, int);
void __sys_free(void *, int);
void *__sys_calloc(size_t , size_t );
void *__sys_realloc(void *, size_t);
char *__sys_strdup_len(const char * str, int *length);
char *__sys_strdup(const char * str);
#endif

#if __USE_KOS_DEBUG__
void mem_file_line_change(const char * file,
  int line, void * maddr);
void mem_file_line_get(const char * *file,
  int *line, void * maddr);
#else
#define mem_file_line_change(f, l, m) DO_NOTHING
#define mem_file_line_get(f, l, m) DO_NOTHING
#endif

#if __USE_KOS_DEBUG__
#define peak_malloc(s) __sys_malloc(s, M_TEMP, M_NOWAIT, __FILE__, __LINE__)
#define peak_free(p) __sys_free(p, M_TEMP, __FILE__, __LINE__)
#define peak_realloc(p, s)  __sys_realloc(p, s, __FILE__, __LINE__)
#define peak_calloc(n, s) __sys_calloc(n, s, __FILE__, __LINE__)
#define peak_strdup(s) __sys_strdup(s, __FILE__, __LINE__)
#define peak_strdup_len(s,l) __sys_strdup_len(s,l, __FILE__, __LINE__)
#else
#define peak_malloc(s) __sys_malloc(s, M_TEMP, M_NOWAIT)
#define peak_free(p) __sys_free(p, M_TEMP)
#define peak_realloc(p, s)  __sys_realloc(p, s)
#define peak_calloc(n, s) __sys_calloc(n, s)
#define peak_strdup(s) __sys_strdup(s)
#define peak_strdup_len(s,l) __sys_strdup_len(s,l)
#endif

#define sys_mem_init() __sys_mem_init()
#define sys_mem_close(q) __sys_mem_close(q)

#endif

#endif /* !__LIBX_VMEM_H__ */

