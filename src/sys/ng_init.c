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
#include "ng_init.h"
#include "ng_hal.h"

void 
__ng_fix_cfg_conv(uint32_t c, ng_fix_cfg_s *f, char *d)
{
  uint32_t *t = f[0].u.n;
  int i = 32 - __CLZ(c);
  while (i--) 
  {
    if (((1UL << i) & c))
    {
      ng_fix_cfg_s *s = &f[i+1];
      NGRTOS_ASSERT(s->u.a.stype == __NG_PTYPE_FIXED);
      switch (s->u.a.dtype)
      {
        case __NG_PTYPE_U32:
          *((uint32_t*)(d+s->u.a.offset)) = t[i];
          break;
        default:
          break;
      }
    }
  };

  return;
}

#define UCDECL(n,o,st,dt) [__NG_DMA_CONFIG_##n+1] = { \
    .u.a.offset = ngrtos_offsetof(__dma_init_s, o), \
    .u.a.stype  = __NG_PTYPE_##st, \
    .u.a.dtype  = __NG_PTYPE_##dt, \
  }
ng_fix_cfg_s dev_config_dma_table[]={
  [0] = { .u.n = hal_config_dma_table },
  UCDECL(NORMAL,                  Mode,               FIXED, U32),
  UCDECL(CIRCULAR,                Mode,               FIXED, U32),
  UCDECL(PFCTRL,                  Mode,               FIXED, U32),
  UCDECL(PRIORITY_LOW,            Priority,           FIXED, U32),
  UCDECL(PRIORITY_MEDIUM,         Priority,           FIXED, U32),
  UCDECL(PRIORITY_HIGH,           Priority,           FIXED, U32),
  UCDECL(PRIORITY_VERY_HIGH,      Priority,           FIXED, U32),
  UCDECL(FIFOMODE_DISABLE,        FIFOMode,           FIXED, U32),
  UCDECL(FIFOMODE_ENABLE,         FIFOMode,           FIXED, U32),
  UCDECL(FIFO_THRESHOLD_1QUARTERFULL,  FIFOThreshold,      FIXED, U32),
  UCDECL(FIFO_THRESHOLD_HALFFULL,      FIFOThreshold,      FIXED, U32),
  UCDECL(FIFO_THRESHOLD_3QUARTERSFULL, FIFOThreshold,      FIXED, U32),
  UCDECL(FIFO_THRESHOLD_FULL,          FIFOThreshold,      FIXED, U32),
  UCDECL(MBURST_SINGLE,           MemBurst,           FIXED, U32),
  UCDECL(MBURST_INC4,             MemBurst,           FIXED, U32),
  UCDECL(MBURST_INC8,             MemBurst,           FIXED, U32),
  UCDECL(MBURST_INC16,            MemBurst,           FIXED, U32),
  UCDECL(PBURST_SINGLE,           PeriphBurst,        FIXED, U32),
  UCDECL(PBURST_INC4,             PeriphBurst,        FIXED, U32),
  UCDECL(PBURST_INC8,             PeriphBurst,        FIXED, U32),
  UCDECL(PBURST_INC16,            PeriphBurst,        FIXED, U32),
  UCDECL(PDATAALIGN_BYTE,         PeriphDataAlignment,     FIXED, U32),
  UCDECL(PDATAALIGN_HALFWORD,     PeriphDataAlignment,     FIXED, U32),
  UCDECL(PDATAALIGN_WORD,         PeriphDataAlignment,     FIXED, U32),
  UCDECL(MDATAALIGN_BYTE,         MemDataAlignment,        FIXED, U32),
  UCDECL(MDATAALIGN_HALFWORD,     MemDataAlignment,        FIXED, U32),
  UCDECL(MDATAALIGN_WORD,         MemDataAlignment,        FIXED, U32),
};
#undef UCDECL
