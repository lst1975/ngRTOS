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
#ifndef __ngRTOS_UARTX_H__
#define __ngRTOS_UARTX_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ng_init.h"

typedef struct uartx_hal uartx_hal_s;
typedef struct ng_uart_cfg ng_uart_cfg_s;

int uartx_get(ng_channel_s *ch, ng_list_s *list);
int uartx_put(ng_channel_s *ch, ng_mbuf_s *m);
int uartx_read(ng_channel_s *ch, ng_mbuf_s *m);
int uartx_write(ng_channel_s *ch, ng_mbuf_s *m);
ng_result_e uartx_open(ng_channel_s *ch, const char *name, int len);
ng_result_e uartx_close(ng_channel_s *ch);

struct ng_uart_cfg{
  uint32_t fix;
  uint32_t rxbsz;
  uint32_t txbsz;
  char avp[0];
};

extern ng_fix_cfg_s uart_config_table[];

ng_result_e 
uartx_init(ng_devtab_s *dev, ng_uart_cfg_s *cfg);
void uartx_deinit(ng_devtab_s *dev);

struct uartx_hal{
  UART_HandleTypeDef huart;
  __dma_handle_s *dmarx;
  __dma_handle_s *dmatx;
  ng_uart_cfg_s cfg;
  hal_dev_param_s *param;
};
  
/**
  * @brief UART Init Structure definition
  */
struct __uart_init
{
  uint32_t BaudRate;                  /*!< This member configures the UART communication baud rate.
                                           The baud rate is computed using the following formula:
                                           - IntegerDivider = ((PCLKx) / (8 * (OVR8+1) * (huart->Init.BaudRate)))
                                           - FractionalDivider = ((IntegerDivider - ((uint32_t) IntegerDivider)) * 8 * (OVR8+1)) + 0.5
                                           Where OVR8 is the "oversampling by 8 mode" configuration bit in the CR1 register. */

  uint32_t WordLength;                /*!< Specifies the number of data bits transmitted or received in a frame.
                                           This parameter can be a value of @ref UART_Word_Length */

  uint32_t StopBits;                  /*!< Specifies the number of stop bits transmitted.
                                           This parameter can be a value of @ref UART_Stop_Bits */

  uint32_t Parity;                    /*!< Specifies the parity mode.
                                           This parameter can be a value of @ref UART_Parity
                                           @note When parity is enabled, the computed parity is inserted
                                                 at the MSB position of the transmitted data (9th bit when
                                                 the word length is set to 9 data bits; 8th bit when the
                                                 word length is set to 8 data bits). */

  uint32_t Mode;                      /*!< Specifies whether the Receive or Transmit mode is enabled or disabled.
                                           This parameter can be a value of @ref UART_Mode */

  uint32_t HwFlowCtl;                 /*!< Specifies whether the hardware flow control mode is enabled or disabled.
                                           This parameter can be a value of @ref UART_Hardware_Flow_Control */

  uint32_t OverSampling;              /*!< Specifies whether the Over sampling 8 is enabled or disabled, to achieve higher speed (up to fPCLK/8).
                                           This parameter can be a value of @ref UART_Over_Sampling */

  __dma_init_s rx;                                           
  __dma_init_s tx;                                           
};
typedef struct __uart_init __uart_init_s;

#define __NG_UART_CONFIG_BAUDRATE_9600                         0
#define __NG_UART_CONFIG_BAUDRATE_19200                        1
#define __NG_UART_CONFIG_BAUDRATE_38400                        2
#define __NG_UART_CONFIG_BAUDRATE_57600                        3
#define __NG_UART_CONFIG_BAUDRATE_115200                       4
#define __NG_UART_CONFIG_BAUDRATE_230400                       5
#define __NG_UART_CONFIG_BAUDRATE_460800                       6
#define __NG_UART_CONFIG_BAUDRATE_921600                       7
#define __NG_UART_CONFIG_BAUDRATE_CUSTOMER                     8
#define __NG_UART_CONFIG_STOPBITS_1                            9
#define __NG_UART_CONFIG_STOPBITS_2                            10
#define __NG_UART_CONFIG_PARITY_NONE                           11
#define __NG_UART_CONFIG_PARITY_EVEN                           12
#define __NG_UART_CONFIG_PARITY_ODD                            13
#define __NG_UART_CONFIG_HWCONTROL_NONE                        14
#define __NG_UART_CONFIG_HWCONTROL_RTS                         15
#define __NG_UART_CONFIG_HWCONTROL_CTS                         16
#define __NG_UART_CONFIG_HWCONTROL_RTS_CTS                     17
#define __NG_UART_CONFIG_MODE_RX                               18
#define __NG_UART_CONFIG_MODE_TX                               19
#define __NG_UART_CONFIG_MODE_TX_RX                            20
#define __NG_UART_CONFIG_OVERSAMPLING_16                       21
#define __NG_UART_CONFIG_OVERSAMPLING_8                        22
#define __NG_UART_CONFIG_WORDLENGTH_8B                         23
#define __NG_UART_CONFIG_WORDLENGTH_9B                         24
#define __NG_UART_CONFIG_MAX                                   25

#define __NG_DEFAULT_RX_BUF_SIZE 4096
#define __NG_DEFAULT_TX_BUF_SIZE 4096

#ifdef __cplusplus
}
#endif

#endif

