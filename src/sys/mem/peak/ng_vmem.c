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

#include "ng_defs.h"

#if configEnable_MemPool

#include "ng_mcopy.h"
#include "ng_limits.h"
#include "ng_list.h"
#include "ng_utils.h"
#include "mem/ng_mem.h"

#define __USE_DEBUG  1
#define __USE_SIMPLE 0
#define __USE_BIGBLOCK__ 1

#define LRN "\n"
#define boolean ng_bool_t

#define DListHeadT ng_list_s
#define list_head_init INIT_LIST_HEAD
#define list_get_entry(m, h, t, field) do { \
    m = list_first_entry(h,t,field); \
    list_del(&(m)->field); \
  } while(0)
  
#define list_poison __list_poison_entry

#define MEM_MLOCK_T(lock) ngIrqTypeT lock
#define MEM_ACQUIRE_LOCK(l) l = __ng_sys_disable_irq()
#define MEM_RELEASE_LOCK(l) __ng_sys_enable_irq(l)
#define MEM_DESTROY_LOCK(l,a) NGRTOS_UNUSED(l);NGRTOS_UNUSED(a)
static inline ng_result_e MEM_CREATE_LOCK(ngIrqTypeT l, const char *n)
{
  NGRTOS_UNUSED(l);NGRTOS_UNUSED(n);
  return NG_result_ok;
}

#define log_crit NGRTOS_ERROR
#define BIT_TEST ngRTOS_BIT_TEST
#define ______SIZE_MAX ng_INT_MAX
#define CHAR_LINE_END '\0'

#define bcopy(s, d, l) ng_memcpy(d, s, l)
#define bzero(s, l) ng_mzero(s, l)
#define log_print NGRTOS_PRINT
#define log_info  NGRTOS_EVENT
#define LOGNMEM() NGRTOS_ERROR("Out of memory!!!\n")

boolean __use_winm = ngrtos_FALSE;
boolean __show_memusage = ngrtos_TRUE;

#define MEM_ALIGNBYTES __ALIGNBYTES

#define MEM_ALIGN(p)  __ROUNDUP(p, size_t, MEM_ALIGNBYTES)

#define MEM_PREHLEN ngrtos_offsetof(DMemHeaderT,m_none)
#define MEM_PAD_LEN  (MEM_ALIGN(MEM_PREHLEN)-MEM_PREHLEN)

#define MEM_HEANDER_LEN  (sizeof(DMemHeaderT))
#define MEM_ADJUSTED_SIZE(size) (size+MEM_HEANDER_LEN)
#define MEM_MTODATA(m)  (((char *)(m)+MEM_HEANDER_LEN))
#define MEM_DATATOM(addr) ((DMemHeaderT*)((char *)addr-MEM_HEANDER_LEN))

 /******************************************************
  * memory block header, used only here
 ******************************************************/

typedef struct d_mem_header_t DMemHeaderT;
typedef struct d_mem_block_t DMemBlockT;

struct d_mem_header_t{
  DListHeadT m_field;
  uint16_t m_mtype; /**/
  uint16_t m_blockid; /**/
#if __USE_KOS_DEBUG__
  const char * m_filename; /**/
  int m_fileline;
#endif
  uint8_t m_none[0];
};

struct d_mem_block_t {
  uint32_t mblockid:4;
  uint32_t msize:16;
  uint32_t mused:12;
  uint32_t mfreed:8;
  uint32_t mnumber:8;
  uint32_t mpad:16;
  uint32_t musedmax;
  DListHeadT mready;
  DListHeadT musing;
#if __USE_BIGBLOCK__
  uint8_t * mbigblock;
  uint8_t * cur;
  uint8_t * end;
#endif
  MEM_MLOCK_T(mlock);
};

/* maxed packet transmit, initially defined in ping6.cpp*/
#define MAX_SPECIAL_SIZE  (1024*1024)

