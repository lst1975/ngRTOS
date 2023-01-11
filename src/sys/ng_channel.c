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
#include "ng_channel.h"
#include "mem/ng_mem.h"
#include "ng_buffer.h"
#include "ng_prio.h"

#define __DECL___(n) [n] = LIST_HEAD_INIT(dev_list[n])

static ng_list_s dev_list[NG_DEV_PRIORITY_MAX] = {
  __DECL___(NG_DEV_PRIORITY_Realtime),
  __DECL___(NG_DEV_PRIORITY_Veryhigh),
  __DECL___(NG_DEV_PRIORITY_high),
  __DECL___(NG_DEV_PRIORITY_normal_above),
  __DECL___(NG_DEV_PRIORITY_normal),
  __DECL___(NG_DEV_PRIORITY_normal_below),
  __DECL___(NG_DEV_PRIORITY_low_above),
  __DECL___(NG_DEV_PRIORITY_low),
};
uint8_t dev_start_dft[]={
  NG_DEV_PRIORITY_Realtime,
  NG_DEV_PRIORITY_Veryhigh,
  NG_DEV_PRIORITY_high,
  NG_DEV_PRIORITY_normal_above,
  NG_DEV_PRIORITY_normal,
  NG_DEV_PRIORITY_normal_below,
  NG_DEV_PRIORITY_low_above,
  NG_DEV_PRIORITY_low,
};

#define CHANNELS  680

/* announce: register a device */
ng_result_e ng_dev_register(ng_devtab_s *dp)
{
  ngIrqTypeT x;
  ng_devtab_s *d;

  if (dp->alloc)
  {
    d = (ng_devtab_s *)ng_malloc(dp->size);
    if (d == NULL)
    {
      NGRTOS_ERROR("Failed to malloc device %pB.\n", &dp->name);
      return NG_result_nmem;
    }
    ng_memcpy (d, dp, dp->size);
  }
  else
  {
    d = dp;
  }
  INIT_LIST_HEAD(&dp->channels);

  x = __ng_sys_disable_irq();
  list_cmp_add_before(dp, 
                      ng_devtab_s, 
                      &dev_list[dp->priority], 
                      link, 
                      __task_pri_is_TgtE, 
                      priority);
  __ng_sys_enable_irq(x);
  return NG_result_ok;
}

/* announce: register a device */
ng_result_e ng_dev_unregister(ng_devtab_s *dp)
{
  ngIrqTypeT x = __ng_sys_disable_irq();
  list_del(&dp->link);
  __ng_sys_enable_irq(x);
  if (dp->alloc)
    ng_free(dp);
  return NG_result_ok;
}

ng_result_e ng_dev_start(uint8_t *priority, int len)
{
  int i;
  ng_result_e r;
  ng_devtab_s *dp;
  ng_list_s *head;
  ng_devtab_s *prev;

  if (priority == NULL)
  {
    priority = dev_start_dft;
    len      = NG_DEV_PRIORITY_MAX;
  }

  for (i = 0, prev = NULL; i < len; i++)
  {
    head = &dev_list[priority[i]];
    list_for_each_entry(dp,ng_devtab_s,head,link)
    {
      r = dp->init(dp);
      if (r != NG_result_ok)
      {
        NGRTOS_ERROR("Failed to init device %pB.", &dp->name);
        goto err0;
      }
      NGRTOS_ERROR("Init device %pB: OK.", &dp->name);
      prev = dp;
      dp->started = ngrtos_TRUE;
    }
  }
  return NG_result_ok;
  
err0:  
  if (prev != NULL)
  {
    head = &dev_list[priority[i]];
    list_for_each_entry_from_reverse(prev,ng_devtab_s,
      head,link)
    {
      dp->free(dp);
    }
  }
  while(i--)
  {
    head = &dev_list[priority[i]];
    list_for_each_entry(dp,ng_devtab_s,head,link)
    {
      dp->free(dp);
    }
  }
  return r;
}

static ng_devtab_s *
__ng_dev_find(const char *name, int len)
{
  ng_list_s    *head;
  ng_list_s    *e;
  ng_devtab_s  *dp;
  ngIrqTypeT    x;

  x = __ng_sys_disable_irq();
  for (int i = 0; i < NG_DEV_PRIORITY_MAX; i++)
  {
    head = &dev_list[i];
    list_for_each(e, head)
    {
      dp = list_entry(e, ng_devtab_s, link);
      if (dp->name.len == len
        && ng_memcmp(dp->name.ptr, name, len) == 0)
        break;
      dp = NULL;
    }
  }
  __ng_sys_enable_irq(x);
  return dp;
}

