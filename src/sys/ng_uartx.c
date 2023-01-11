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
#include "ng_uartx.h"
#include "mem/ng_mem.h"
#include "ng_init.h"

#if !defined (USE_HAL_UART_REGISTER_CALLBACKS)
#error "USE_HAL_UART_REGISTER_CALLBACKS not defined in stm32f4xx_hal_conf.h"
#endif
#if (USE_HAL_UART_REGISTER_CALLBACKS == 0U)
#error "USE_HAL_UART_REGISTER_CALLBACKS not enabled in stm32f4xx_hal_conf.h"
#endif

#define UCDECL(n,o,st,dt) [__NG_UART_CONFIG_##n+1] = { \
    .u.a.offset = ngrtos_offsetof(__uart_init_s, o), \
    .u.a.stype  = __NG_PTYPE_##st, \
    .u.a.dtype  = __NG_PTYPE_##dt, \
  }
ng_fix_cfg_s uart_config_table[]={
  [0] = { .u.n = hal_uart_config_table },
  UCDECL(BAUDRATE_9600,     BaudRate,          FIXED, U32),
  UCDECL(BAUDRATE_19200,    BaudRate,          FIXED, U32),
  UCDECL(BAUDRATE_38400,    BaudRate,          FIXED, U32),
  UCDECL(BAUDRATE_57600,    BaudRate,          FIXED, U32),
  UCDECL(BAUDRATE_115200,   BaudRate,          FIXED, U32),
  UCDECL(BAUDRATE_230400,   BaudRate,          FIXED, U32),
  UCDECL(BAUDRATE_460800,   BaudRate,          FIXED, U32),
  UCDECL(BAUDRATE_921600,   BaudRate,          FIXED, U32),
  UCDECL(BAUDRATE_CUSTOMER, BaudRate,          U32,   U32),

  UCDECL(STOPBITS_1,        StopBits,          FIXED, U32),
  UCDECL(STOPBITS_2,        StopBits,          FIXED, U32),
  UCDECL(PARITY_NONE,       Parity,            FIXED, U32),
  UCDECL(PARITY_EVEN,       Parity,            FIXED, U32),
  UCDECL(PARITY_ODD,        Parity,            FIXED, U32),
  UCDECL(HWCONTROL_NONE,    HwFlowCtl,         FIXED, U32),
  UCDECL(HWCONTROL_RTS,     HwFlowCtl,         FIXED, U32),
  UCDECL(HWCONTROL_CTS,     HwFlowCtl,         FIXED, U32),
  UCDECL(HWCONTROL_RTS_CTS, HwFlowCtl,         FIXED, U32),
  UCDECL(MODE_RX,           Mode,              FIXED, U32),
  UCDECL(MODE_TX,           Mode,              FIXED, U32),
  UCDECL(MODE_TX_RX,        Mode,              FIXED, U32),
  UCDECL(OVERSAMPLING_16,   OverSampling,      FIXED, U32),
  UCDECL(OVERSAMPLING_8,    OverSampling,      FIXED, U32),
  UCDECL(WORDLENGTH_8B,     WordLength,        FIXED, U32),
  UCDECL(WORDLENGTH_9B,     WordLength,        FIXED, U32),
};
#undef UCDECL

static void 
HAL_UARTx_TxCpltCallback(UART_HandleTypeDef *huart)
{
  /* Start transfer */
  ngIrqTypeT b;
  ng_mbuf_s *m, *n=NULL;
  uartx_hal_s *u = ngrtos_container_of(huart,uartx_hal_s,huart);
  ng_devtab_s *dp = NG_PRIV2DEV(u);
  /* NOTE: This function should not be modified, when the callback is needed,
       the HAL_UART_TxCpltCallback could be implemented in the user file
   */

  b = __ng_sys_disable_irq();
  m = dp->mbtx.mb_cur;
  if (m->m_next != NULL)
  {
    n = m->m_next;
    m->m_next = NULL;
  }
  dp->mbtx.mb_cur = n;
  __ng_sys_enable_irq(b);
  m_free_one(m);

  if (n == NULL)
    n = ng_dev_get_mbuf_tx(dp);
  if (n != NULL)
    ng_UARTdev_send_mbuf(dp, &u->huart, n);
}