/******************************************************
 * system global varialbes
******************************************************/
#define  MAX_MEM_BLOCK 12
static long numxxx;
#define __DECLR(n) .mready = LIST_HEAD_INIT(blockarray[n].mready)
#define __DECLU(n) .musing = LIST_HEAD_INIT(blockarray[n].musing)
DMemBlockT blockarray[MAX_MEM_BLOCK] = {
  { .mblockid=0, /*SIZE*/.msize = MEM_ALIGN(   64),  .mused=0, .mfreed=0, 
    .musedmax=0, /*SIZENUM*/.mnumber=8, __DECLR(0), __DECLU(0) },
  { .mblockid=1, /*SIZE*/.msize = MEM_ALIGN(  108),  .mused=0, .mfreed=0, 
    .musedmax=0, /*SIZENUM*/.mnumber=8, __DECLR(1), __DECLU(1) },
  { .mblockid=2, /*SIZE*/.msize = MEM_ALIGN(  280),  .mused=0, .mfreed=0,
    .musedmax=0, /*SIZENUM*/.mnumber=8, __DECLR(2), __DECLU(2) },
  { .mblockid=3, /*SIZE*/.msize = MEM_ALIGN(  416),  .mused=0, .mfreed=0, 
    .musedmax=0, /*SIZENUM*/.mnumber=8, __DECLR(3), __DECLU(3) },
  { .mblockid=4, /*SIZE*/.msize = MEM_ALIGN( 1024),  .mused=0, .mfreed=0, 
    .musedmax=0, /*SIZENUM*/.mnumber=8, __DECLR(4), __DECLU(4) },
  { .mblockid=5, /*SIZE*/.msize = MEM_ALIGN( 2072),  .mused=0, .mfreed=0, 
    .musedmax=0, /*SIZENUM*/.mnumber=0, __DECLR(5), __DECLU(5) },
  { .mblockid=6, /*SIZE*/.msize = MEM_ALIGN( 3000),  .mused=0, .mfreed=0,
    .musedmax=0, /*SIZENUM*/.mnumber=0, __DECLR(6), __DECLU(6) },
  { .mblockid=7, /*SIZE*/.msize = MEM_ALIGN( 4000),  .mused=0, .mfreed=0,
    .musedmax=0, /*SIZENUM*/.mnumber=0, __DECLR(7), __DECLU(7) },
  { .mblockid=8, /*SIZE*/.msize = MEM_ALIGN( 5000),  .mused=0, .mfreed=0,
    .musedmax=0, /*SIZENUM*/.mnumber=0, __DECLR(8), __DECLU(8) },
  { .mblockid=9, /*SIZE*/.msize = MEM_ALIGN( 6000),  .mused=0, .mfreed=0,
    .musedmax=0, /*SIZENUM*/.mnumber=0, __DECLR(9), __DECLU(9) },
  {.mblockid=10, /*SIZE*/.msize = MEM_ALIGN(30000),  .mused=0, .mfreed=0,
    .musedmax=0, /*SIZENUM*/.mnumber=0, __DECLR(10),__DECLU(10) },
};

/******************************************************
 * main malloc and free functions
 *******************************************************/
static __inline DMemHeaderT *
list_alloc(DMemBlockT* list)
{
  DMemHeaderT *m;

  MEM_ACQUIRE_LOCK(list->mlock);

  if (list_empty(&list->mready))
  {
#if __USE_BIGBLOCK__
    if (list->cur < list->end)
    {
      m = (DMemHeaderT*)list->cur;
      list->cur += list->msize;
      list_poison(&m->m_field);
      m->m_blockid = list->mblockid;
    }
    else
#endif
    {
      m = (DMemHeaderT*)vos_malloc(list->msize);
      if (m == NULL)
        goto leave;
      list_poison(&m->m_field);
    }
  }
  else
  {
    list_get_entry(m, &list->mready,
      DMemHeaderT, m_field);
    list->mfreed--;
  }

  m->m_blockid = list->mblockid;
  list_add_tail(&m->m_field, &list->musing);
  list->mused++;
  if (list->mused > list->musedmax)
    list->musedmax = list->mused;

leave:
  MEM_RELEASE_LOCK(list->mlock);
  return m;
}

