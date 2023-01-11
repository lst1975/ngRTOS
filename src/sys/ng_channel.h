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

#ifndef __NGRTOS_CHANLLE_H__
#define __NGRTOS_CHANLLE_H__

#include "ng_defs.h"
#include "ng_string.h"
#include "ng_list.h"
#include "ng_semphr.h"
#include "ng_mbuf.h"
#include "ng_msg.h"

typedef struct ng_devtab ng_devtab_s;
typedef struct ng_channel ng_channel_s;

struct ng_channel {
  ng_list_s       link;
  ng_devtab_s	   *dp;
  ng_sem_s        sem;
  uint32_t        type:8;
  uint32_t        pad1:8;
  uint32_t        pad2:16;
  char	         *priv;
};
#define NG_CHT_TX    0x01
#define NG_CHT_RX    0x02
#define NG_CHT_IOCTL 0x04

struct ng_dev_stats{
  u_long errors;
  u_long bytes;
  u_long packets;
  u_long overflow;
};
typedef struct ng_dev_stats ng_dev_stats_s;

struct ng_rxtx{
  ng_list_s mb_head;
  u_long mb_size:21;
  u_long mb_count:10;
  u_long mb_lock:1;
  union{
    ng_mbuf_s *mb_cur;
    ng_channel_s *mb_ch;
  };
  ng_dev_stats_s stat;
};
typedef struct ng_rxtx ng_rxtx_s;

#define NG_DEV_MBUF_MAX  8

#define NG_DEV_PRIORITY_Realtime       0
#define NG_DEV_PRIORITY_Veryhigh       1
#define NG_DEV_PRIORITY_high           2
#define NG_DEV_PRIORITY_normal_above   3
#define NG_DEV_PRIORITY_normal         4
#define NG_DEV_PRIORITY_normal_below   5
#define NG_DEV_PRIORITY_low_above      6
#define NG_DEV_PRIORITY_low            7
#define NG_DEV_PRIORITY_MAX            8

#define NG_DEV_FLAG_TX        0x0001
#define NG_DEV_FLAG_RX        0x0002
#define NG_DEV_FLAG_HOTPLUG   0x0004

struct ng_devtab {
  ng_list_s       channels;
  ng_list_s       link;
  ng_string_s     name;
  ng_result_e   (*init )(ng_devtab_s *dev);
  void          (*free )(ng_devtab_s *dev);
  ng_result_e   (*open )(ng_channel_s *ch, const char *name, int len);
  ng_result_e   (*close)(ng_channel_s *ch);
  int	          (*read )(ng_channel_s *ch, ng_mbuf_s *m);
  int	          (*write)(ng_channel_s *ch, ng_mbuf_s *m);
  int             (*get  )(ng_channel_s *ch, ng_list_s *list);
  int	          (*put  )(ng_channel_s *ch, ng_mbuf_s *m);
  int	          (*ioctl)(ng_channel_s *ch, int cmd, void *arg);
  uint16_t        alloc:1;
  uint16_t        started:1;
  uint16_t        size:12;
  uint16_t        priority:3;
  uint8_t         flag;
  uint8_t         pad;
  ng_rxtx_s       mbrx;
  ng_rxtx_s       mbtx;
  char	          priv[0];
};
#define NG_DEV2PRIV(dev, t) ((t *)((dev)->priv))
#define NG_PRIV2DEV(p) ngrtos_container_of(p, ng_devtab_s, priv);

