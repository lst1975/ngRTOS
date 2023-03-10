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

#ifndef __NGRTOS_FUNC_H__
#define __NGRTOS_FUNC_H__

#ifdef __cplusplus
    extern "C" {
#endif

/*-----------------------------------------------------------*/

#if !defined( __GNUC__ )
/* Suppress warnings that are generated by the IAR tools, but cannot be fixed in
 * the source code because to do so would cause other compilers to generate
 * warnings. */
    #pragma diag_suppress=Pe191
    #pragma diag_suppress=Pa082
#endif

/*---------------------------------------------------------------------------*/
#ifndef __ARM_ARCH_6M__
  #define __ARM_ARCH_6M__         0
#endif
#ifndef __ARM_ARCH_7M__
  #define __ARM_ARCH_7M__         0
#endif
#ifndef __ARM_ARCH_7EM__
  #define __ARM_ARCH_7EM__        0
#endif
#ifndef __ARM_ARCH_8M_MAIN__
  #define __ARM_ARCH_8M_MAIN__    0
#endif
#ifndef __ARM_ARCH_7A__
  #define __ARM_ARCH_7A__         0
#endif
    
#if ((__ARM_ARCH_7M__   == 1U) || (__ARM_ARCH_7EM__     == 1U) || (__ARM_ARCH_8M_MAIN__ == 1U))
  #define IS_IRQ_MASKED()         ((__get_PRIMASK() != 0U) || (__get_BASEPRI() != 0U))
#elif  (__ARM_ARCH_6M__ == 1U)
  #define IS_IRQ_MASKED()         (__get_PRIMASK() != 0U)
#elif (__ARM_ARCH_7A__  == 1U)
      /* CPSR mask bits */
  #define CPSR_MASKBIT_I          0x80U
  #define IS_IRQ_MASKED()         ((__get_CPSR() & CPSR_MASKBIT_I) != 0U)
#else
  #define IS_IRQ_MASKED()         (__get_PRIMASK() != 0U)
#endif
    
#if (__ARM_ARCH_7A__      == 1U)
    /* CPSR mode bitmasks */
  #define CPSR_MODE_USER          0x10U
  #define CPSR_MODE_SYSTEM        0x1FU
  #define IS_IRQ_MODE()           ((__get_mode() != CPSR_MODE_USER) && (__get_mode() != CPSR_MODE_SYSTEM))
#else
  #define IS_IRQ_MODE()           (__get_IPSR() != 0U)
#endif
    
#define IS_IRQ()                  IS_IRQ_MODE()

/* SVCall_IRQ_NBR added as SV_Call handler name is not the same for CM0 and for all other CMx */
#define SVCall_IRQ_NBR            (IRQn_Type) -5	

/**
  \brief   Get IPSR Register
  \details Returns the content of the IPSR Register.
  \return               IPSR Register value
 */
__STATIC_FORCEINLINE uint32_t __ng_get_IPSR(void)
{
  return __get_IPSR();
}

/**
  \brief   Get Current Irq
  \details Returns the content Irq.
  \return               Current Irq number
 */
__STATIC_FORCEINLINE ngIrqTypeT __ng_get_Irqn(void)
{
#if 1
  return NVIC->STIR & NVIC_STIR_INTID_Msk;
#else
  return __ng_get_IPSR() & NVIC_STIR_INTID_Msk;
#endif
}

static inline void rte_prefetch0(const volatile void *p)
{
  asm volatile ("pld [%0]" : : "r" (p));
}

static inline void rte_prefetch1(const volatile void *p)
{
  asm volatile ("pld [%0]" : : "r" (p));
}

static inline void rte_prefetch2(const volatile void *p)
{
  asm volatile ("pld [%0]" : : "r" (p));
}

static inline void rte_prefetch_non_temporal(const volatile void *p)
{
  /* non-temporal version not available, fallback to rte_prefetch0 */
  rte_prefetch0(p);
}

/* A context switch is required.  Context switching is performed in
 * the PendSV interrupt. Pend the PendSV interrupt. 
 */
__STATIC_FORCEINLINE void __ng_sys_yield(void)   
{                                                   
  /* Set a PendSV to request a context switch. 
   *
     SCB: 0xE000E000UL, The base address of System Control Clock register map and reset values.
    ICSR: 0xE000E004UL, uint32_t, (R/W), Interrupt Control and State Register 

    __IOM uint32_t ICSR;  

     Bit 28 PENDSVSET: PendSV set-pending bit.
     Write:
       0: No effect
       1: Change PendSV exception state to pending.
     Read:
       0: PendSV exception is not pending
       1: PendSV exception is pending
     Writing 1 to this bit is the only way to set the PendSV exception state to pending.
   */ 
  SCB->ICSR = SCB_ICSR_PENDSVSET_Msk; 
}
    
/* Scheduler utilities. */
__STATIC_FORCEINLINE void __ng_sys_yield_barrier(void)   
{
  __ng_sys_yield();
                                                  
  /* Barriers are normally not required but do ensure the code is completely 
   * within the specified behaviour for the architecture. */ 
  __DSB();                                        
  __ISB();                                        
}

/*
  Figure 7. BASEPRI bit assignments

    31--------23--------15--------7--------0
    |          reserved           | basepri|
    ----------------------------------------
  Table 10. BASEPRI register bit assignments
      Bits 31:8 Reserved
      Bits  7:4 BASEPRI[7:4] Priority mask bits(1)
                   0x00: no effect
                Nonzero: defines the base priority for 
  			           exception processing. The 
  					   processor does not process 
  					   any exception with a priority 
  					   value greater than or equal to 
  					   BASEPRI.
      Bits 3:0 Reserved

  This field is similar to the priority fields in the 
  interrupt priority registers. See Interrupt priority 
  registers (NVIC_IPRx) on page 201 for more information. 
  Remember that higher priority field values correspond 
  to lower exception priorities.

  From: PM0214, Programming manual STM32F3xxx and STM32F4xxx
        Cortex-M4 programming manual
*/
__STATIC_FORCEINLINE void __ng_sys_disable_irq_minpri(void)  
{
  __set_BASEPRI(__ngRTOS_IRQ_PRIORITY_KERNEL_MAX); 
  __DSB();                                              
  __ISB();
}

__STATIC_FORCEINLINE void __ng_sys_enable_irq_minpri(void)  
{      
  __set_BASEPRI(0);                                       
}

extern void __ng_irq_priority_sanity_check(void);
__STATIC_FORCEINLINE ngIrqTypeT __ng_sys_disable_irq(void)  
{
  ngIrqTypeT basepri;
  if (IS_IRQ())
  {
    __ng_irq_priority_sanity_check();
    basepri = __get_BASEPRI();
  }
  else
  {
    basepri = 0;
  }
  __ng_sys_disable_irq_minpri();
  return basepri;
}

__STATIC_FORCEINLINE void __ng_sys_enable_irq(ngIrqTypeT basepri)  
{      
  __set_BASEPRI(basepri);                                       
}

#define __ng_sys_check_recursive_critical() \
  NGRTOS_ASSERT((SCB->ICSR & portVECTACTIVE_MASK) == 0);

static inline uint32_t __CTZ(uint32_t n)
{
  n = __RBIT(n);
  return n == 32 ? 0 : 31-__CLZ(n); 
}

/*********** Other general functions / macros ********/

/**
 * Searches the input parameter for the least significant set bit
 * (starting from zero).
 * If a least significant 1 bit is found, its bit index is returned.
 * If the content of the input parameter is zero, then the content of 
 * the return value is undefined.
 * @param v
 *     input parameter, should not be zero.
 * @return
 *     least significant set bit in the input parameter.
 */
static inline uint32_t rte_bsf32(uint32_t v)
{
	return __CTZ(v);
}

#define __FLS(v) rte_bsf32(v)

/*
 * An internal function to swap bytes in a 16-bit value.
 *
 * It is used by rte_bswap16() when the value is constant. Do not use
 * this function directly; rte_bswap16() is preferred.
 */
__STATIC_FORCEINLINE uint16_t rte_bswap16(uint16_t x)
{
  return (uint16_t)__REVSH(x);
}

/*
 * An internal function to swap bytes in a 32-bit value.
 *
 * It is used by rte_bswap32() when the value is constant. Do not use
 * this function directly; rte_bswap32() is preferred.
 */
__STATIC_FORCEINLINE uint32_t rte_bswap32(uint32_t x)
{
  return (uint32_t)__REV(x);
}

/*
 * An internal function to swap bytes of a 64-bit value.
 *
 * It is used by rte_bswap64() when the value is constant. Do not use
 * this function directly; rte_bswap64() is preferred.
 */
__STATIC_FORCEINLINE uint64_t rte_bswap64(uint64_t x)
{
  uint32_t t;
  uint32_t *p = (uint32_t *)&x;
  t = p[0];
  p[0] = rte_bswap32(p[1]);
  p[1] = rte_bswap32(t);
  return x;
}

__STATIC_FORCEINLINE uint32_t atom_getu32(uint32_t *p)
{
  return __LDREXW(p);
}

__STATIC_FORCEINLINE uint64_t atom_getu64(uint64_t *p)
{
  uint64_t r;
  uint32_t *a = (uint32_t *)p;
  ngIrqTypeT x = __ng_sys_disable_irq();
  uint32_t m = __LDREXW(a);
  uint32_t n = __LDREXW(a+1);
  r = (uint64_t)n << 32 | (uint64_t)m;
  __ng_sys_enable_irq(x); 
  return r;
}

/**
  Exclusive or (XOR, EOR or EXOR) is a logical operator which results true when 
  either of the operands are true (one is true and the other one is false) but 
  both are not true and both are not false. In logical condition making, the 
  simple "or" is a bit ambiguous when both operands are true. Because in that 
  case it is very difficult to understand what exactly satisfies the condition. 
  To remove this ambiguity, the "exclusive" term has been added to "or" to make 
  it more clear in meaning.
 */
__STATIC_FORCEINLINE uint32_t __EORW(volatile uint32_t *addr, uint32_t bits)
{
  uint32_t result;

  __ASM volatile ("EOR %0, [%1], =bits" : "=r" (result) : "r" (addr));
  return(result);
}

/**
  Exclusive or (XOR, EOR or EXOR) is a logical operator which results true when 
  either of the operands are true (one is true and the other one is false) but 
  both are not true and both are not false. In logical condition making, the 
  simple "or" is a bit ambiguous when both operands are true. Because in that 
  case it is very difficult to understand what exactly satisfies the condition. 
  To remove this ambiguity, the "exclusive" term has been added to "or" to make 
  it more clear in meaning.
 */
__STATIC_FORCEINLINE uint32_t __EORW__(volatile uint32_t addr, uint32_t bits)
{
  uint32_t result;

  __ASM volatile ("EOR %0, %1, =bits" : "=r" (result) : "r" (addr));
  return(result);
}

/**
  op{S}{cond} {Rd,} Rn, Operand2

  Rd is the destination register
  Rn is the register holding the first operand
  Operand2 is a flexible second operand
  
  The BIC instruction performs an AND operation on the bits in Rn with the 
  complements of the corresponding bits in the value of operand2.

  The bitwise complement operator is a unary operator (works on only one 
  operand). It takes one number and inverts all bits of it. When bitwise 
  operator is applied on bits then, all the 1??s become 0??s and vice 
  versa. 

  The operator for the bitwise complement is ~ (Tilde).
 */
__STATIC_FORCEINLINE uint32_t __BIC(volatile uint32_t *addr, uint32_t bits)
{
  uint32_t result;

  __ASM volatile ("BIC %0, [%1], =bits" : "=r" (result) : "r" (addr));
  return(result);
}

/**
  op{S}{cond} {Rd,} Rn, Operand2

  Rd is the destination register
  Rn is the register holding the first operand
  Operand2 is a flexible second operand
  
  The BIC instruction performs an AND operation on the bits in Rn with the 
  complements of the corresponding bits in the value of operand2.
 */
__STATIC_FORCEINLINE uint32_t __BIC__(volatile uint32_t addr, uint32_t bits)
{
  uint32_t result;

  __ASM volatile ("BIC %0, %1, =bits" : "=r" (result) : "r" (addr));
  return(result);
}

__STATIC_FORCEINLINE double __sqrt(double x)
{
	__ASM volatile ("vsqrt.f64 %P0, %P1" : "=w"(x) : "w"(x));
	return x;
}

__STATIC_FORCEINLINE double __fabs(double x)
{
	__ASM volatile ("vabs.f64 %P0, %P1" : "=w"(x) : "w"(x));
	return x;
}

__STATIC_FORCEINLINE double __fma(double x, double y, double z)
{
	__ASM volatile ("vfma.f64 %P0, %P1, %P2" : "+w"(z) : "w"(x), "w"(y));
	return z;
}

#ifdef __cplusplus
    }
#endif

#endif /* __NGRTOS_ARCH_H__ */