static __inline void
list_free(void * maddr)
{
  DMemBlockT* list;
  DMemHeaderT *m;

  m = MEM_DATATOM(maddr);
  if (m->m_blockid == MEM_FROM_SYSTEM) 
  {
    vos_free(m);
    return;
  }

  if (m->m_mtype == M_FREE) 
  {
    log_crit("free same memory again, called "
      "file:%s, line:%d"LRN,
      m->m_filename, 
      m->m_fileline);
    return;
  }
  list = &blockarray[m->m_blockid];

  MEM_ACQUIRE_LOCK(list->mlock);
  list_del(&m->m_field);
  list_add_tail(&m->m_field, &list->mready);
  list->mused--;
  list->mfreed++;
  m->m_mtype = M_FREE;
  MEM_RELEASE_LOCK(list->mlock);
}

#define M_RETRY_MAX 50

static __inline int
mem_get_blockid(size_t size)
{
  int pos1=0, pos2, pos3 = MAX_MEM_BLOCK-1;
  DMemBlockT *mlist = blockarray;

  pos2 = pos3 >> 1;
  while (ngrtos_TRUE)
  {
    if (size == mlist[pos2].msize)
    {
      return pos2;
    }
    else if (size < mlist[pos2].msize)
    {
      if (pos2 == 0)
        return pos2;
      if (size > mlist[pos2-1].msize)
        return pos2;
      pos3 = pos2;
      pos2 = ((pos2+pos1) >> 1);
    }
    else
    {
      if (pos2 == MAX_MEM_BLOCK-1 ||
        pos2+1 == MAX_MEM_BLOCK-1)
        return -1;
      if (size < mlist[pos2+1].msize)
        return pos2+1;
      pos1 = pos2;
      pos2 = ((pos2+pos3) >> 1);
    }
  }
}

void *
__sys_malloc(size_t size, int type, int flag
#if __USE_KOS_DEBUG__
  , const char * file, int line
#endif
  )
{
  DMemHeaderT *m;
  void * p=NULL;
  int blockid;

  if (__use_winm)
  {
    void *m = vos_malloc(size);
    if (BIT_TEST(flag, M_ZERO))
      bzero(m, size);
    return m;
  }

  if (size <= 0) 
  {
    log_crit("try to alloc size of 0, called "
      "file:%s, line:%d"LRN, file, line);
    return NULL;
  }

  blockid = mem_get_blockid(MEM_ADJUSTED_SIZE(size));
restart:
  if (blockid != -1)
  {
    m = list_alloc(&blockarray[blockid]);
  }
  else
  {
    m = (DMemHeaderT*)vos_malloc(MEM_ADJUSTED_SIZE(size));
    if (m == NULL) 
    {
      log_crit("System out of memory, called file:%s, line:%d"LRN,
         file, line);
      return NULL;
    }
    m->m_blockid = MEM_FROM_SYSTEM;
    numxxx++;
  }

  if (m == NULL && !BIT_TEST(flag, M_NOWAIT))
  {
    kos_task_switch2other(NULL);
    goto restart;
  }

  if (m!=NULL) 
  {
    p = MEM_MTODATA(m);
#if __USE_KOS_DEBUG__
    m->m_filename = file;
    m->m_fileline = line;
#endif
    m->m_mtype = type;
    if (BIT_TEST(flag, M_ZERO))
      bzero(p, size);
  }

  return p;
}

void
__sys_free(void * maddr, int type
#if __USE_KOS_DEBUG__
  , const char * file, int line
#endif
  )
{
  if (__use_winm)
  {
    vos_free(maddr);
    return;
  }
  if (maddr == NULL) 
  {
    log_crit("Free null address, file:%s, line %d"LRN, 
      file, line);
    return;
  }
  list_free(maddr);
}