/* open: call a device open and return a channel */
ng_channel_s *
ng_open(const ng_string_s *name)
{
  ngIrqTypeT    x;
  ng_result_e   r;
  ng_devtab_s  *dp;
  ng_channel_s *ch;
  const char   *p;

  p = ng_strchr(name, '/');
  if (p == NULL)
  {
    NGRTOS_ERROR("Bad name %pB, need '/'.\n", name);
    goto err0;
  }
  
  dp = __ng_dev_find(name->ptr, p-name->ptr);
  if (dp == NULL)
  {
    NGRTOS_DEBUG("Failed to find device %pB.\n", name);
    goto err0;
  }
  
  ch = (ng_channel_s *)ng_malloc(sizeof(*ch));
  if (ch == NULL) 
  {
    NGRTOS_ERROR("Failed to malloc channel %pB.\n", name);
    goto err0;
  }
  
  ch->dp   = dp;
  ch->priv = NULL;
  ch->type = 0;

  r = ngrtos_sem_init(&ch->sem, &dp->name);
  if (r != NG_result_ok)
  {
    NGRTOS_ERROR("Failed to init channel semaphr.\n");
    goto err1;
  }
  
  r = (*dp->open)(ch, name->ptr+dp->name.len, 
                      name->len-dp->name.len);
  if (r != NG_result_ok) 
  {
    NGRTOS_ERROR("Device refused to open %pB.\n", name);
    goto err2;
  }

  x = __ng_sys_disable_irq();
  list_add_tail(&ch->link, &dp->channels);
  __ng_sys_enable_irq(x);
  return ch;
  
err2:  
  ngrtos_sem_destroy(&ch->sem);
err1:  
  ng_free(ch);
err0:  
  NGRTOS_ERROR("Failed to open channel %pB.\n", name);
  return NULL;
}

/* close: close a channel and return associated channel */
ng_result_e ng_close(ng_channel_s *ch)
{
  ng_result_e r;
  if (ch == NULL) 
    return NG_result_ok;
  r = (*ch->dp->close)(ch);
  ngrtos_sem_destroy(&ch->sem);
  ng_free(ch);
  return r;
}

/* write: stub for the driver */
int ng_write(ng_channel_s *ch, ng_mbuf_s *m)
{
  if (ch == NULL 
    || ngRTOS_BIT_TEST(ch->type,NG_CHT_IOCTL)
    || !ngRTOS_BIT_TEST(ch->type, NG_CHT_TX)
    || !ngRTOS_BIT_TEST(ch->dp->flag, NG_DEV_FLAG_TX)) 
    return -NG_result_nsupport;
  return (*ch->dp->write)(ch, m);
}

/* read: stub for the driver */
int ng_read(ng_channel_s *ch, ng_mbuf_s *m)
{
  if (ch == NULL 
    || ngRTOS_BIT_TEST(ch->type,NG_CHT_IOCTL)
    || !ngRTOS_BIT_TEST(ch->type,NG_CHT_RX)
    || !ngRTOS_BIT_TEST(ch->dp->flag, NG_DEV_FLAG_RX)) 
    return -NG_result_nsupport;
  return (*ch->dp->read)(ch, m);
}

int ng_get(ng_channel_s *ch, ng_list_s *list)
{
  if (ch == NULL 
    || ngRTOS_BIT_TEST(ch->type,NG_CHT_IOCTL)
    || !ngRTOS_BIT_TEST(ch->type,NG_CHT_RX)
    || !ngRTOS_BIT_TEST(ch->dp->flag, NG_DEV_FLAG_RX)) 
    return -NG_result_nsupport;
  return (*ch->dp->get)(ch, list);
}

/* read: stub for the driver */
int ng_put(ng_channel_s *ch, ng_mbuf_s *m)
{
  if (ch == NULL 
    || ngRTOS_BIT_TEST(ch->type,NG_CHT_IOCTL)
    || !ngRTOS_BIT_TEST(ch->type,NG_CHT_TX)
    || !ngRTOS_BIT_TEST(ch->dp->flag, NG_DEV_FLAG_TX)) 
    return -NG_result_nsupport;
  return (*ch->dp->put)(ch, m);
}

/* ioctl: stub for the driver */
int ng_ioctl(ng_channel_s *ch, int cmd, void *ptr)
{
  NGRTOS_ASSERT(!ngRTOS_BIT_TEST(ch->type,NG_CHT_TX|NG_CHT_RX));
  if (ch == NULL 
    || !ngRTOS_BIT_TEST(ch->type,NG_CHT_IOCTL)) 
    return -NG_result_nsupport;
  return (*ch->dp->ioctl)(ch, cmd, ptr);
}
