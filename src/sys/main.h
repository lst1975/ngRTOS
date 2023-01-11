/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "ng_arch.h"
#include "ng_defs.h"
#include "ng_system.h"
#include "mem/ng_mem.h"
#include "ng_task.h"

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

extern ng_result_e __ng_init_devices(void);
extern ng_result_e __ng_deinit_devices(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