void *
__sys_realloc(void * p, size_t size
#if __USE_KOS_DEBUG__
  , const char * file, int line
#endif
  )
{
  size_t sizep=0;
  char * newp;
  int mtype = M_TEMP;

  if (__use_winm)
  {
    if (p == NULL)
      return vos_malloc(size);
    return vos_realloc(p, size);
  }

  if (p == NULL) 
  {
    if (size)
#if __USE_KOS_DEBUG__
      return (void *)__sys_malloc(size, mtype, M_NOWAIT, file, line);
#else
      return (void *)__sys_malloc(size, mtype, M_NOWAIT);
#endif
    if (size == 0)
      return NULL;
  } else if (size == 0) {
#if __USE_KOS_DEBUG__
    __sys_free(p, mtype, file, line);
#else
    __sys_free(p);
#endif
    return NULL;
  }
  else
  {
    DMemHeaderT *m;
    m = MEM_DATATOM(p);
    sizep = blockarray[m->m_blockid].msize;
    sizep -= MEM_HEANDER_LEN;
    mtype = m->m_mtype;
  }

  /* must malloc first, and then free old p,
   * otherwise, p will be realloced and was
   * filled with 0
   */
#if __USE_KOS_DEBUG__
  newp = (char *)__sys_malloc(size, mtype, M_NOWAIT, file, line);
#else
  newp = (char *)__sys_malloc(size, mtype, M_NOWAIT);
#endif
  if (p != NULL){
#if __USE_KOS_DEBUG__
    __sys_free(p, mtype, file, line);
#else
    __sys_free(p, mtype);
#endif
  }
  if (newp != NULL && sizep != 0)
    bcopy(p, newp, min(sizep, size));
  return newp;
}

void *
__sys_calloc(size_t num, size_t size
#if __USE_KOS_DEBUG__
  , const char * file, int line
#endif
  )
{
  if (__use_winm){
    return vos_calloc(num, size);
  }

  void * p;
  if (num * size >= ______SIZE_MAX)
    return NULL;
  size *= num;
#if __USE_KOS_DEBUG__
  p = __sys_malloc(size, M_TEMP, M_NOWAIT, file, line);
#else
  p = __sys_malloc(size, M_TEMP);
#endif
  if (p != NULL)
    bzero(p, size);
  return (p);
}

/**
 * strdupex - Copies a string into a newly created location.
 * @str: The source string
 *
 * returns the address of the new location
 */
char *
__sys_strdup_len(const char *str, int *length
#if __USE_KOS_DEBUG__
  , const char * file, int line
#endif
)
{
  char *tmp;
  size_t len;

  if (str == NULL)
    return NULL;

  if (__use_winm)
    return (char *)vos_strdup_len(str, length);

  tmp = NULL;
  len = (length == NULL || *length == 0) ?
    ng_strlen(str) : *length;

#if __USE_KOS_DEBUG__
  tmp = (char*)__sys_malloc(1+ len,
    M_TEMP, M_NOWAIT, file, line);
#else
  tmp = (char*)__sys_malloc(1+ len,
    M_TEMP, M_NOWAIT);
#endif
  if (tmp != NULL)
    bcopy(str, tmp, len+1);

  if (length != NULL)
    *length = len;
  tmp[len] = CHAR_LINE_END;
  return (char *)tmp;
}

char *
__sys_strdup(const char * str
#if __USE_KOS_DEBUG__
  , const char * file, int line
#endif
)
{
  if (__use_winm){
    if (!str) return NULL;
      return (char *)vos_strdup(str);
  }

#if __USE_KOS_DEBUG__
  return (char *)__sys_strdup_len(str, NULL, file, line);
#else
  return (char *)__sys_strdup_len(str, NULL);
#endif
}

/******************************************************
 *    function provided for pool.
 *******************************************************/
#if __USE_KOS_DEBUG__
void
mem_file_line_change(const char * file,
  int line, void * maddr)
{
  DMemHeaderT *m;
  m = MEM_DATATOM(maddr);
  m->m_filename = file;
  m->m_fileline = line;
}

void
mem_file_line_get(const char * *file,
  int *line, void * maddr)
{
  DMemHeaderT *m;
  m = MEM_DATATOM(maddr);
  *file = m->m_filename;
  *line = m->m_fileline;
}
#endif

/******************************************************
 *    Initialize the mem  list.
 *******************************************************/