static ng_result_e 
HAL_UARTx_StartRx(UART_HandleTypeDef *huart,
  ng_devtab_s *dp, ng_mbuf_s *_m)
{
  ng_bool_t isLocked;
  ngIrqTypeT b;
  ng_result_e r;
  ng_mbuf_s *m;

  b = __ng_sys_disable_irq();
  isLocked = dp->mbrx.mb_lock;
  __ng_sys_enable_irq(b);
  __DMB();

  if (isLocked)
  {
    return NG_result_lock;
  }
  
  b = __ng_sys_disable_irq();
  if (dp->mbrx.mb_cur != NULL)
  {
    __ng_sys_enable_irq(b);
    return NG_result_ok;
  }
  dp->mbrx.mb_lock = ngrtos_TRUE;
  __ng_sys_enable_irq(b);
  __DMB();
  
  r = NG_result_ok;
  m = _m == NULL ? m_get(M_DONTWAIT, MT_DATA) : _m;
  if (m != NULL)
  {
    b = __ng_sys_disable_irq();
    dp->mbrx.mb_cur = m;
    __ng_sys_enable_irq(b);
    
    do {
      r = ng_UART_Receive_DMA(huart, m);
    } while(r == NG_result_busy && !IS_IRQ());

    if (r != NG_result_ok)
    {
      b = __ng_sys_disable_irq();
      dp->mbrx.mb_cur = NULL;
      dp->mbrx.stat.errors++;
      dp->mbrx.mb_lock = ngrtos_FALSE;
      __ng_sys_enable_irq(b);
      m_free_one(m);
    }
    else
    {
      b = __ng_sys_disable_irq();
      dp->mbrx.mb_lock = ngrtos_FALSE;
      __ng_sys_enable_irq(b);
    }
  }
  else
  {
    b = __ng_sys_disable_irq();
    dp->mbrx.mb_lock = ngrtos_FALSE;
    __ng_sys_enable_irq(b);
  }

  return r;
}

static void 
__HAL_UARTx_RxCpltCallback(UART_HandleTypeDef *huart, 
  uint16_t size, ng_bool_t error)
{
  /* Start transfer */
  ng_mbuf_s *m;
  ngIrqTypeT b;
  uartx_hal_s *u  = ngrtos_container_of(huart,uartx_hal_s,huart);
  ng_devtab_s *dp = NG_PRIV2DEV(u);
  
  b = __ng_sys_disable_irq();
  m = dp->mbrx.mb_cur;
  NGRTOS_ASSERT(dp->mbrx.mb_cur != NULL);
  dp->mbrx.mb_cur = NULL;
  __ng_sys_enable_irq(b);

  m->m_len = size;
  if (!error &&
    ng_dev_add_mbuf_rx(dp, m, u->cfg.rxbsz) == NG_result_ok)
  {
    m = NULL;
  }
  HAL_UARTx_StartRx(huart, dp, m);
}

static void 
HAL_UARTx_RxCpltCallback(UART_HandleTypeDef *huart)
{
  __HAL_UARTx_RxCpltCallback(huart, 
    huart->RxXferSize - huart->RxXferCount, ngrtos_FALSE);
}

/**
  * @brief  Reception Event Callback (Rx event notification called 
            after use of advanced reception service).
  * @param  huart UART handle
  * @param  Size  Number of data available in application reception 
            buffer (indicates a position in
  *               reception buffer until which, data are available)
  * @retval None
  */
static void 
ng_HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, 
   uint16_t Size)
{
  if (huart->RxState == HAL_UART_STATE_BUSY_RX)
    return;
  __HAL_UARTx_RxCpltCallback(huart, Size, ngrtos_FALSE);
}

static void 
HAL_UARTx_MspInit(UART_HandleTypeDef *huart)
{
  uartx_hal_s *u;
  hal_dev_param_s *p;
  ng_devtab_s *dp;

  u = ngrtos_container_of(huart,uartx_hal_s,huart);
  p = u->param;
  dp = NG_PRIV2DEV(u);

  if (NG_result_ok != HAL_Dev_MspInit(
            p, 
            &u->dmatx,
            &u->dmarx, 
            ngRTOS_BIT_TEST(dp->flag,NG_DEV_FLAG_TX) ? &huart->hdmatx : NULL,
            ngRTOS_BIT_TEST(dp->flag,NG_DEV_FLAG_RX) ? &huart->hdmarx : NULL,
            &dp->name))
    goto err0;
  
  return;

err0:
  return;
  /* USER CODE END USARTx_MspInit 1 */
}

