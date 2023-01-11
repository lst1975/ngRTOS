#ifndef __ngRTOS_GPIO_H__
#define __ngRTOS_GPIO_H__

#include "stm32f4xx_hal.h"

#define __NG_GPIOA 0
#define __NG_GPIOB 1
#define __NG_GPIOC 2
#define __NG_GPIOD 3
#define __NG_GPIOE 4
#define __NG_GPIOF 5
#define __NG_GPIOG 6
#define __NG_GPIOH 7
#define __NG_GPIOI 8
#define __NG_GPIO_MAX 9

#define __NG_GPIO_ENABLE  0
#define __NG_GPIO_DISABLE 1

void __ng_gpio_manage(uint8_t type, uint8_t op);

#endif /* __ngRTOS_HAL_H__ */