static ng_result_e
mem_init_onesize(DMemBlockT *mlist, int blockid)
{
  int msize, count;

  if (!mlist->mnumber)
    return NG_result_ok;
  
#if !__USE_BIGBLOCK__
  msize = mlist->msize;
  count = mlist->mnumber;
  while (count--> 0) 
  {
    DMemHeaderT *m;
    m = (DMemHeaderT*)vos_malloc(msize);
    if (!m) 
    {
      LOGNMEM();
      return NG_result_nmem;
    }
    bzero(m, msize);
    m->m_blockid = blockid;
    list_poison(&m->m_field);
    list_add_tail(&m->m_field,&mlist->mready);
  }
#else
  uint8_t * p0, *p;
  size_t size;

  msize = mlist->msize;
  count = mlist->mnumber;
  size = count*msize+MEM_ALIGNBYTES;
  p0 = (uint8_t *)vos_malloc(size);
  if (p0 == NULL) 
  {
    LOGNMEM();
    return NG_result_nmem;
  }

  p = (uint8_t *)MEM_ALIGN(p0);
  mlist->mbigblock = p0;
  mlist->cur = p;
  mlist->end = p+count*msize;
#if 0  
  {
    for (int i=0; i<count; i++) 
    {
      DMemHeaderT *m;
      m = (DMemHeaderT*)p;
      m->m_blockid = blockid;
      list_poison(&m->m_field);
      list_add_tail(&m->m_field, &mlist->mready);
      p+=msize;
    }
    KASSERT(i==count);
  }
#endif  
#endif
  return NG_result_ok;
}

static __inline void
mem_free_mused(DMemBlockT *mlist, boolean del)
{
  DMemHeaderT *m;
  if (!list_empty(&mlist->musing)) 
  {
    log_crit("Memory leak!!!");
    while (!list_empty(&mlist->musing)) 
    {
      m = list_first_entry(&mlist->musing,
        DMemHeaderT, m_field);
      log_crit("Memory leak, called file:%s, line:%d, "
        "size %lu, %p", m->m_filename, 
        m->m_fileline, mlist->msize,
        (void*)(MEM_MTODATA(m)));
      list_del(&m->m_field);
      if (del) vos_free(m);
    }
  }
}

static void
mem_free_onesize(DMemBlockT *mlist)
{
#if !__USE_BIGBLOCK__
  while (!list_empty(&mlist->mready)) 
  {
    DMemHeaderT *m;
    m = list_first_entry(&mlist->mready,
      DMemHeaderT, m_field);
    list_del(&m->m_field);
    vos_free(m);
  }
  mem_free_mused(mlist, ngrtos_TRUE);
#else
  if (mlist->mbigblock == NULL)
  {
    KASSERT(list_empty(&mlist->mready));
    KASSERT(list_empty(&mlist->musing));
    return;
  }
  mem_free_mused(mlist, ngrtos_FALSE);
  vos_free(mlist->mbigblock);
#endif
}

ng_result_e
__sys_mem_init(void)
{
  int n;
  ng_result_e error;
  DMemBlockT *mlist;

  if (!__use_winm) 
  {
    mlist = blockarray;
    for (n=0; n<MAX_MEM_BLOCK; n++) 
    {
#if __USE_BIGBLOCK__
      mlist[n].mbigblock = NULL;
#else
      mlist[n].mfreed = mlist[n].mnumber;
#endif
      MEM_CREATE_LOCK(mlist[n].mlock, NULL);
    }
    numxxx = 0;

    for (n=0; n<MAX_MEM_BLOCK; n++)
      if ((error = MEM_CREATE_LOCK(mlist[n].mlock, "memblock"))!=0)
        goto clean0;

    for (n=0; n<MAX_MEM_BLOCK; n++)
      if ((error = mem_init_onesize(&mlist[n], n))!=0)
        goto clean1;
  }

#if __USE_DEBUG
  if (__show_memusage)
    log_info("Initialize memory successfully");
#endif
  return NG_result_ok;

clean1:
  for (n=0; n<MAX_MEM_BLOCK; n++)
    mem_free_onesize(&mlist[n]);
clean0:
  for (n=0; n<MAX_MEM_BLOCK; n++)
    MEM_DESTROY_LOCK(mlist[n].mlock, NULL);
  return error;
}

