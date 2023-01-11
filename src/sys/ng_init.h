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

#ifndef __ngRTOS_INIT_H__
#define __ngRTOS_INIT_H__

#define __NG_PTYPE_FIXED    0
#define __NG_PTYPE_U32      1
#define __NG_PTYPE_STRING   2

struct ng_fix_cfg{
  union{
    struct {
      uint32_t offset:16;
      uint32_t stype:8;
      uint32_t dtype:8;
    }a;
    uint32_t *n;
  }u;
};
typedef struct ng_fix_cfg ng_fix_cfg_s;

/** 
  * @brief  DMA Configuration Structure definition
  */
struct __dma_init
{
  uint32_t DmaNumber:8;
  uint32_t StreamNumber:8;
  uint32_t ChannelNumber:8;
  
  uint32_t Direction;            /*!< Specifies if the data will be transferred from memory to peripheral, 
                                      from memory to memory or from peripheral to memory.
                                      This parameter can be a value of @ref DMA_Data_transfer_direction              */

  uint32_t Mode;                 /*!< Specifies the operation mode of the DMAy Streamx.
                                      This parameter can be a value of @ref DMA_mode
                                      @note The circular buffer mode cannot be used if the memory-to-memory
                                            data transfer is configured on the selected Stream                        */

  uint32_t PeriphDataAlignment;  /*!< Specifies the Peripheral data width.
                                      This parameter can be a value of @ref DMA_Peripheral_data_size                 */
  
  uint32_t MemDataAlignment;     /*!< Specifies the Memory data width.
                                      This parameter can be a value of @ref DMA_Memory_data_size                     */
                                      
  uint32_t Priority;             /*!< Specifies the software priority for the DMAy Streamx.
                                      This parameter can be a value of @ref DMA_Priority_level                       */

  uint32_t FIFOMode;             /*!< Specifies if the FIFO mode or Direct mode will be used for the specified stream.
                                      This parameter can be a value of @ref DMA_FIFO_direct_mode
                                      @note The Direct mode (FIFO mode disabled) cannot be used if the 
                                            memory-to-memory data transfer is configured on the selected stream       */

  uint32_t FIFOThreshold;        /*!< Specifies the FIFO threshold level.
                                      This parameter can be a value of @ref DMA_FIFO_threshold_level                  */

  uint32_t MemBurst;             /*!< Specifies the Burst transfer configuration for the memory transfers. 
                                      It specifies the amount of data to be transferred in a single non interruptible
                                      transaction.
                                      This parameter can be a value of @ref DMA_Memory_burst 
                                      @note The burst mode is possible only if the address Increment mode is enabled. */

  uint32_t PeriphBurst;          /*!< Specifies the Burst transfer configuration for the peripheral transfers. 
                                      It specifies the amount of data to be transferred in a single non interruptible 
                                      transaction. 
                                      This parameter can be a value of @ref DMA_Peripheral_burst
                                      @note The burst mode is possible only if the address Increment mode is enabled. */
};
typedef struct __dma_init __dma_init_s;

#define __NG_DMA_CONFIG_NORMAL                       0
#define __NG_DMA_CONFIG_CIRCULAR                     1
#define __NG_DMA_CONFIG_PFCTRL                       2
#define __NG_DMA_CONFIG_PRIORITY_LOW                 3
#define __NG_DMA_CONFIG_PRIORITY_MEDIUM              4
#define __NG_DMA_CONFIG_PRIORITY_HIGH                5
#define __NG_DMA_CONFIG_PRIORITY_VERY_HIGH           6
#define __NG_DMA_CONFIG_FIFOMODE_DISABLE             7
#define __NG_DMA_CONFIG_FIFOMODE_ENABLE              8
#define __NG_DMA_CONFIG_FIFO_THRESHOLD_1QUARTERFULL  9
#define __NG_DMA_CONFIG_FIFO_THRESHOLD_HALFFULL      10
#define __NG_DMA_CONFIG_FIFO_THRESHOLD_3QUARTERSFULL 11
#define __NG_DMA_CONFIG_FIFO_THRESHOLD_FULL          12
#define __NG_DMA_CONFIG_MBURST_SINGLE                13
#define __NG_DMA_CONFIG_MBURST_INC4                  14
#define __NG_DMA_CONFIG_MBURST_INC8                  15
#define __NG_DMA_CONFIG_MBURST_INC16                 16
#define __NG_DMA_CONFIG_PBURST_SINGLE                17
#define __NG_DMA_CONFIG_PBURST_INC4                  18
#define __NG_DMA_CONFIG_PBURST_INC8                  19
#define __NG_DMA_CONFIG_PBURST_INC16                 20
#define __NG_DMA_CONFIG_PDATAALIGN_BYTE              21
#define __NG_DMA_CONFIG_PDATAALIGN_HALFWORD          22
#define __NG_DMA_CONFIG_PDATAALIGN_WORD              23
#define __NG_DMA_CONFIG_MDATAALIGN_BYTE              24
#define __NG_DMA_CONFIG_MDATAALIGN_HALFWORD          25
#define __NG_DMA_CONFIG_MDATAALIGN_WORD              26
#define __NG_DMA_CONFIG_MAX                          27

void __ng_fix_cfg_conv(uint32_t c, ng_fix_cfg_s *f, char *d);

extern ng_fix_cfg_s dev_config_dma_table[];

#endif
