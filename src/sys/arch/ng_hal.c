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
#include "ng_init.h"
#include "ng_uartx.h"
#include "mem/ng_mem.h"

#define __DECL(n) [__NG_UART_CONFIG_##n] = UART_##n
#define __DECN(n,v) [__NG_UART_CONFIG_##n##v] = v
uint32_t hal_uart_config_table[]={
  __DECN(BAUDRATE_,9600),
  __DECN(BAUDRATE_,19200),
  __DECN(BAUDRATE_,38400),
  __DECN(BAUDRATE_,57600),
  __DECN(BAUDRATE_,115200),
  __DECN(BAUDRATE_,230400),
  __DECN(BAUDRATE_,460800),
  __DECN(BAUDRATE_,921600),
  __DECL(STOPBITS_1),
  __DECL(STOPBITS_2),
  __DECL(PARITY_NONE),
  __DECL(PARITY_EVEN),
  __DECL(PARITY_ODD),
  __DECL(HWCONTROL_NONE),
  __DECL(HWCONTROL_RTS),
  __DECL(HWCONTROL_CTS),
  __DECL(HWCONTROL_RTS_CTS),
  __DECL(MODE_RX),
  __DECL(MODE_TX),
  __DECL(MODE_TX_RX),
  __DECL(OVERSAMPLING_16),
  __DECL(OVERSAMPLING_8),
  __DECL(WORDLENGTH_8B),
  __DECL(WORDLENGTH_9B),
};
#undef __DECL
#undef __DECN

#define __DECL(n) [__NG_DMA_CONFIG_##n] = DMA_##n
uint32_t hal_config_dma_table[]={
  __DECL(NORMAL),
  __DECL(CIRCULAR),
  __DECL(PFCTRL),
  __DECL(PRIORITY_LOW),
  __DECL(PRIORITY_MEDIUM),
  __DECL(PRIORITY_HIGH),
  __DECL(PRIORITY_VERY_HIGH),
  __DECL(FIFOMODE_DISABLE),
  __DECL(FIFOMODE_ENABLE),
  __DECL(FIFO_THRESHOLD_1QUARTERFULL),
  __DECL(FIFO_THRESHOLD_HALFFULL),
  __DECL(FIFO_THRESHOLD_3QUARTERSFULL),
  __DECL(FIFO_THRESHOLD_FULL),
  __DECL(MBURST_SINGLE),
  __DECL(MBURST_INC4),
  __DECL(MBURST_INC8),
  __DECL(MBURST_INC16),
  __DECL(PBURST_SINGLE),
  __DECL(PBURST_INC4),
  __DECL(PBURST_INC8),
  __DECL(PBURST_INC16),
  __DECL(PDATAALIGN_BYTE),
  __DECL(PDATAALIGN_HALFWORD),
  __DECL(PDATAALIGN_WORD),
  __DECL(MDATAALIGN_BYTE),
  __DECL(MDATAALIGN_HALFWORD),
  __DECL(MDATAALIGN_WORD),
};
#undef __DECL
#undef __DECN

static void 
HAL_Dev_MspDeInit_DMA(__dma_handle_s **dmatx, 
  __dma_handle_s **dmarx)
{
  if (dmatx != NULL)
  {
    __dma_handle_destroy(*dmatx);
    __dma_unregister_IRQ((*dmatx)->v.channel);
    *dmatx = NULL;
  }
  if (dmarx != NULL)
  {
    __dma_handle_destroy(*dmarx);
    __dma_unregister_IRQ((*dmarx)->v.channel);
    *dmarx = NULL;
  }
}

struct __dmarx_link{
  DMA_HandleTypeDef *hdmarx;
};
typedef struct __dmarx_link dmarx_link_s;
struct __dmatx_link{
  DMA_HandleTypeDef *hdmatx;
};
typedef struct __dmatx_link dmatx_link_s;

