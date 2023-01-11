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
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *************************************************************************************
 *                              https://www.ngRTOS.org
 *                              https://github.com/ngRTOS
 **************************************************************************************
 */

/*-----------------------------------------------------------
* Implementation of functions defined in portable.h for the ARM CM4F port.
*----------------------------------------------------------*/

#if !defined( __GNUC__ )
/* IAR includes. */
#include <intrinsics.h>
#endif

/* Scheduler includes. */
#include "ng_defs.h"
#include "ng_func.h"
#include "ng_asm.h"

#if !defined(__ARMVFP__) && !defined(__VFP_FP__)
  #error This port can only be used when the project options are configured to enable hardware floating point support.
#endif


#if ( __ngRTOS_IRQ_PRIORITY_KERNEL_MAX == 0 )
  #error "As described above, it is essential that interrupt service routines that"
         "make use of the RTOS API have a logical priority equal to or below that "
         "set by the __ngRTOS_IRQ_PRIORITY_KERNEL_MAX (lower logical priority "
         "means higher numeric value). CMSIS, and different microcontroller "
         "manufacturers, provide library functions that can be used to set the "
         "priority of an interrupt. Some library functions expect the interrupt "
         "priority to be specified in the least significant bits of an eight bit "
         "bytes, while others expect the interrupt priority to be specified already "
         "shifted to the most significant bits of the eight bit byte. Check the "
         "documentation for the function being called to see which is required in "
         "your case, as getting this wrong can lead to unexpected behaviour."
         "The __ngRTOS_IRQ_PRIORITY_KERNEL_MAX and __ngRTOS_IRQ_PRIORITY_KERNEL_MIN "
         "settings found in FreeRTOSConfig.h require their priority values to be "
         "specified as the ARM Cortex-M core itself wants them - already shifted to "
         "the most significant bits of the byte. That is why "
         "__ngRTOS_IRQ_PRIORITY_KERNEL_MIN, which should be set to the lowest interrupt "
         "priority, is set to 255 (1111 1111 in binary) in the FreeRTOSConfig.h header "
         "files delivered with each official FreeRTOS demo. The values are specified "
         "this way for a number of reasons: The RTOS kernel accesses the ARM Cortex-M3 "
         "hardware directly (without going through any third party library function), "
         "the RTOS kernel implementation pre-dates most library function implementations, "
         "and this was the scheme used by the first ARM Cortex-M3 libraries to come to market.";
#endif

#ifndef configSYSTICK_CLOCK_HZ
  #define configSYSTICK_CLOCK_HZ   configCPU_CLOCK_HZ
  /* Ensure the SysTick is clocked at the same frequency as the core. */
  #define portNVIC_SYSTICK_CLK_BIT  ( 1UL << 2UL )
#else

/* The way the SysTick is clocked is not modified in case it is not the same
 * as the core. */
  #define portNVIC_SYSTICK_CLK_BIT  ( 0 )
#endif

/* Cortex-M specific definitions. */
#ifdef __NVIC_PRIO_BITS
 /* __BVIC_PRIO_BITS will be specified when CMSIS is being used. */
 #define configPRIO_BITS  __NVIC_PRIO_BITS
#else
 #define configPRIO_BITS  4        /* 15 priority levels */
#endif

/* Constants used to detect a Cortex-M7 r0p1 core, which should use the ARM_CM7
 * r0p1 port. */
#define portCPUID                    ( *( ( volatile uint32_t * ) 0xE000ed00 ) )
#define portCORTEX_M7_r0p1_ID        ( 0x410FC271UL )
#define portCORTEX_M7_r0p0_ID        ( 0x410FC270UL )

/* Constants required to check the validity of an interrupt priority. */
#define portFIRST_USER_INTERRUPT_NUMBER    ( 16 )
#define portMAX_8_BIT_VALUE          ( ( uint8_t ) 0xff )
#define portMAX_PRIGROUP_BITS        ( ( uint8_t ) 7 )
#define portPRIORITY_GROUP_MASK      ( SCB_AIRCR_PRIGROUP_Msk )
#define portPRIGROUP_SHIFT           ( SCB_AIRCR_PRIGROUP_Pos )

