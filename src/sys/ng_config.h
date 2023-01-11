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

#ifndef __ngRTOS_CONFIG_H__
#define __ngRTOS_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Kernel definitions. */
#define ngRTOS_TICK_RATE_HZ                       (1000UL)
#define ngRTOS_MINIMAL_STACK_SIZE                 (128UL)
#define ngRTOS_TOTAL_HEAP_SIZE                    (96<<10)
#define ngRTOS_TASK_NAME_IDLE                     "IDLE"
#define ngRTOS_TASK_NAME_TIMER                    "TIMER"

#define ngRTOS_PRIO_REALTIME                      (0)
#define ngRTOS_PRIO_HIGHEST                       (1)
#define ngRTOS_PRIO_HIGHER                        (2)
#define ngRTOS_PRIO_HIGH                          (3)
#define ngRTOS_PRIO_NORMAL                        (4)
#define ngRTOS_PRIO_LOW                           (5)
#define ngRTOS_PRIO_LOWER                         (6)
#define ngRTOS_PRIO_LOWEST                        (7)
#define ngRTOS_PRIO_MAX                           (8)
#define ngRTOS_PRIO_MASK                          (7)

#define configENABLE_ERRNO                          1
#define configENABLE_ASSERT                         1
#define configENABLE_FPU                            1
#define configENABLE_MPU                            0
#define configENABLE_PREEMPTION                     1
#define configENABLE_RUN_TIME_STATS                 1
#define configENABLE_16_BIT_TICKS                   0
#define configENABLE_MUTEXES                        1
#define configENABLE_RECURSIVE_MUTEXES              1
#define configENABLE_TICKLESS_IDLE                  1
#define configENABLE_RECORD_STACK_HIGH_ADDRESS      1
#define configENABLE_TASK_STATS                     1
#define configENABLE_PREEMPTION                     1
#define configENABLE_TIME_SLICING                   1
#define configENABLE_IDLE_HOOK                      1
#define configENABLE_TICK_HOOK                      1
#define configEnable_VSNPRINTF                      1
/* Co-routine definitions. */
#define configEnable_COROUTINE                      1
#define configEnable_DIY_MBUF                       1
#define configEnable_IPV6                           1
#define configEnable_MemPool                        1
#define configEnable_Random                         1

#define configEnable_TIMER_TINY                     0
#define configEnable_TIMER_WHEEL                    0
#define configEnable_TIMER_SKIP                     1

#if configEnable_TIMER_TINY
#define configEnable_TIMER_WHEEL 0
#define configEnable_TIMER_SKIP  0
#endif
#if configEnable_TIMER_SKIP
#define MAX_SKIPLIST_DEPTH 10
#define configEnable_Random 1
#endif

#if configEnable_COROUTINE
#define configDefaultNUM_CO_ROUTINE                (4)
#endif

#define configENABLE_SPEED_FIRST                    1
#define configENABLE_SMALL_KERNEL                   0

#define __configUseBase64Decode  1
#define __configUseBase64Encode  1
#define __configUseGnssCrcVerify 0

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */
#define INCLUDE_vTaskPrioritySet             1
#define INCLUDE_uxTaskPriorityGet            1
#define INCLUDE_vTaskDelete                  1
#define INCLUDE_vTaskCleanUpResources        0
#define INCLUDE_vTaskSuspend                 1
#define INCLUDE_vTaskDelayUntil              1
#define INCLUDE_vTaskDelay                   1
#define INCLUDE_xTaskGetSchedulerState       1
#define INCLUDE_xTimerPendFunctionCall       1
#define INCLUDE_xQueueGetMutexHolder         1
#define INCLUDE_uxTaskGetStackHighWaterMark  1
#define INCLUDE_xTaskGetCurrentTaskHandle    1
#define INCLUDE_eTaskGetState                1

/* Cortex-M specific definitions. */
#ifdef __NVIC_PRIO_BITS
 /* __BVIC_PRIO_BITS will be specified when CMSIS is being used. */
 #define __ngRTOS_IRQ_PRIO_BITS         __NVIC_PRIO_BITS
#else
 #define __ngRTOS_IRQ_PRIO_BITS         4
#endif
    
/* The lowest interrupt priority that can be used in a call to a "set priority"
   function. */