static ng_result_e
HAL_Dev_MspInit_DMA(hal_dev_param_s *p, __dma_handle_s **dmatx, 
  __dma_handle_s **dmarx, void *linktx, void *linkrx)
{
  int size;
  __dma_init_s *inirx;
  __dma_init_s *initx;

  /* USARTx DMA Init */
  if (dmatx != NULL && dmarx != NULL)
    size = sizeof(*dmarx)<<1;
  else
    size = sizeof(*dmarx);

  inirx = (__dma_init_s *)ng_malloc(size);
  if (inirx == NULL)
  {
    NGRTOS_ERROR("Failed to malloc __dma_init_s.\n");
    goto err0;
  }
  
  if (dmarx != NULL)
  {
    __ng_fix_cfg_conv(p->dmarx.fix, dev_config_dma_table, (char *)inirx);
    inirx->DmaNumber     = p->dmarx.dma;
    inirx->StreamNumber  = p->dmarx.stream;
    inirx->ChannelNumber = p->dmarx.channel;

    *dmarx = __dma_handle_create_receiver(p, inirx);
    if (*dmarx == NULL)
    {
      NGRTOS_ERROR("Failed to create DMA receiver.\n");
      goto err1;
    }
    __HAL_LINKDMA((dmarx_link_s*)linkrx,hdmarx,(*dmarx)->h);
  }
  
  if (dmatx != NULL)
  {
    initx = inirx+1;
    __ng_fix_cfg_conv(p->dmatx.fix, dev_config_dma_table, (char *)initx);
    initx->DmaNumber     = p->dmatx.dma;
    initx->StreamNumber  = p->dmatx.stream;
    initx->ChannelNumber = p->dmatx.channel;

    *dmatx = __dma_handle_create_sender(p, initx);
    if (*dmatx == NULL)
    {
      NGRTOS_ERROR("Failed to create DMA sender.\n");
      goto err2;
    }
    __HAL_LINKDMA((dmatx_link_s*)linktx, hdmatx, (*dmatx)->h);
  }

  ng_free(inirx);
  return NG_result_ok;
  
err2:
  HAL_Dev_MspDeInit_DMA(dmarx, NULL);
err1:
  ng_free(dmarx);
err0:
  return NG_result_failed;
}

ng_result_e
HAL_Dev_MspInit(hal_dev_param_s *p, __dma_handle_s **dmatx, 
  __dma_handle_s **dmarx, void *linktx, void *linkrx, 
  ng_string_s *name)
{
  int i;
  ng_result_e r;

  p->init_clk();

  for (i=0; p->gpios[i].GPIOx != NULL;i++){
    HAL_GPIO_Init(p->gpios[i].GPIOx, &p->gpios[i].Init);
  }

  r = HAL_Dev_MspInit_DMA(p, dmatx, dmarx, linktx, linkrx);
  if (NG_result_ok != r)
    goto err0;
  
  /* USARTx interrupt Init */
  HAL_NVIC_SetPriority((IRQn_Type)p->IRQn, p->Priority, 0);
  HAL_NVIC_EnableIRQ((IRQn_Type)p->IRQn);

  NGRTOS_EVENT("Device %pB MspInit OK.\n", name);
  return NG_result_ok;

err0:
  NGRTOS_ERROR("Device %pB MspInit Failed.\n", name);
  return r;
  /* USER CODE END USARTx_MspInit 1 */
}

void 
HAL_Dev_MspDeInit(hal_dev_param_s *p, __dma_handle_s **dmatx, 
  __dma_handle_s **dmarx, ng_string_s *name)
{
  int i;
  
  p->deinit_clk();
  
  /**USARTx GPIO Configuration
  PAx      ------> USARTx_TX
  PAx      ------> USARTx_RX
  */
  for (i=0; p->gpios[i].GPIOx != NULL;i++){
    HAL_GPIO_DeInit(p->gpios[i].GPIOx, p->gpios[i].Init.Pin);
  }

  /* USARTx DMA DeInit */
  HAL_Dev_MspDeInit_DMA(dmatx, dmarx);
  
  /* USARTx interrupt Deinit */
  HAL_NVIC_DisableIRQ((IRQn_Type)p->IRQn);

  NGRTOS_DEBUG("Device %pB MspDeInit OK.\n", name);
}