/* Masks off all bits but the VECTACTIVE bits in the ICSR register. */
#define portVECTACTIVE_MASK         ( SCB_ICSR_VECTACTIVE_Msk )

/* Constants required to set up the initial stack. */
#define portINITIAL_XPSR            ( 0x01000000 )
#define portINITIAL_EXC_RETURN      ( 0xfffffffd )

/* For strict compliance with the Cortex-M spec the task start address should
 * have bit-0 clear, as it is loaded into the PC on exit from an ISR. */
#define portSTART_ADDRESS_MASK      ( ( ngStackTypeT ) 0xfffffffeUL )

/* A fiddle factor to estimate the number of SysTick counts that would have
 * occurred while the SysTick counter is stopped during tickless idle
 * calculations. */
#define portMISSED_COUNTS_FACTOR    ( 45UL )

/*-----------------------------------------------------------*/

/* Each task maintains its own interrupt status in the critical nesting
 * variable. */
static ngAtomTypeT uxCriticalNesting = 0xaaaaaaaa;

/*
 * Used to catch tasks that attempt to return from their implementing function.
 */
static void ng_task_exit_error(void)
{
  ngIrqTypeT x;
  volatile uint32_t ulDummy = 0;

  /* A function that implements a task must not exit or attempt to return to
   * its caller as there is nothing to return to.  If a task wants to exit it
   * should instead call vTaskDelete( NULL ).
   *
   * Artificially force an assert() to be triggered if configASSERT() is
   * defined, then stop here so application writers can catch the error. */
  NGRTOS_ASSERT(uxCriticalNesting == ~0UL );

  x = __ng_sys_disable_irq();
  while( ulDummy == 0 )
  {
    /* This file calls prvTaskExitError() after the scheduler has been
    * started to remove a compiler warning about the function being defined
    * but never called.  ulDummy is used purely to quieten other warnings
    * about code appearing after this function is called - making ulDummy
    * volatile makes the compiler think the function could return and
    * therefore not output an 'unreachable code' warning for code that appears
    * after it. */
  }
  __ng_sys_enable_irq(x);
}
/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
static uint8_t ucMaxSysCallPriority = 0;
static uint32_t ulMaxPRIGROUPValue = 0;
ng_result_e __ng_scheduler_start( void )
{
  /* __ngRTOS_IRQ_PRIORITY_KERNEL_MAX must not be set to 0.
   
   Cortex-M Internal Priority Representation
   Cortex-M hardware details
   The ARM Cortex-M core stores interrupt priority values in the most significant 
   bits of its eight bit interrupt priority registers. For example, if an 
   implementation of a ARM Cortex-M microcontroller only implements three 
   priority bits, then these three bits are shifted up to be bits five, six and 
   seven respectively. Bits zero to four can take any value, although, for future 
   proofing and maximum compatibility, they should be set to one.
   The internal ARM Cortex-M representation is demonstrated in the images below.
   
     
     8-bit register of device implementing 3 priority bits
     ---------------------------------------------------------
     |   n  |   n  |   n  |   x  |   x  |   x  |   x  |   x  |
     ---------------------------------------------------------
     | Bit7 | Bit6 | Bit5 | Bit4 | Bit3 | Bit2 | Bit1 | Bit0 |
     ---------------------------------------------------------
     Cortex-M interrupt priority registers showing the implemented bits
     
   Cortex-M priority registers have space for a maximum of eight priority bits. 
   If, as an example, a microcontroller only implements three bits, then it is 
   the three most significant bits that are used.
   
   
     Priority 5, or 191, in a device that implement 3 priority bit
     ++++++++++++++++++++++-----------------------------------
     |   1  |   0  |   1  |   1  |   1  |   1  |   1  |   1  |
     ++++++++++++++++++++++-----------------------------------
     | Bit7 | Bit6 | Bit5 | Bit4 | Bit3 | Bit2 | Bit1 | Bit0 |
     ++++++++++++++++++++++-----------------------------------
     Storing the value 5 in a ARM Cortex-M core that implements 
     three priority bits
   
   The diagram above shows how the value 5 (binary 101) is stored in a priority 
   register of a microcontroller that implements three priority bits. The diagram 
   demonstrates why the value 5 (binary 0000 0101) can also be considered to be 
   191 (binary 1011 1111) when the three bits are shifted into the required 
   position and the remaining bits are set to 1.
   
     Priority 5, or 191, in a device that implement 4 priority bit
     +++++++++++++++++++++++++++++----------------------------
     |   0  |   1  |   0  |   1  |   1  |   1  |   1  |   1  |
     +++++++++++++++++++++++++++++----------------------------
     | Bit7 | Bit6 | Bit5 | Bit4 | Bit3 | Bit2 | Bit1 | Bit0 |
     +++++++++++++++++++++++++++++----------------------------
     Storing the value 5 in a ARM Cortex-M core that implements four 
     priority bits
   
   The diagram above shows how the value 5 (binary 101) is stored in a priority 
   register of a microcontroller that implements four priority bits. The diagram 
   demonstrates why the value 5 (binary 0000 0101) can also be considered to be 95 
   (binary 0101 1111) when the four bits are shifted into the required position 
   and the remaining bits are set to 1.
   */
  NGRTOS_ASSERT( __ngRTOS_IRQ_PRIORITY_KERNEL_MAX );

  /* This port can be used on all revisions of the Cortex-M7 core other than
   * the r0p1 parts.  r0p1 parts should use the port from the
   * /source/portable/GCC/ARM_CM7/r0p1 directory. */
  NGRTOS_ASSERT( portCPUID != portCORTEX_M7_r0p1_ID );
  NGRTOS_ASSERT( portCPUID != portCORTEX_M7_r0p0_ID );

  #if configENABLE_ASSERT
    {
      volatile uint8_t ulOriginalPriority;
      volatile uint8_t ucMaxPriorityValue;

      /* Determine the maximum priority from which ISR safe ngRTOS API
      * functions can be called.  ISR safe functions are those that end in
      * "FromISR".  ngRTOS maintains separate thread and ISR API functions to
      * ensure interrupt entry is as fast and simple as possible.
      *
      * Save the interrupt priority value that is about to be clobbered. */
      ulOriginalPriority = NVIC->IP[0];

      /* Determine the number of priority bits available.  First write to all
      * possible bits. */
      NVIC->IP[0] = portMAX_8_BIT_VALUE;

      /* Read the value back to see how many bits stuck. */
      ucMaxPriorityValue = NVIC->IP[0];

      /* Use the same mask on the maximum system call priority. */
      ucMaxSysCallPriority = __ngRTOS_IRQ_PRIORITY_KERNEL_MAX & ucMaxPriorityValue;

      /* Calculate the maximum acceptable priority group value for the number
      * of bits read back. */
      ulMaxPRIGROUPValue = portMAX_PRIGROUP_BITS;

      while (ucMaxPriorityValue & (uint8_t)0x80)
      {
        ulMaxPRIGROUPValue--;
        ucMaxPriorityValue <<= (uint8_t) 0x01;
      }

      #ifdef __NVIC_PRIO_BITS
        {
          /* Check the CMSIS configuration that defines the number of
          * priority bits matches the number of priority bits actually queried
          * from the hardware. */
          NGRTOS_ASSERT( ( portMAX_PRIGROUP_BITS - ulMaxPRIGROUPValue ) == __NVIC_PRIO_BITS );
        }
      #endif

      #ifdef configPRIO_BITS
        {
          /* Check the ngRTOS configuration that defines the number of
          * priority bits matches the number of priority bits actually queried
          * from the hardware. */
          NGRTOS_ASSERT( ( portMAX_PRIGROUP_BITS - ulMaxPRIGROUPValue ) == configPRIO_BITS );
        }
      #endif

      /* Shift the priority group value back to its position within the AIRCR
      * register. */
      ulMaxPRIGROUPValue <<= portPRIGROUP_SHIFT;
      ulMaxPRIGROUPValue &= portPRIORITY_GROUP_MASK;

      /* Restore the clobbered interrupt priority register to its original
      * value. */
      NVIC->IP[0] = ulOriginalPriority;
    }
  #endif /* configENABLE_ASSERT */

#define portNVIC_PENDSV_PRI					__ngRTOS_IRQ_PRIORITY_KERNEL_MIN
#define portNVIC_SYSTICK_PRI				__ngRTOS_IRQ_PRIORITY_KERNEL_MIN
  /* Make PendSV and SysTick the lowest priority interrupts. */
  SCB->SHP[8] = portNVIC_SYSTICK_PRI;
  SCB->SHP[9] = portNVIC_PENDSV_PRI;


  /* Initialise the critical nesting count ready for the first task. */
  uxCriticalNesting = 0;

#if configENABLE_FPU
  /* Ensure the VFP is enabled - it should be anyway. */
  __ng_enable_vfp();
  /* Lazy save always. */
  FPU->FPCCR |= FPU_FPCCR_ASPEN_Msk|FPU_FPCCR_LSPEN_Msk;
#endif

  return NG_result_ok;
}