/******************************************************
 *    show  the mem  list.
 *******************************************************/
#define M_SHOW_HSTR "   "
#define MSHOW_FMT "%s size:%-6lu used:%-6lu ready:%-6lu count:%-6lu burst:%-6lu max:%-6lu"
#define MSHOW_FMTX "%s size:%-6s used:%-6lu ready:%-6s count:%-6lu burst:%-6s max:%-6lu"
#define MSHOW_FMT_ENC "*******************************************************************************"
#define MSHOW_PRO_EMPTY "    "
#define MSHOW_PROLOG \
  MSHOW_PRO_EMPTY"###################################################################"LRN \
  MSHOW_PRO_EMPTY"######### 1.This is the MLCT tool 1.0                     #########"LRN \
  MSHOW_PRO_EMPTY"#########        -- for memory leaking check              #########"LRN \
  MSHOW_PRO_EMPTY"######### 2.This is the MUT tool 1.0                      #########"LRN \
  MSHOW_PRO_EMPTY"#########        -- for memory usage analyzing            #########"LRN \
  MSHOW_PRO_EMPTY"######### 3.This is the RTMM tool 1.0                     #########"LRN \
  MSHOW_PRO_EMPTY"#########        -- for realtime memory management        #########"LRN \
  MSHOW_PRO_EMPTY"###################################################################"LRN \
  MSHOW_PRO_EMPTY"######### Copyright Liusongtao, Email:songtaoliu@sohu.com #########"LRN \
  MSHOW_PRO_EMPTY"###################################################################"

void
mem_show_general(size_t size, int all, int show)
{
  if (!__show_memusage)
    return;

  log_print(MSHOW_FMT_ENC);

#if !__USE_SIMPLE
  do {
    int i;
    DMemBlockT *mlist;

    log_print(MSHOW_PROLOG);
    log_print(M_SHOW_HSTR" Memory allocation statistics table:");

    mlist = blockarray;
    for (i=0; i<MAX_MEM_BLOCK; i++)
    {
      if (all || size == mlist[i].msize)
      {
        log_print(MSHOW_FMT
            , M_SHOW_HSTR
            , (u_long)mlist[i].msize
            , mlist[i].mused
            , mlist[i].mfreed
            , mlist[i].mnumber
            , mlist[i].mused+mlist[i].mfreed-mlist[i].mnumber
            , mlist[i].musedmax);
      }
    }
    if (all || size == 0)
    {
      log_print(MSHOW_FMTX
            , M_SHOW_HSTR
            , "xxx"
            , numxxx
            , "xxx"
            , numxxx
            , "xxx"
            , numxxx);
      log_print(MSHOW_FMT_ENC);
    }
  }
  while(0);
#else
  (void)(size);
  (void)(all);
  (void)(show);
#endif
}

/******************************************************
 *    free  the mem  list.
 *******************************************************/
void
__sys_mem_close(boolean quiet)
{
  int show = 1;
  int n, mleaks;
  DMemHeaderT *m;
  DMemBlockT *mlist;

  if (!__use_winm) 
  {
    mleaks= 0;
    mlist = blockarray;

    if (!quiet)
      mem_show_general(0, ngrtos_TRUE, show);

    if (__show_memusage)
    {
      for (n=0; n<MAX_MEM_BLOCK; n++)
        if (!list_empty(&mlist[n].musing))
          list_for_each_entry(m, DMemHeaderT,
            &mlist[n].musing, m_field)
            mleaks++;

      if (!quiet)
      {
        log_print(M_SHOW_HSTR" memory leaks:%d"
          M_SHOW_HSTR, mleaks);
        log_print(MSHOW_FMT_ENC);
      }
    }

    for (n=0; n<MAX_MEM_BLOCK; n++)
      mem_free_onesize(&mlist[n]);

    for (n=0; n<MAX_MEM_BLOCK; n++)
      MEM_DESTROY_LOCK(mlist[n].mlock, NULL);

    KASSERT(!mleaks);
  }
  return;
}

#endif