static void 
HAL_UARTx_MspDeInit(UART_HandleTypeDef *huart)
{
  uartx_hal_s *u;
  hal_dev_param_s *p;
  ng_devtab_s *dp;
  
  u = ngrtos_container_of(huart,uartx_hal_s,huart);
  p = u->param;
  dp = NG_PRIV2DEV(u);

  HAL_Dev_MspDeInit(
      p, 
      &u->dmatx,
      &u->dmarx, 
      &dp->name);
}

/**
  * @brief  UART error callbacks.
  * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval None
  */
static void 
ng_HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  __HAL_UARTx_RxCpltCallback(huart, 
        huart->RxXferSize - huart->RxXferCount, 
        ngrtos_TRUE);
  /* NOTE: This function should not be modified, when the callback is needed,
           the HAL_UART_ErrorCallback could be implemented in the user file
   */
}

/* uartHandle->Instance==USARTx */
static ng_result_e 
MX_USARTX_UART_Init(UART_HandleTypeDef *huart,
  __uart_init_s *ini, hal_dev_param_s *param)
{
  ng_result_e r;
  
  if (HAL_OK != HAL_UART_RegisterCallback(huart, 
                  HAL_UART_MSPINIT_CB_ID, 
                  HAL_UARTx_MspInit))
  {
    r = NG_result_esys;
    goto err0;
  }
  
  if (HAL_OK != HAL_UART_RegisterCallback(huart, 
                  HAL_UART_MSPDEINIT_CB_ID, 
                  HAL_UARTx_MspDeInit))
  {
    r = NG_result_esys;
    goto err1;
  }
  
  /* USARTx init function */
  huart->Instance          = (USART_TypeDef *)param->Instance;
  huart->Init.BaudRate     = ini->BaudRate;
  huart->Init.WordLength   = ini->WordLength;
  huart->Init.StopBits     = ini->StopBits;
  huart->Init.Parity       = ini->Parity;
  huart->Init.Mode         = ini->Mode;
  huart->Init.HwFlowCtl    = ini->HwFlowCtl;
  huart->Init.OverSampling = ini->OverSampling;
  if (HAL_UART_Init(huart) != HAL_OK)
  {
    r = NG_result_esys;
    goto err2;
  }

  if (HAL_OK != HAL_UART_RegisterCallback(huart, 
                  HAL_UART_TX_COMPLETE_CB_ID, 
                  HAL_UARTx_TxCpltCallback))
  {
    r = NG_result_esys;
    goto err3;
  }
  
  if (HAL_OK != HAL_UART_RegisterCallback(huart, 
                  HAL_UART_RX_COMPLETE_CB_ID, 
                  HAL_UARTx_RxCpltCallback))
  {
    r = NG_result_esys;
    goto err4;
  }

  if (HAL_OK != HAL_UART_RegisterRxEventCallback(huart, 
                  ng_HAL_UARTEx_RxEventCallback))
  {
    r = NG_result_esys;
    goto err5;
  }

  if (HAL_OK != HAL_UART_RegisterCallback(huart, 
                  HAL_UART_ERROR_CB_ID, 
                  ng_HAL_UART_ErrorCallback))
  {
    r = NG_result_esys;
    goto err6;
  }

  return NG_result_ok;

err6:
  HAL_UART_UnRegisterRxEventCallback(huart);
err5:
  HAL_UART_UnRegisterCallback(huart, 
          HAL_UART_RX_COMPLETE_CB_ID);
err4:
  HAL_UART_UnRegisterCallback(huart, 
          HAL_UART_TX_COMPLETE_CB_ID);
err3:
  HAL_UART_DeInit(huart);
err2:
  HAL_UART_UnRegisterCallback(huart, 
          HAL_UART_MSPDEINIT_CB_ID);
err1:
  HAL_UART_UnRegisterCallback(huart, 
          HAL_UART_MSPINIT_CB_ID);
err0:
  return r;
}

static void 
MX_USARTX_UART_DeInit(UART_HandleTypeDef *huart)
{
  HAL_UART_UnRegisterCallback(huart, 
          HAL_UART_ERROR_CB_ID);
  HAL_UART_UnRegisterRxEventCallback(huart);
  HAL_UART_UnRegisterCallback(huart, 
          HAL_UART_RX_COMPLETE_CB_ID);
  HAL_UART_UnRegisterCallback(huart, 
          HAL_UART_TX_COMPLETE_CB_ID);
  HAL_UART_DeInit(huart);
  HAL_UART_UnRegisterCallback(huart, 
          HAL_UART_MSPDEINIT_CB_ID);
  HAL_UART_UnRegisterCallback(huart, 
          HAL_UART_MSPINIT_CB_ID);
}

