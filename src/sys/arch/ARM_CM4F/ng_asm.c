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
#include "cmsis_compiler.h"
#include "ng_config.h"

void __ng_context_save(void *sp)
{
  /* Get the location of the current TCB. */
  __ASM volatile ("mov r0, %0" :: "r"(sp));
  __ASM volatile (
    "isb \n"
    /* Save the core registers. */
    "stmia r0!, {r4-r11, r14} \n"
    "mrs r0, psp \n"
  );
}

void __ng_context_restore(void *sp)
{
  __ASM volatile ("mov r0, %0" :: "r"(sp));
  __ASM volatile (
    /* Pop the core registers. */
    "ldmia r0!, {r4-r11, r14} \n"
    "msr psp, r0 \n"
    "isb \n"
    "bx r14 \n"
  );
}

void __ng_start_first_task(void)
{
  /* Use the NVIC offset register to locate the stack. */
  __ASM volatile (
    "ldr r0, =0xE000ED08 \n"
    "ldr r0, [r0] \n"
    "ldr r0, [r0] \n"
    
    /* Set the msp back to the start of the stack. */
    "msr msp, r0 \n"
    /* Clear the bit that indicates the FPU is in use in case the FPU was used
       before the scheduler was started - which would otherwise result in the
       unnecessary leaving of space in the SVC stack for lazy saving of FPU
       registers. 
     */
    "mov r0, #0 \n"
    "msr control, r0 \n"
    
    /* Call SVC to start the first task. */
    "cpsie i \n"
    "cpsie f \n"
    "dsb \n"
    "isb \n"
    "svc 0 \n"
  );
}

#if configENABLE_FPU
void __ng_enable_vfp(void)
{
  /* The FPU enable bits are in the CPACR. */
  __ASM volatile (
    "ldr.w r0, =0xE000ED88 \n"
    "ldr r1, [r0] \n"

    /* Enable CP10 and CP11 coprocessors, then save back. */
    "orr r1, r1, #( 0xf << 20 ) \n"
    "str r1, [r0] \n"
    "bx  r14 \n"
  );
}

#endif