#define __ngRTOS_IRQ_PRIORITY_MIN   ((1<<(__ngRTOS_IRQ_PRIO_BITS))-1)
    
/* The highest interrupt priority that can be used by any interrupt service
  routine that makes calls to interrupt safe FreeRTOS API functions.  

  #########################################################################
  DO NOT CALL INTERRUPT SAFE ngRTOS API FUNCTIONS FROM ANY INTERRUPT 
  THAT HAS A HIGHER PRIORITY THAN THIS! 
  #########################################################################
  
  (higher priorities are lower numeric values. */
#define __ngRTOS_IRQ_PRIORITY_MAX 5

/*     
  The next thing to know is that, in ARM Cortex-M cores, numerically low priority 
  values are used to specify logically high interrupt priorities. For example, the 
  logical priority of an interrupt assigned a numeric priority value of 2 is above 
  that of an interrupt assigned a numeric priority of 5. In other words, interrupt 
  priority 2 is higher than interrupt priority 5, even though the number 2 is lower 
  than the number 5. To hopefully make that clear: An interrupt assigned a numeric 
  priority of 2 can interrupt (nest with) an interrupt assigned a numeric priority 
  of 5, but an interrupt assigned a numeric priority of 5 cannot interrupt an 
  interrupt assigned a numeric priority of 2. This is the most counterintuitive 
  aspect of ARM Cortex-M interrupt priorities because it is the opposite to most 
  non ARM Cortex-M3 microcontroller architectures.

  Therefore, any interrupt service routine that uses an RTOS API function must have 
  its priority manually set to a value that is numerically equal to or greater than 
  the configMAX_SYSCALL_INTERRUPT_PRIORITY setting. This ensures the interrupt's 
  logical priority is equal to or less than the __ngRTOS_IRQ_PRIORITY_KERNEL_MAX 
  setting. Cortex-M interrupts default to having a priority value of zero. Zero is 
  the highest possible priority value. Therefore, never leave the priority of an 
  interrupt that uses the interrupt safe RTOS API at its default value.
  
  As described above, it is essential that interrupt service routines that
  make use of the RTOS API have a logical priority equal to or below that 
  set by the __ngRTOS_IRQ_PRIORITY_KERNEL_MAX (lower logical priority 
  means higher numeric value). CMSIS, and different microcontroller 
  manufacturers, provide library functions that can be used to set the 
  priority of an interrupt. Some library functions expect the interrupt 
  priority to be specified in the least significant bits of an eight bit 
  bytes, while others expect the interrupt priority to be specified already 
  shifted to the most significant bits of the eight bit byte. Check the 
  documentation for the function being called to see which is required in 
  your case, as getting this wrong can lead to unexpected behaviour.
  The __ngRTOS_IRQ_PRIORITY_KERNEL_MAX and __ngRTOS_IRQ_PRIORITY_KERNEL_MIN 
  settings found in FreeRTOSConfig.h require their priority values to be 
  specified as the ARM Cortex-M core itself wants them - already shifted to 
  the most significant bits of the byte. That is why 
  __ngRTOS_IRQ_PRIORITY_KERNEL_MIN, which should be set to the lowest interrupt 
  priority, is set to 255 (1111 1111 in binary) in the FreeRTOSConfig.h header 
  files delivered with each official FreeRTOS demo. The values are specified 
  this way for a number of reasons: The RTOS kernel accesses the ARM Cortex-M3 
  hardware directly (without going through any third party library function), 
  the RTOS kernel implementation pre-dates most library function implementations, 
  and this was the scheme used by the first ARM Cortex-M3 libraries to come to market.;

Interrupt priorities used by the kernel port layer itself.  These are generic
to all Cortex-M ports, and do not rely on any particular library functions. */
#define __ngRTOS_IRQ_PRIORITY_KERNEL_MIN 	(__ngRTOS_IRQ_PRIORITY_MIN << (8 - __ngRTOS_IRQ_PRIO_BITS) )
/* !!!! __ngRTOS_IRQ_PRIORITY_KERNEL_MAX must not be set to zero !!!! */
#define __ngRTOS_IRQ_PRIORITY_KERNEL_MAX 	(__ngRTOS_IRQ_PRIORITY_MAX << (8 - __ngRTOS_IRQ_PRIO_BITS) )

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_CONFIG_H */