ng_result_e 
uartx_open(ng_channel_s *ch, const char *name, int len)
{
  if (len < 1)
    return NG_result_inval;

  if (name[0] == '/')
  {
    name++;  
    len--;
  }
  if (len == 2 && name[0] == 'r' && name[1] == 'x')
    ch->type |= NG_CHT_RX;
  else if (len == 2 && name[0] == 't' && name[1] == 'x')
    ch->type |= NG_CHT_TX;
  else if (len == 1 && name[0] == 'x')
    ch->type |= NG_CHT_TX|NG_CHT_RX;
  else
    return NG_result_inval;
  return NG_result_ok;
}

ng_result_e 
uartx_close(ng_channel_s *ch)
{
  NGRTOS_UNUSED(ch);
  return NG_result_ok;
}

int 
uartx_get(ng_channel_s *ch, ng_list_s *list)
{
  ng_result_e r;
  uartx_hal_s *u  = NG_DEV2PRIV(ch->dp, uartx_hal_s);
  ng_devtab_s *dp = NG_PRIV2DEV(u);
  
  ng_dev_get_mbuf_rx(ch->dp, list);

  do {
    r = HAL_UARTx_StartRx(&u->huart, dp, NULL);
  } while (r == NG_result_lock);
  
  if (!list_empty(list))
    return NG_result_ok;
  
  return -r;
}

int 
uartx_put(ng_channel_s *ch, ng_mbuf_s *m)
{
  ng_result_e r       = NG_result_ok;
  ng_devtab_s *dp     = ch->dp;
  uartx_hal_s *u      = NG_DEV2PRIV(dp, uartx_hal_s);

  r = ng_dev_add_mbuf_tx(dp, m, u->cfg.txbsz);
  if (r != NG_result_ok)
  {
    if (r == NG_result_eirq)
    {
      return -ng_UARTdev_send_mbuf(dp, &u->huart, m);
    }
    return -r;
  }

  return 0;
}

int 
uartx_read(ng_channel_s *ch, ng_mbuf_s *m)
{
  uartx_hal_s *u = NG_DEV2PRIV(ch->dp, uartx_hal_s);
  return ng_UART_Receive_SYNC(&u->huart, m);
}

int 
uartx_write(ng_channel_s *ch, ng_mbuf_s *m)
{
  uartx_hal_s *u  = NG_DEV2PRIV(ch->dp, uartx_hal_s);
  return ng_UART_Transmit_SYNC(&u->huart, m);
}

ng_result_e 
uartx_init(ng_devtab_s *dev, ng_uart_cfg_s *cfg)
{
  ng_result_e r;
  __uart_init_s *ini;
  uartx_hal_s *u = NG_DEV2PRIV(dev, uartx_hal_s);

  u->dmarx = u->dmatx = NULL;
  u->cfg = *cfg;
  ini = (__uart_init_s *)ng_malloc(sizeof(*ini));
  if (ini == NULL)
  {
    NGRTOS_ERROR("Failed to malloc __uart_init_s.\n");
    r = NG_result_nmem;
    goto err0;
  }
  
  __ng_fix_cfg_conv(u->cfg.fix, uart_config_table, (char *)ini);
  if (u->cfg.fix & (1UL<<__NG_UART_CONFIG_MODE_RX)
    || u->cfg.fix & (1UL<<__NG_UART_CONFIG_MODE_TX_RX))
  {
    dev->flag |= NG_DEV_FLAG_RX;
  }
  if (u->cfg.fix & (1UL<<__NG_UART_CONFIG_MODE_TX)
    || u->cfg.fix & (1UL<<__NG_UART_CONFIG_MODE_TX_RX))
  {
    dev->flag |= NG_DEV_FLAG_TX;
  }
  
  r = MX_USARTX_UART_Init(&u->huart, ini, u->param);
  if (r != NG_result_ok)
  {
    NGRTOS_ERROR("Failed to init device %s.\n", dev->name);
    goto err1;
  }

  ng_free(ini);
  return NG_result_ok;

err1:  
  ng_free(ini);
err0:
  return r;
}

void 
uartx_deinit(ng_devtab_s *dev)
{
  uartx_hal_s *u = NG_DEV2PRIV(dev, uartx_hal_s);
  MX_USARTX_UART_DeInit(&u->huart);
}