/*-----------------------------------------------------------
   -------------------------
   |  portINITIAL_XPSR     |
   -------------------------
   |           f           |
   -------------------------
   |  ng_task_exit_error   |
   -------------------------
   |                       |
   -------------------------
   |                       |
   -------------------------
   |                       |
   -------------------------
   |                       |
   -------------------------
   |          args         |
   -------------------------
   |   INITIAL_EXC_RETURN  |
   -------------------------
   |                       |
   -------------------------
   |                       |
   -------------------------
   |                       |
   -------------------------
   |                       |
   -------------------------
   |                       |
   -------------------------
   |                       |
   -------------------------
   |                       |
   -------------------------
   |                       |
   ------------------------- SP

 *
 * See header file for description.
 *
 * Simulate the stack frame as it would be created by a context switch
 * interrupt. 
 *
 */
#if __ngRTOS_STACK_DECREASE  
#define _INC_PUT(x) *--sp   = (x)
#define _INC_NUM(n)    sp  -= (n)
#define _XXX_PUT(x)   *sp   = (x)
#else
#define _INC_PUT(x) *sp++   = (x)
#define _INC_NUM(n)  sp    += (n)
#define _XXX_PUT(x) *(sp-1) = (x)
#endif
uint8_t *__ng_stack_init(uint8_t *_sp, ng_task_func_f f, void *args)
{
  ngStackTypeT *sp = (ngStackTypeT *)_sp;
  
  /* Offset added to account for the way the MCU uses the stack on entry/exit
  * of interrupts, and to ensure alignment. */
  _INC_PUT(portINITIAL_XPSR);                          /* xPSR */
  _INC_PUT(((ngStackTypeT)f)&portSTART_ADDRESS_MASK); /* PC */
  _INC_PUT((ngStackTypeT)ng_task_exit_error);          /* LR */

  /* Save code space by skipping register initialisation. */
  _INC_NUM(5);                                         /* R12, R3, R2 and R1. */
  _XXX_PUT((ngStackTypeT)args);                        /* R0 */

  /* A save method is being used that requires each task to maintain its
   * own exec return value. */
  _INC_PUT(portINITIAL_EXC_RETURN);
  _INC_NUM(8);                        /* R11, R10, R9, R8, R7, R6, R5 and R4. */

  return (uint8_t *)sp;
}

