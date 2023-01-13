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
 *                              
 *                              https://github.com/ngRTOS
 **************************************************************************************
 */

#include "main.h"
#include "ng_channel.h"
#include "ng_rand.h"
#include "ng_rtos.h"
#include "ng_utils.h"

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  ngrtos_assert_failed((const char *)file, line);
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  ng_result_e ret;

  ret = ____ng_SystemInit();
  if (ret != NG_result_ok)
  {
    goto err0;
  }

  ret = ng_mem_init();
  if (ret != NG_result_ok)
  {
    goto err1;
  }

  ret = __ng_init_devices();
  if (ret != NG_result_ok)
  {
    goto err2;
  }

  ret = __ng_init_systime();
  if (ret != NG_result_ok)
  {
    goto err2;
  }
  
  ret = ng_rand_init();
  if (ret != NG_result_ok)
  {
    goto err3;
  }

  ret = ngrtos_scheder_init();
  if (ret != NG_result_ok)
  {
    goto err4;
  }
  
  ret = ng_rtos_init();
  if (ret != NG_result_ok)
  {
    goto err5;
  }
  
  /* Start scheduler */
  ret = ng_scheduler_start();
  if (ret != NG_result_ok)
  {
    goto err6;
  }

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  while (ngrtos_TRUE)
  {
  }
  
err6:
  ng_rtos_deinit();
err5:
  ngrtos_scheder_deinit();
err4:
  ng_rand_deinit();
err3:
  __ng_deinit_devices();
err2:
  ng_mem_destroy();
err1:
  ____ng_SystemClose();
err0:
  return -ret;
}
