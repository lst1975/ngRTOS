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
/* Includes ------------------------------------------------------------------*/
#include "ng_defs.h"
#include "ng_task.h"
#include "ng_channel.h"
#include "ng_mcopy.h"
#include "mem/ng_mem.h"

#if configENABLE_IDLE_HOOK
void ngSystemIdleHook(void)
{
  /* ngSystemIdleHook() will only be called if 
                configENABLE_IDLE_HOOK 
    is set to 1 in ng_onfig.h. It will be called on each iteration 
    of the idle task. It is essential that code added to this hook 
    function never attempts to block in any way (for example, call 
        ngrtos_sem_wait(), 
        ng_read(), 
        ng_write(),
        ngrtos_suspend(),
        ng_os_delay()
    will block the system. If the application makes use of the 
    ngrtos_task_delete() API function then it is also important 
    that ngSystemIdleHook() is permitted to return to its calling 
    function, because it is the responsibility of the idle task to 
    clean up the dead tasks. 
  */
}
#endif

#if configENABLE_TICK_HOOK
__WEAK void ngrtos_process_epoch(void)
{
}
void ngSystemTickHook(void)
{
  /* This function will be called by each tick interrupt if 
                configENABLE_TICK_HOOK 
    is set to 1 in ng_onfig.h. User code can be added here, but 
    the tick hook is called from an SYSTEM TICK interrupt context, 
    so code must not attempt to block, and only the interrupt 
    safe or non-blocking codes can be used here. 
  */
  ngrtos_process_epoch();
}
#endif

ng_handle_t __output_uart = NULL;

static void ng_task_output_uart(void *arg)
{
  ng_channel_s *ch;
  ng_string_s name = __DFS("uart1/x");
  
  ch = ng_open(&name);
  if (ch == NULL)
  {
    NGRTOS_ERROR("Failed to open device uart1.\n", );
    ngrtos_task_delete(__output_uart);
    NGRTOS_ERROR("Task output_uart was deleted.\n", );
  }

  while (ngrtos_TRUE)
  {
    ng_mbuf_s *m;

#if 0
    m = m_get(M_DONTWAIT, MT_DATA);
    ng_strcpy(mtod(m,caddr_t), "0123456789\r\n");
    m->m_len = 12;
    (void)ng_write(ch, m);
#elif 1
    {
      int r;
      ng_list_s mlist;

      INIT_LIST_HEAD(&mlist);
      r = ng_get(ch, &mlist);
      if (r < 0 || list_empty(&mlist))
      {
        ng_os_delay(10);
        continue;
      }
      while(!list_empty(&mlist))
      {
        ng_mbuf_s *n;
        m = list_first_entry(&mlist,ng_mbuf_s,m_link);
        list_del(&m->m_link);
        m->m_nextpkt = NULL;
        while (m != NULL)
        {
          n = m->m_next;
          m->m_next = NULL;
          ng_put(ch, m);
          m = n;
        }
      }
    }
#elif 0
  m = m_get(M_DONTWAIT, MT_DATA);
  ng_strcpy(mtod(m,caddr_t), "abcdefghijklmnopqrstuvwxyz0123456789\r\n");
  m->m_len = 38;
  (void)ng_put(ch, m);
#endif

  }
}

ng_result_e ng_rtos_init(void)
{
  ng_handle_t th;
  ng_task_init_s _init;

  ng_mzero(&_init, sizeof(_init));
  _init.name      = "output_uart",
  _init.func      = ng_task_output_uart, 
  _init.sp_size   = ngRTOS_MINIMAL_STACK_SIZE<<3,
  _init.priority  = ngRTOS_PRIO_HIGH,
  th = ngrtos_task_create(&_init);
  if (th == NULL)
  {
    goto err0;
  }
  __output_uart = th;
  return NG_result_ok;
  
err0:
  return NG_result_failed;
}

void ng_rtos_deinit(void)
{
  ngrtos_task_delete(__output_uart);
  __output_uart = NULL;
  return;
}

