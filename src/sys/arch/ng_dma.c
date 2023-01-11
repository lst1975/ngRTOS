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

#include "ng_channel.h"
#include "ng_hal.h"
#include "mem/ng_mem.h"
#include "ng_uartx.h"

#define __NG_DMA_STREAM_MAX 16

#if defined (DMA_SxCR_CHSEL_3)
#define __NG_DMA_CHANNEL_MAX 16
#else
#define __NG_DMA_CHANNEL_MAX 8
#endif

static __dma_cb_f __dma_irq_table[__NG_DMA_STREAM_MAX] = {
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};
#define __DECL_DMA_IRQ(m,n) \
  void DMA##m##_Stream##n##_IRQHandler(void) {\
    __dma_cb_f f = __dma_irq_table[((m-1)<<3)+n];\
    if (f != NULL) f(); \
  }
__DECL_DMA_IRQ(1, 0);
__DECL_DMA_IRQ(1, 1);
__DECL_DMA_IRQ(1, 2);
__DECL_DMA_IRQ(1, 3);
__DECL_DMA_IRQ(1, 4);
__DECL_DMA_IRQ(1, 5);
__DECL_DMA_IRQ(1, 6);
__DECL_DMA_IRQ(1, 7);
__DECL_DMA_IRQ(2, 0);
__DECL_DMA_IRQ(2, 1);
__DECL_DMA_IRQ(2, 2);
__DECL_DMA_IRQ(2, 3);
__DECL_DMA_IRQ(2, 4);
__DECL_DMA_IRQ(2, 5);
__DECL_DMA_IRQ(2, 6);
__DECL_DMA_IRQ(2, 7);

void __dma_register_IRQ(uint32_t ch, __dma_cb_f cb)
{
  __dma_irq_table[ch]  = cb;
}

void __dma_unregister_IRQ(uint32_t ch)
{
  __dma_irq_table[ch]  = NULL;
}

#define __NG_DMA1 0
#define __NG_DMA2 1

typedef struct __dma_clkfunc __dma_clkfunc_s;
typedef void (*__dma_clk)(__dma_clkfunc_s *u);
struct __dma_clkfunc{
  __dma_clk enable;
  __dma_clk disable;
  ngAtomTypeT users;
};

static void ng__HAL_RCC_DMA1_CLK_ENABLE(__dma_clkfunc_s *u);
static void ng__HAL_RCC_DMA2_CLK_ENABLE(__dma_clkfunc_s *u);
static void ng__HAL_RCC_DMA1_CLK_DISABLE(__dma_clkfunc_s *u);
static void ng__HAL_RCC_DMA2_CLK_DISABLE(__dma_clkfunc_s *u);

static __dma_clkfunc_s __dma_clkinit[]={
  { .enable  = ng__HAL_RCC_DMA1_CLK_ENABLE,
    .disable = ng__HAL_RCC_DMA1_CLK_DISABLE,
    .users   = 0,
  },
  { .enable  = ng__HAL_RCC_DMA2_CLK_ENABLE,
    .disable = ng__HAL_RCC_DMA2_CLK_DISABLE,
    .users   = 0,
  },
};

static void ng__HAL_RCC_DMA1_CLK_ENABLE(__dma_clkfunc_s *u)
{
  if (u->users++) return;
  __HAL_RCC_DMA1_CLK_ENABLE();
}

static void ng__HAL_RCC_DMA2_CLK_ENABLE(__dma_clkfunc_s *u)
{
  if (u->users++) return;
  __HAL_RCC_DMA2_CLK_ENABLE();
}

static void ng__HAL_RCC_DMA1_CLK_DISABLE(__dma_clkfunc_s *u)
{
  NGRTOS_ASSERT(u->users);
  if (--u->users) return;
  __HAL_RCC_DMA1_CLK_DISABLE();
}

static void ng__HAL_RCC_DMA2_CLK_DISABLE(__dma_clkfunc_s *u)
{
  NGRTOS_ASSERT(u->users);
  if (--u->users) return;
  __HAL_RCC_DMA2_CLK_DISABLE();
}

__STATIC_FORCEINLINE void 
ng__HAL_RCC_DMA_CLK_ENABLE(uint32_t id)
{
  __dma_clkinit[id].enable(&__dma_clkinit[id]);
}

__STATIC_FORCEINLINE void 
ng__HAL_RCC_DMA_CLK_DISABLE(uint32_t id)
{
  __dma_clkinit[id].disable(&__dma_clkinit[id]);
}

#if defined (DMA_SxCR_CHSEL_3)
#define __NG_DMA_CHANNEL_MAX 16
#else
#define __NG_DMA_CHANNEL_MAX 8
#endif
static uint32_t __dma_channel_ids[__NG_DMA_CHANNEL_MAX] = {
  DMA_CHANNEL_0,
  DMA_CHANNEL_1,
  DMA_CHANNEL_2,
  DMA_CHANNEL_3,
  DMA_CHANNEL_4,
  DMA_CHANNEL_5,
  DMA_CHANNEL_6,
  DMA_CHANNEL_7,
#if defined (DMA_SxCR_CHSEL_3)
  DMA_CHANNEL_8,
  DMA_CHANNEL_9,
  DMA_CHANNEL_10,
  DMA_CHANNEL_11,
  DMA_CHANNEL_12,
  DMA_CHANNEL_13,
  DMA_CHANNEL_14,
  DMA_CHANNEL_15,
#endif
};

