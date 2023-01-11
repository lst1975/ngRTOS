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

#include "ng_defs.h"
#include "ng_gpio.h"

static uint8_t __ng_gpios[__NG_GPIO_MAX]={
  0, 0, 0, 0, 0, 0, 0, 0, 0
};

static void __ng_gpio_enable(uint8_t type)
{
  switch (type)
  {
    case __NG_GPIOA:
      __HAL_RCC_GPIOA_CLK_ENABLE();
      break;
    case __NG_GPIOB:
      __HAL_RCC_GPIOB_CLK_ENABLE();
      break;
    case __NG_GPIOC:
      __HAL_RCC_GPIOC_CLK_ENABLE();
      break;
    case __NG_GPIOD:
      __HAL_RCC_GPIOD_CLK_ENABLE();
      break;
    case __NG_GPIOE:
      __HAL_RCC_GPIOE_CLK_ENABLE();
      break;
    case __NG_GPIOF:
      __HAL_RCC_GPIOF_CLK_ENABLE();
      break;
    case __NG_GPIOG:
      __HAL_RCC_GPIOG_CLK_ENABLE();
      break;
    case __NG_GPIOH:
      __HAL_RCC_GPIOH_CLK_ENABLE();
      break;
    case __NG_GPIOI:
      __HAL_RCC_GPIOI_CLK_ENABLE();
      break;
    default:
      NGRTOS_ASSERT(ngrtos_FALSE);
      break;
  }
}

static void __ng_gpio_disable(uint8_t type)
{
  switch (type)
  {
    case __NG_GPIOA:
      __HAL_RCC_GPIOA_CLK_DISABLE();
      break;
    case __NG_GPIOB:
      __HAL_RCC_GPIOB_CLK_DISABLE();
      break;
    case __NG_GPIOC:
      __HAL_RCC_GPIOC_CLK_DISABLE();
      break;
    case __NG_GPIOD:
      __HAL_RCC_GPIOD_CLK_DISABLE();
      break;
    case __NG_GPIOE:
      __HAL_RCC_GPIOE_CLK_DISABLE();
      break;
    case __NG_GPIOF:
      __HAL_RCC_GPIOF_CLK_DISABLE();
      break;
    case __NG_GPIOG:
      __HAL_RCC_GPIOG_CLK_DISABLE();
      break;
    case __NG_GPIOH:
      __HAL_RCC_GPIOH_CLK_DISABLE();
      break;
    case __NG_GPIOI:
      __HAL_RCC_GPIOI_CLK_DISABLE();
      break;
    default:
      NGRTOS_ASSERT(ngrtos_FALSE);
      break;
  }
}

void __ng_gpio_manage(uint8_t type, uint8_t op)
{
  ngIrqTypeT b;
  NGRTOS_ASSERT(type < __NG_GPIO_MAX);
  
  b = __ng_sys_disable_irq();

  switch (op)
  {
    case __NG_GPIO_ENABLE:
      if (__ng_gpios[type]++)
      {
        __ng_sys_enable_irq(b);
        return;
      }
      __ng_gpio_enable(type);
      break;
    case __NG_GPIO_DISABLE:
      if (--__ng_gpios[type])
      {
        __ng_sys_enable_irq(b);
        return;
      }
      __ng_gpio_disable(type);
      break;
    default:
      __ng_sys_enable_irq(b);
      NGRTOS_ASSERT(ngrtos_FALSE);
      break;
  }
  
  __ng_sys_enable_irq(b);
}