/* USER CODE BEGIN PREPOSTSLEEP */
__ng_WEAK void 
PreSleepProcessing(uint32_t ulExpectedIdleTime)
{
  /* place for user code */
}

__ng_WEAK void 
PostSleepProcessing(uint32_t ulExpectedIdleTime)
{
  /* place for user code */
}
/* USER CODE END PREPOSTSLEEP */

#if 0
/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
SemaphoreHandle_t uartSemaphoreHandle;
void *uartRingBuffer = NULL;
/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 1024+128,
  .priority = (osPriority_t) osPriorityNormal,
};
osThreadId_t uartTaskHandle;
const osThreadAttr_t uartTask_attributes = {
  .name = "uartTask",
  .stack_size = 1024+128,
  .priority = (osPriority_t) osPriorityAboveNormal,
};


/* USER CODE BEGIN 1 */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
__weak void configureTimerForRunTimeStats(void)
{

}

__weak unsigned long getRunTimeCounterValue(void)
{
return 0;
}
/* USER CODE END 1 */

/* USER CODE BEGIN 2 */
void vApplicationIdleHook( void )
{
   /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
   to 1 in FreeRTOSConfig.h. It will be called on each iteration of the idle
   task. It is essential that code added to this hook function never attempts
   to block in any way (for example, call xQueueReceive() with a block time
   specified, or call vTaskDelay()). If the application makes use of the
   vTaskDelete() API function (as this demo application does) then it is also
   important that vApplicationIdleHook() is permitted to return to its calling
   function, because it is the responsibility of the idle task to clean up
   memory allocated by the kernel to any task that has since been deleted. */

   isaac_RNG_Reseed();
}
/* USER CODE END 2 */

/* USER CODE BEGIN 3 */
void vApplicationTickHook( void )
{
   /* This function will be called by each tick interrupt if
   configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h. User code can be
   added here, but the tick hook is called from an interrupt context, so
   code must not attempt to block, and only the interrupt safe FreeRTOS API
   functions can be used (those that end in FromISR()). */
   ngrtos_timer_check();
}
/* USER CODE END 3 */

/* USER CODE BEGIN PREPOSTSLEEP */
__weak void PreSleepProcessing(uint32_t ulExpectedIdleTime)
{
/* place for user code */
}

__weak void PostSleepProcessing(uint32_t ulExpectedIdleTime)
{
/* place for user code */
}
/* USER CODE END PREPOSTSLEEP */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  uartSemaphoreHandle = xSemaphoreCreateBinary();
  uartRingBuffer = rte_ring_init(NULL, 128, RTE_RING_SYNC_ST);
  
  /* definition and creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);
  uartTaskHandle = osThreadNew(StartUartTask, NULL, &uartTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartUartTask(void * argument)
{
  for(;;)
  {
    int n;
    uint8_t b[128];
    n = rte_ring_sc_dequeue_burst_elem(uartRingBuffer, b, sizeof(b), NULL);
    if (n > 0)
    {
      while (HAL_BUSY == HAL_UART_Transmit_DMA(&huart1, b, n)){};
    }
    xSemaphoreTake(uartSemaphoreHandle, 100);
  }
}

ng_timer_t test_timer;
static void rte_timer_cb(ng_timer_t *t)
{
  uint8_t __STR[] = "\r\n#**********************TIMER***********************#\r\n";
  int __LEN = sizeof(__STR)-1;
  uint32_t n=0;
  do {
  n += rte_ring_mp_enqueue_burst_elem(uartRingBuffer, &__STR[n], __LEN-n, NULL);
  } while(n < __LEN);
}

void StartDefaultTask(void * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* init code for USB_HOST */
  MX_USB_HOST_Init();
  ngrtos_timer_init();

  ngrtos_timer_set(&test_timer, 1000, rte_timer_cb, NULL, 1);
  ngrtos_timer_add(&test_timer);
  
  /* Infinite loop */
  for(;;)
  {
    uint32_t n=0;
    do {
      n += rte_ring_mp_enqueue_burst_elem(uartRingBuffer, &__STR[n], __LEN-n, NULL);
    } while(n < __LEN);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
#endif