#define NG_DEV_DEFNAME(name)  NG_CONCAT3(____, name, _dev)
#define NG_DEV_INITFUNC_NAME(name) NG_CONCAT4(____, name, _dev, _init)
#define NG_DEV_INITFUNC_DECL(name) ng_result_e NG_DEV_INITFUNC_NAME(name)(int op)
#define NG_GET_DEV_PRIV(name) (&NG_DEV_DEFNAME(name).priv)
#define NG_DEF_DEV(_name, pri, type) \
  static struct NG_CONCAT2(_name, _dev) { \
    ng_devtab_s dev; \
    type        priv; \
  } NG_DEV_DEFNAME(_name) = { \
    .dev = { \
      .link  = LIST_ENTRY_INIT(), \
      .name  = __DFS(#_name),    \
      .init  = NG_CONCAT2(_name, _init),  \
      .free  = NG_CONCAT2(_name, _deinit),  \
      .open  = NG_CONCAT2(_name, _open),  \
      .close = NG_CONCAT2(_name, _close), \
      .read  = NG_CONCAT2(_name, _read),  \
      .write = NG_CONCAT2(_name, _write), \
      .get   = NG_CONCAT2(_name, _get),  \
      .put   = NG_CONCAT2(_name, _put), \
      .ioctl = NG_CONCAT2(_name, _ioctl), \
      .alloc = ngrtos_FALSE, \
      .started = ngrtos_FALSE, \
      .flag  = 0, \
      .priority = pri, \
      .size  = sizeof(ng_devtab_s) + sizeof(type), \
      .mbrx  = { \
        .mb_head  = LIST_HEAD_INIT(NG_DEV_DEFNAME(_name).dev.mbrx.mb_head), \
        .mb_size  = 0, \
        .mb_count = 0, \
        .mb_lock  = 0, \
        .mb_cur   = NULL, \
        .stat     = { \
            .errors   = 0, \
            .bytes    = 0, \
            .packets  = 0, \
            .overflow = 0, \
          }, \
      }, \
      .mbtx  = { \
        .mb_head  = LIST_HEAD_INIT(NG_DEV_DEFNAME(_name).dev.mbtx.mb_head), \
        .mb_size  = 0, \
        .mb_count = 0, \
        .mb_lock  = 0, \
        .mb_cur   = NULL, \
        .stat     = { \
            .errors   = 0, \
            .bytes    = 0, \
            .packets  = 0, \
            .overflow = 0, \
          }, \
      }, \
    } \
  };\
  NG_DEV_INITFUNC_DECL(_name) \
  { \
    ng_devtab_s *dp = &NG_DEV_DEFNAME(_name).dev; \
    ng_result_e r = op == NG_DEV_OP_DEINIT ? \
      ng_dev_unregister(dp) : ng_dev_register(dp); \
    if (r != NG_result_ok) {\
      NGRTOS_ERROR("Failed to %sinit device %s.", \
          op == NG_DEV_OP_DEINIT ? "de":"", dp->name); \
    }\
    return r; \
  } 

#define NG_DEV_OP_INIT   0
#define NG_DEV_OP_DEINIT 1

ng_result_e ng_dev_register(ng_devtab_s *dp);
ng_result_e ng_dev_unregister(ng_devtab_s *dp);
ng_result_e ng_dev_start(uint8_t *priority, int len);
extern uint8_t dev_start_dft[];

ng_channel_s *ng_open(const ng_string_s *name);
ng_result_e ng_close(ng_channel_s *ch);
int ng_get(ng_channel_s *ch, ng_list_s *list);
int ng_put(ng_channel_s *ch, ng_mbuf_s *m);
int ng_read(ng_channel_s *ch, ng_mbuf_s *m);
int ng_write(ng_channel_s *ch, ng_mbuf_s *m);
int ng_ioctl(ng_channel_s *ch, int cmd, void *arg);

static inline ng_mbuf_s *
ng_dev_get_mbuf_tx(ng_devtab_s *dp)
{
  ngIrqTypeT b;
  ng_mbuf_s *m = NULL;
  ng_rxtx_s *tx = &dp->mbtx;

  NGRTOS_ASSERT(IS_IRQ());

  __ng_irq_priority_sanity_check();
  
  b = __ng_sys_disable_irq();
  tx->stat.packets++;
  tx->stat.bytes += tx->mb_cur->m_len;
  if (!list_empty(&tx->mb_head))
  {
    m = list_first_entry(&tx->mb_head,ng_mbuf_s,m_nextpkt);
    list_del(&m->m_link);
    m->m_nextpkt = NULL;
    tx->mb_size -= m->m_len;
    tx->mb_count--;
  }
  __ng_sys_enable_irq(b);
  return m;
}

static inline ng_result_e
ng_dev_add_mbuf_tx(ng_devtab_s *dp,
  ng_mbuf_s *m, u_long bsz)
{
  ngIrqTypeT b;
  ng_rxtx_s *tx = &dp->mbtx;
  ng_result_e r = NG_result_ok;

  NGRTOS_ASSERT(!IS_IRQ());

  b = __ng_sys_disable_irq();
  if (tx->mb_size + m->m_len > bsz
    || tx->mb_count > NG_DEV_MBUF_MAX)
  {
    r = NG_result_overflow;
    tx->stat.overflow++;
    tx->stat.errors++;
  }
  else
  {
    if (!tx->mb_size && tx->mb_cur == NULL)
    {
      r = NG_result_eirq;
      tx->mb_cur = m;
    }
    else
    {
      list_add_tail(&m->m_link, &tx->mb_head);
      tx->mb_size += m->m_len;
      tx->mb_count++;
    }
  }
  __ng_sys_enable_irq(b);

  if (r == NG_result_overflow)
    m_free_one(m);
  return r;
}

static inline void
ng_dev_get_mbuf_rx(ng_devtab_s *dp, ng_list_s *list)
{
  ngIrqTypeT b;
  ng_rxtx_s *rx = &dp->mbrx;

  NGRTOS_ASSERT(!IS_IRQ());
  b = __ng_sys_disable_irq();
  list_splice_init(&rx->mb_head, list);
  rx->mb_size  = 0;
  rx->mb_count = 0;
  __ng_sys_enable_irq(b);
}

static inline ng_result_e
ng_dev_add_mbuf_rx(ng_devtab_s *dp, 
  ng_mbuf_s *m, u_long bsz)
{
  ngIrqTypeT b;
  ng_rxtx_s *rx = &dp->mbrx;
  ng_result_e r = NG_result_ok;

  NGRTOS_ASSERT(IS_IRQ());

  __ng_irq_priority_sanity_check();
  
  b = __ng_sys_disable_irq();
  if (rx->mb_size + m->m_len > bsz)
  {
    r = NG_result_overflow;
    rx->stat.overflow++;
    rx->stat.errors++;
  }
  else
  {
    list_add_tail(&m->m_link, &rx->mb_head);
    rx->mb_size += m->m_len;
    rx->mb_count++;
    rx->stat.packets++;
  }
  __ng_sys_enable_irq(b);

  return r;
}

#endif