/* RTOS ports that support interrupt nesting have the concept of a maximum
system call (or maximum API call) interrupt priority.  Interrupts that are
above the maximum system call priority are kept permanently enabled, even
when the RTOS kernel is in a critical section, but cannot make any calls to
ngRTOS API functions.  If configASSERT() is defined in ng_config.h
then __ng_irq_priority_sanity_check() will result in an assertion
failure if a ngRTOS API function is called from an interrupt that has been
assigned a priority above the configured maximum system call priority.
Only ngRTOS functions that end in FromISR can be called from interrupts
that have been assigned a priority at or (logically) below the maximum
system call interrupt priority.  ngRTOS maintains a separate interrupt
safe API to ensure interrupt entry is as fast and as simple as possible.
More information (albeit Cortex-M specific) is provided on the following
link: 

Cortex-M Internal Priority Representation
Cortex-M hardware details
The ARM Cortex-M core stores interrupt priority values in the most significant 
bits of its eight bit interrupt priority registers. For example, if an 
implementation of a ARM Cortex-M microcontroller only implements three 
priority bits, then these three bits are shifted up to be bits five, six and 
seven respectively. Bits zero to four can take any value, although, for future 
proofing and maximum compatibility, they should be set to one.
The internal ARM Cortex-M representation is demonstrated in the images below.

  
  8-bit register of device implementing 3 priority bits
  ---------------------------------------------------------
  |   n  |   n  |   n  |   x  |   x  |   x  |   x  |   x  |
  ---------------------------------------------------------
  | Bit7 | Bit6 | Bit5 | Bit4 | Bit3 | Bit2 | Bit1 | Bit0 |
  ---------------------------------------------------------
  Cortex-M interrupt priority registers showing the implemented bits
  
Cortex-M priority registers have space for a maximum of eight priority bits. 
If, as an example, a microcontroller only implements three bits, then it is 
the three most significant bits that are used.


  Priority 5, or 191, in a device that implement 3 priority bit
  ++++++++++++++++++++++-----------------------------------
  |   1  |   0  |   1  |   1  |   1  |   1  |   1  |   1  |
  ++++++++++++++++++++++-----------------------------------
  | Bit7 | Bit6 | Bit5 | Bit4 | Bit3 | Bit2 | Bit1 | Bit0 |
  ++++++++++++++++++++++-----------------------------------
  Storing the value 5 in a ARM Cortex-M core that implements 
  three priority bits

The diagram above shows how the value 5 (binary 101) is stored in a priority 
register of a microcontroller that implements three priority bits. The diagram 
demonstrates why the value 5 (binary 0000 0101) can also be considered to be 
191 (binary 1011 1111) when the three bits are shifted into the required 
position and the remaining bits are set to 1.

  Priority 5, or 191, in a device that implement 4 priority bit
  +++++++++++++++++++++++++++++----------------------------
  |   0  |   1  |   0  |   1  |   1  |   1  |   1  |   1  |
  +++++++++++++++++++++++++++++----------------------------
  | Bit7 | Bit6 | Bit5 | Bit4 | Bit3 | Bit2 | Bit1 | Bit0 |
  +++++++++++++++++++++++++++++----------------------------
  Storing the value 5 in a ARM Cortex-M core that implements four 
  priority bits

The diagram above shows how the value 5 (binary 101) is stored in a priority 
register of a microcontroller that implements four priority bits. The diagram 
demonstrates why the value 5 (binary 0000 0101) can also be considered to be 95 
(binary 0101 1111) when the four bits are shifted into the required position 
and the remaining bits are set to 1.

*/
void __ng_irq_priority_sanity_check(void)
{
  ngIrqTypeT curIrq = __ng_get_Irqn();

  /* Is the interrupt number a user defined interrupt? */
  if (curIrq >= portFIRST_USER_INTERRUPT_NUMBER)
  {
    /* Look up the interrupt's priority. */
    uint8_t ucCurrentPriority = (NVIC->IP-16)[curIrq];

    /* The following assertion will fail if a service routine (ISR) for
    an interrupt that has been assigned a priority above
    __ngRTOS_IRQ_PRIORITY_KERNEL_MAX calls an ISR safe ngRTOS API
    function. ISR safe ngRTOS API functions must *only* be called
    from interrupts that have been assigned a priority at or below
    __ngRTOS_IRQ_PRIORITY_KERNEL_MAX.

    Numerically low interrupt priority numbers represent logically high
    interrupt priorities, therefore the priority of the interrupt must
    be set to a value equal to or numerically *higher* than
    __ngRTOS_IRQ_PRIORITY_KERNEL_MAX.

    Interrupts that use the ngRTOS API must not be left at their
    default priority of zero as that is the highest possible priority,
    which is guaranteed to be above __ngRTOS_IRQ_PRIORITY_KERNEL_MAX,
    and therefore also guaranteed to be invalid.

    ngRTOS maintains separate thread and ISR API functions to ensure
    interrupt entry is as fast and simple as possible.

    The following text provide detailed information:
    
    Cortex-M Internal Priority Representation
    Cortex-M hardware details
    The ARM Cortex-M core stores interrupt priority values in the most significant 
    bits of its eight bit interrupt priority registers. For example, if an 
    implementation of a ARM Cortex-M microcontroller only implements three 
    priority bits, then these three bits are shifted up to be bits five, six and 
    seven respectively. Bits zero to four can take any value, although, for future 
    proofing and maximum compatibility, they should be set to one.
    The internal ARM Cortex-M representation is demonstrated in the images below.
    
      
      8-bit register of device implementing 3 priority bits
      ---------------------------------------------------------
      |   n  |   n  |   n  |   x  |   x  |   x  |   x  |   x  |
      ---------------------------------------------------------
      | Bit7 | Bit6 | Bit5 | Bit4 | Bit3 | Bit2 | Bit1 | Bit0 |
      ---------------------------------------------------------
      Cortex-M interrupt priority registers showing the implemented bits
      
    Cortex-M priority registers have space for a maximum of eight priority bits. 
    If, as an example, a microcontroller only implements three bits, then it is 
    the three most significant bits that are used.
    
    
      Priority 5, or 191, in a device that implement 3 priority bit
      ++++++++++++++++++++++-----------------------------------
      |   1  |   0  |   1  |   1  |   1  |   1  |   1  |   1  |
      ++++++++++++++++++++++-----------------------------------
      | Bit7 | Bit6 | Bit5 | Bit4 | Bit3 | Bit2 | Bit1 | Bit0 |
      ++++++++++++++++++++++-----------------------------------
      Storing the value 5 in a ARM Cortex-M core that implements 
      three priority bits
    
    The diagram above shows how the value 5 (binary 101) is stored in a priority 
    register of a microcontroller that implements three priority bits. The diagram 
    demonstrates why the value 5 (binary 0000 0101) can also be considered to be 
    191 (binary 1011 1111) when the three bits are shifted into the required 
    position and the remaining bits are set to 1.
    
      Priority 5, or 191, in a device that implement 4 priority bit
      +++++++++++++++++++++++++++++----------------------------
      |   0  |   1  |   0  |   1  |   1  |   1  |   1  |   1  |
      +++++++++++++++++++++++++++++----------------------------
      | Bit7 | Bit6 | Bit5 | Bit4 | Bit3 | Bit2 | Bit1 | Bit0 |
      +++++++++++++++++++++++++++++----------------------------
      Storing the value 5 in a ARM Cortex-M core that implements four 
      priority bits
    
    The diagram above shows how the value 5 (binary 101) is stored in a priority 
    register of a microcontroller that implements four priority bits. The diagram 
    demonstrates why the value 5 (binary 0000 0101) can also be considered to be 95 
    (binary 0101 1111) when the four bits are shifted into the required position 
    and the remaining bits are set to 1.
    */
    NGRTOS_ASSERT(ucCurrentPriority >= ucMaxSysCallPriority);
  }

  /* Priority grouping:  The interrupt controller (NVIC) allows the bits
  that define each interrupt's priority to be split between bits that
  define the interrupt's pre-emption priority bits and bits that define
  the interrupt's sub-priority.  For simplicity all bits must be defined
  to be pre-emption priority bits.  The following assertion will fail if
  this is not the case (if some bits represent a sub-priority).

  If the application only uses CMSIS libraries for interrupt
  configuration then the correct setting can be achieved on all Cortex-M
  devices by calling NVIC_SetPriorityGrouping( 0 ); before starting the
  scheduler.  Note however that some vendor specific peripheral libraries
  assume a non-zero priority group setting, in which cases using a value
  of zero will result in unpredictable behaviour. */
  NGRTOS_ASSERT((SCB->AIRCR & portPRIORITY_GROUP_MASK) <= ulMaxPRIGROUPValue);
}