struct __dma_bucket{
  DMA_Stream_TypeDef *stream;
  uint32_t dma:2;
  uint32_t irq:8;
  uint32_t used:1;
};
typedef struct __dma_bucket __dma_bucket_s;
#define __DECL_DMA_BUCKETS(an, sn) \
  { DMA##an##_Stream##sn, __NG_DMA##an,  DMA##an##_Stream##sn##_IRQn, 0}
static __dma_bucket_s __dma_streams[__NG_DMA_STREAM_MAX] = {
  __DECL_DMA_BUCKETS(1, 0),
  __DECL_DMA_BUCKETS(1, 1),
  __DECL_DMA_BUCKETS(1, 2),
  __DECL_DMA_BUCKETS(1, 3), 
  __DECL_DMA_BUCKETS(1, 4),
  __DECL_DMA_BUCKETS(1, 5),
  __DECL_DMA_BUCKETS(1, 6),
  __DECL_DMA_BUCKETS(1, 7),
  __DECL_DMA_BUCKETS(2, 0),
  __DECL_DMA_BUCKETS(2, 1),
  __DECL_DMA_BUCKETS(2, 2),
  __DECL_DMA_BUCKETS(2, 3),
  __DECL_DMA_BUCKETS(2, 4),
  __DECL_DMA_BUCKETS(2, 5),
  __DECL_DMA_BUCKETS(2, 6),
  __DECL_DMA_BUCKETS(2, 7),
};

static __dma_handle_s *
__dma_handle_create(hal_dev_param_s *p, __dma_init_s *s,
  __dma_bucket_s *bucket)
{
  __dma_handle_s *h;

  h = (__dma_handle_s *)ng_malloc(sizeof(*h));
  if (h == NULL)
  {
    NGRTOS_ERROR("Failed to alloc DMA_HandleTypeDef.\n");
    goto err0;
  }

  h->h.Instance       = bucket->stream;
  h->h.Init.Channel   = __dma_channel_ids[s->ChannelNumber];
  h->h.Init.Direction = s->Direction;
  h->h.Init.PeriphInc = DMA_PINC_DISABLE;
  h->h.Init.MemInc    = DMA_MINC_ENABLE;
  h->h.Init.Mode      = s->Mode;
  h->h.Init.Priority  = s->Priority;
  h->h.Init.FIFOMode  = s->FIFOMode;
  h->h.Init.FIFOThreshold       = s->FIFOThreshold;
  h->h.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
  h->h.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  h->h.Init.MemBurst            = s->MemBurst;
  h->h.Init.PeriphBurst         = s->PeriphBurst;
  if (HAL_DMA_Init(&h->h) != HAL_OK)
  {
    goto err1;
  }
  
  return h;
  
err1:
  ng__HAL_RCC_DMA_CLK_DISABLE(bucket->dma);
  ng_free(h);
err0:
  return NULL;
}

int __dma_handle_destroy(__dma_handle_s *h)
{
  if (h == NULL)
    return 0;

  HAL_NVIC_DisableIRQ((IRQn_Type)h->v.irq);
  HAL_NVIC_DisableIRQ((IRQn_Type)h->v.irq);
  ng__HAL_RCC_DMA_CLK_DISABLE(h->v.dma);
  
  if (HAL_DMA_DeInit(&h->h) != HAL_OK)
  {
    NGRTOS_ERROR("Failed to destroy DMA handle.\n");
    goto err0;
  }
  ng_free(h);
  return 0;
  
err0:
  return -1;
}

__dma_handle_s *
__dma_handle_create_receiver(void *_p, __dma_init_s *dmarx)
{
  __dma_handle_s *h;
  hal_dev_param_s *p = (hal_dev_param_s *)_p;  
  int bucket = ((dmarx->DmaNumber-1)<<3)+dmarx->StreamNumber;
  __dma_bucket_s *s = &__dma_streams[bucket];
  
  ng__HAL_RCC_DMA_CLK_ENABLE(dmarx->DmaNumber-1);
  __dma_register_IRQ(bucket, p->dma_rxirq);
  HAL_NVIC_SetPriority((IRQn_Type)s->irq, p->dmarxPriority, 0);
  HAL_NVIC_EnableIRQ((IRQn_Type)s->irq);
  
  dmarx->Direction = DMA_PERIPH_TO_MEMORY;
  h = __dma_handle_create(p, dmarx, s);
  if (h == NULL)
    return NULL;
  
  h->v.irq      = bucket;
  h->v.channel  = dmarx->ChannelNumber;
  h->v.bucket   = bucket;
  h->v.dma      = dmarx->DmaNumber-1;
  return h;
}

__dma_handle_s *
__dma_handle_create_sender(void *_p, __dma_init_s *dmatx)
{
  __dma_handle_s *h;
  hal_dev_param_s *p = (hal_dev_param_s *)_p;  
  int bucket = ((dmatx->DmaNumber-1)<<3)+dmatx->StreamNumber;
  __dma_bucket_s *s = &__dma_streams[bucket];

  ng__HAL_RCC_DMA_CLK_ENABLE(dmatx->DmaNumber-1);
  __dma_register_IRQ(bucket, p->dma_txirq);
  HAL_NVIC_SetPriority((IRQn_Type)s->irq, p->dmatxPriority, 0);
  HAL_NVIC_EnableIRQ((IRQn_Type)s->irq);
  
  dmatx->Direction = DMA_MEMORY_TO_PERIPH;
  h = __dma_handle_create(p, dmatx, s);
  if (h == NULL)
    return NULL;
  
  h->v.irq      = bucket;
  h->v.channel  = dmatx->ChannelNumber;
  h->v.bucket   = bucket;
  h->v.dma      = dmatx->DmaNumber-1;
  return h;
}

