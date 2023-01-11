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
#ifndef __ngRTOS_HAL_H__
#define __ngRTOS_HAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

#include "ng_mbuf.h"
#include "ng_init.h"

#define NG_PRIORITY_VERYLOW_DMA_RXIRQ     14
#define NG_PRIORITY_VERYLOW_DMA_TXIRQ     14
#define NG_PRIORITY_VERYLOW_IRQ           14
#define NG_PRIORITY_LOW_DMA_RXIRQ         13
#define NG_PRIORITY_LOW_DMA_TXIRQ         13
#define NG_PRIORITY_LOW_IRQ               13
#define NG_PRIORITY_ABOVELOW_DMA_RXIRQ    12
#define NG_PRIORITY_ABOVELOW_DMA_TXIRQ    12
#define NG_PRIORITY_ABOVELOW_IRQ          12
#define NG_PRIORITY_BELOWNORMAL_DMA_RXIRQ 11
#define NG_PRIORITY_BELOWNORMAL_DMA_TXIRQ 11
#define NG_PRIORITY_BELOWNORMAL_IRQ       11
#define NG_PRIORITY_NORMAL_DMA_RXIRQ      10
#define NG_PRIORITY_NORMAL_DMA_TXIRQ      10
#define NG_PRIORITY_NORMAL_IRQ            10
#define NG_PRIORITY_ABOVENORMAL_DMA_RXIRQ 9
#define NG_PRIORITY_ABOVENORMAL_DMA_TXIRQ 9
#define NG_PRIORITY_ABOVENORMAL_IRQ       9
#define NG_PRIORITY_MIDDLE_DMA_RXIRQ      8
#define NG_PRIORITY_MIDDLE_DMA_TXIRQ      8
#define NG_PRIORITY_MIDDLE_IRQ            8
#define NG_PRIORITY_HIGH_DMA_RXIRQ        7
#define NG_PRIORITY_HIGH_DMA_TXIRQ        7
#define NG_PRIORITY_HIGH_IRQ              7
#define NG_PRIORITY_VERYHIGH_DMA_RXIRQ    6
#define NG_PRIORITY_VERYHIGH_DMA_TXIRQ    6
#define NG_PRIORITY_VERYHIGH_IRQ          6
#define NG_PRIORITY_REALTIME_DMA_RXIRQ    5
#define NG_PRIORITY_REALTIME_DMA_TXIRQ    5
#define NG_PRIORITY_REALTIME_IRQ          5

struct ng_dma_cfg{
  uint32_t fix;
  uint8_t dma;
  uint8_t stream;
  uint8_t channel;
  uint8_t pad;
  char avp[0];
};
typedef struct ng_dma_cfg ng_dma_cfg_s;

struct ng_gpio {
  GPIO_InitTypeDef Init;
  GPIO_TypeDef  *GPIOx;
};
typedef struct ng_gpio ng_gpio_s;

struct hal_dev_param {
  ng_gpio_s *gpios;

  uint32_t IRQn:8;
  uint32_t Priority:8;
  uint32_t dmatxPriority:8;
  uint32_t dmarxPriority:8;

  void (*init_clk)(void);
  void (*deinit_clk)(void);
  void (*dma_rxirq)(void);
  void (*dma_txirq)(void);

  ng_dma_cfg_s dmarx;
  ng_dma_cfg_s dmatx;

  void *Instance;        /*!< DEV registers base address        */
};
typedef struct hal_dev_param hal_dev_param_s;

struct __dma_var{
  uint32_t irq:8;
  uint32_t channel:8;
  uint32_t bucket:8;
  uint32_t dma:8;
};
typedef struct __dma_var __dma_var_s;

struct __dma_handle{
  DMA_HandleTypeDef h;
  __dma_var_s v;
};

typedef struct __dma_handle __dma_handle_s;

typedef void (*__dma_cb_f)(void);

__dma_handle_s *__dma_handle_create_receiver(void *p, __dma_init_s *dmarx);
__dma_handle_s *__dma_handle_create_sender(void *p, __dma_init_s *dmarx);
int __dma_handle_destroy(__dma_handle_s *h);

