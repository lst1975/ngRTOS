/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
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