ng_result_e HAL_Dev_MspInit(hal_dev_param_s *p, __dma_handle_s **dmatx, 
  __dma_handle_s **dmarx, void *linktx, void *linkrx, ng_string_s *name);
void HAL_Dev_MspDeInit(hal_dev_param_s *p, __dma_handle_s **dmatx, 
  __dma_handle_s **dmarx, ng_string_s *name);

void __dma_register_IRQ(uint32_t ch, __dma_cb_f cb);
void __dma_unregister_IRQ(uint32_t ch);

extern uint32_t hal_uart_config_table[];
extern uint32_t hal_config_dma_table[];

__STATIC_FORCEINLINE int
ng_UART_Transmit_SYNC(UART_HandleTypeDef *huart, 
  ng_mbuf_s *m) 
{
  int len = 0;
  
  while(m != NULL)
  {
    ng_mbuf_s *n;
    HAL_StatusTypeDef r;

    n = m->m_next;
    do {
      r = HAL_UART_Transmit(huart, mtod(m, uint8_t *), 
              m->m_len, HAL_MAX_DELAY);
    } 
    while(r == HAL_BUSY);
    
    if (r != HAL_OK)
    {
      if (r == HAL_TIMEOUT)
        return -NG_result_timeout;
      else if (r == HAL_BUSY)
        return -NG_result_busy;
      else
        return -NG_result_esys;
    }
    len += m->m_len;
    m->m_next = NULL;
    m_free_one(m);
    m = n;
  }

  return len;
}

__STATIC_FORCEINLINE int
ng_UART_Receive_SYNC(UART_HandleTypeDef *huart, 
  ng_mbuf_s *m) 
{
  HAL_StatusTypeDef r;
  r = HAL_UART_Receive(huart, mtod(m, uint8_t *),
        MLEN, HAL_MAX_DELAY);
  if (HAL_OK != r)
    return -NG_result_esys;
  return MLEN - __HAL_DMA_GET_COUNTER(huart->hdmarx);
}

__STATIC_FORCEINLINE ng_result_e
ng_UART_Transmit_DMA(UART_HandleTypeDef *huart, 
  ng_mbuf_s *m) 
{
  HAL_StatusTypeDef r;
  
  r = HAL_UART_Transmit_DMA(huart, mtod(m, uint8_t *), 
        m->m_len);
  if (HAL_OK != r)
  {
    if (r == HAL_BUSY)
      return NG_result_busy;
    else
      return NG_result_esys;
  }
  else
  {
    return NG_result_ok;
  }
}

__STATIC_FORCEINLINE ng_result_e
ng_UART_Receive_DMA(UART_HandleTypeDef *huart, 
  ng_mbuf_s *m) 
{
  HAL_StatusTypeDef r;

  r = HAL_UARTEx_ReceiveToIdle_DMA(huart, 
           mtod(m, uint8_t *), MLEN);
  if (HAL_OK != r)
  {
    if (r == HAL_BUSY)
      return NG_result_busy;
    else
      return NG_result_esys;
  }
  else
  {
    return NG_result_ok;
  }
}

static inline ng_result_e
ng_UARTdev_send_mbuf(ng_devtab_s *dp, 
  UART_HandleTypeDef *huart, ng_mbuf_s *m)
{
  ngIrqTypeT b;
  ng_result_e r;

  b = __ng_sys_disable_irq();
  dp->mbtx.mb_cur = m;
  __ng_sys_enable_irq(b);

  do {
    r = ng_UART_Transmit_DMA(huart, m);
  } while(r == NG_result_busy && !IS_IRQ());
  
  if (r != NG_result_ok)
  {
    b = __ng_sys_disable_irq();
    dp->mbtx.stat.errors++;
    dp->mbtx.mb_cur = NULL;
    __ng_sys_enable_irq(b);
    m_free_one(m);
    return r;
  }
  else
  {
    return NG_result_ok;
  }
}

#ifdef __cplusplus
}
#endif

#endif /* __ngRTOS_HAL_H__ */
