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

#ifndef __ngRTOS_ATOMIC_H__
#define __ngRTOS_ATOMIC_H__

#ifdef __cplusplus
    extern "C" {
#endif

#define ATOMIC_ENTER_CRITICAL() ngIrqTypeT x=__ng_sys_disable_irq()
#define ATOMIC_EXIT_CRITICAL() __ng_sys_enable_irq(x)

/**
 * Atomic compare-and-swap
 *
 * @brief Performs an atomic compare-and-swap operation on the specified values.
 *
 * @param[in, out] pulDestination  Pointer to memory location from where value is
 *                               to be loaded and checked.
 * @param[in] ulExchange         If condition meets, write this value to memory.
 * @param[in] ulComparand        Swap condition.
 *
 * @return Unsigned integer of value 1 or 0. 1 for swapped, 0 for not swapped.
 *
 * @note This function only swaps *pulDestination with ulExchange, if previous
 *       *pulDestination value equals ulComparand.
 */
__STATIC_FORCEINLINE ng_bool_t 
ngrtos_Atomic_CompareAndSwap_u32(uint32_t volatile *pulDestination,
  uint32_t ulExchange, uint32_t ulComparand)
{
  ng_bool_t success = ngrtos_FALSE;

  ATOMIC_ENTER_CRITICAL();
  do {
    uint32_t val;
    val = __LDREXW(pulDestination); 
    if (val != ulComparand)
      return ngrtos_FALSE;
    success = (__STREXW(ulExchange,pulDestination) != 0U);
  } while(!success);
  ATOMIC_EXIT_CRITICAL();
  
  return ngrtos_TRUE;
}

/*-----------------------------------------------------------*/

/**
 * Atomic swap (pointers)
 *
 * @brief Atomically sets the address pointed to by *ppvDestination to the value
 *        of *pvExchange.
 *
 * @param[in, out] ppvDestination  Pointer to memory location from where a pointer
 *                                 value is to be loaded and written back to.
 * @param[in] pvExchange           Pointer value to be written to *ppvDestination.
 *
 * @return The initial value of *ppvDestination.
 */
__STATIC_FORCEINLINE void * 
ngrtos_Atomic_SwapPointers_p32(void * volatile * ppvDestination,
    void * pvExchange )
{
  uint32_t val;
  
  ATOMIC_ENTER_CRITICAL();
  do {
    val = __LDREXW((uint32_t volatile*)ppvDestination); 
  } while(__STREXW((uint32_t)pvExchange,(uint32_t *)ppvDestination) != 0U);
  ATOMIC_EXIT_CRITICAL();
  
  return (void *)val;
}
/*-----------------------------------------------------------*/

/**
 * Atomic compare-and-swap (pointers)
 *
 * @brief Performs an atomic compare-and-swap operation on the specified pointer
 *        values.
 *
 * @param[in, out] ppvDestination  Pointer to memory location from where a pointer
 *                                 value is to be loaded and checked.
 * @param[in] pvExchange           If condition meets, write this value to memory.
 * @param[in] pvComparand          Swap condition.
 *
 * @return Unsigned integer of value 1 or 0. 1 for swapped, 0 for not swapped.
 *
 * @note This function only swaps *ppvDestination with pvExchange, if previous
 *       *ppvDestination value equals pvComparand.
 */
__STATIC_FORCEINLINE ng_bool_t 
ngrtos_Atomic_CompareAndSwapPointers_p32(
  void * volatile * ppvDestination,
  void * pvExchange,
  void * pvComparand )
{
  ng_bool_t success = ngrtos_FALSE;
  
  ATOMIC_ENTER_CRITICAL();
  do {
    uint32_t val;            
    val = __LDREXW((uint32_t*)ppvDestination); 
    if (val != (uint32_t)pvComparand)
      return ngrtos_FALSE;
    success = (__STREXW((uint32_t)pvExchange,(uint32_t*)ppvDestination) != 0U);
  } while(!success);
  ATOMIC_EXIT_CRITICAL();

  return ngrtos_TRUE;
}

/*----------------------------- Arithmetic ------------------------------*/

/**
 * Atomic add
 *
 * @brief Atomically adds count to the value of the specified pointer points to.
 *
 * @param[in,out] pulAddend  Pointer to memory location from where value is to be
 *                         loaded and written back to.
 * @param[in] ulCount      Value to be added to *pulAddend.
 *
 * @return previous *pulAddend value.
 */
__STATIC_FORCEINLINE uint32_t 
ngrtos_Atomic_Add_u32(uint32_t volatile * pulAddend,
  uint32_t ulCount )
{
  uint32_t val;
  
  ATOMIC_ENTER_CRITICAL();
  do {                                                     
    val = __LDREXW(pulAddend);      
  } while ((__STREXW(val+ulCount,pulAddend)) != 0U); 
  ATOMIC_EXIT_CRITICAL();

  return val;
}
/*-----------------------------------------------------------*/

/**
 * Atomic subtract
 *
 * @brief Atomically subtracts count from the value of the specified pointer
 *        pointers to.
 *
 * @param[in,out] pulAddend  Pointer to memory location from where value is to be
 *                         loaded and written back to.
 * @param[in] ulCount      Value to be subtract from *pulAddend.
 *
 * @return previous *pulAddend value.
 */
__STATIC_FORCEINLINE uint32_t 
ngrtos_Atomic_Subtract_u32(uint32_t volatile * pulAddend,
  uint32_t ulCount)
{
  uint32_t val;
  
  ATOMIC_ENTER_CRITICAL();
  do {                                                     
    val = __LDREXW(pulAddend);      
  } while ((__STREXW(val-ulCount,pulAddend)) != 0U); 
  ATOMIC_EXIT_CRITICAL();

  return val;
}
/*-----------------------------------------------------------*/

/**
 * Atomic increment
 *
 * @brief Atomically increments the value of the specified pointer points to.
 *
 * @param[in,out] pulAddend  Pointer to memory location from where value is to be
 *                         loaded and written back to.
 *
 * @return *pulAddend value before increment.
 */
__STATIC_FORCEINLINE uint32_t 
ngrtos_Atomic_Increment_u32(uint32_t volatile * pulAddend)
{
  uint32_t val;

  ATOMIC_ENTER_CRITICAL();
  do {                                                     
    val = __LDREXW(pulAddend);      
  } while ((__STREXW(val+1,pulAddend)) != 0U); 
  ATOMIC_EXIT_CRITICAL();

  return val;
}
/*-----------------------------------------------------------*/

/**
 * Atomic decrement
 *
 * @brief Atomically decrements the value of the specified pointer points to
 *
 * @param[in,out] pulAddend  Pointer to memory location from where value is to be
 *                         loaded and written back to.
 *
 * @return *pulAddend value before decrement.
 */
__STATIC_FORCEINLINE uint32_t 
ngrtos_Atomic_Decrement_u32(uint32_t volatile * pulAddend)
{
  uint32_t val;

  ATOMIC_ENTER_CRITICAL();
  do {                                                     
    val = __LDREXW(pulAddend);      
  } while ((__STREXW(val-1,pulAddend)) != 0U); 
  ATOMIC_EXIT_CRITICAL();

  return val;
}

/*----------------------------- Bitwise Logical ------------------------------*/

/**
 * Atomic OR
 *
 * @brief Performs an atomic OR operation on the specified values.
 *
 * @param [in, out] pulDestination  Pointer to memory location from where value is
 *                                to be loaded and written back to.
 * @param [in] ulValue            Value to be ORed with *pulDestination.
 *
 * @return The original value of *pulDestination.
 */
__STATIC_FORCEINLINE uint32_t 
ngrtos_Atomic_OR_u32(uint32_t volatile * pulDestination,
  uint32_t ulValue )
{
  uint32_t val;

  ATOMIC_ENTER_CRITICAL();
  do {                                                     
    val = __LDREXW(pulDestination);      
  } while ((__STREXW(val|ulValue,pulDestination)) != 0U); 
  ATOMIC_EXIT_CRITICAL();

  return val;
}
/*-----------------------------------------------------------*/

/**
 * Atomic AND
 *
 * @brief Performs an atomic AND operation on the specified values.
 *
 * @param [in, out] pulDestination  Pointer to memory location from where value is
 *                                to be loaded and written back to.
 * @param [in] ulValue            Value to be ANDed with *pulDestination.
 *
 * @return The original value of *pulDestination.
 */
__STATIC_FORCEINLINE uint32_t 
ngrtos_Atomic_AND_u32(uint32_t volatile * pulDestination,
  uint32_t ulValue)
{
  uint32_t val;

  ATOMIC_ENTER_CRITICAL();
  do {                                                     
    val = __LDREXW(pulDestination);      
  } while ((__STREXW(val&ulValue,pulDestination)) != 0U); 
  ATOMIC_EXIT_CRITICAL();

  return val;
}
/*-----------------------------------------------------------*/

/**
 * Atomic NAND
 *
 * @brief Performs an atomic NAND operation on the specified values.
 *
 * @param [in, out] pulDestination  Pointer to memory location from where value is
 *                                to be loaded and written back to.
 * @param [in] ulValue            Value to be NANDed with *pulDestination.
 *
 * @return The original value of *pulDestination.
 */
__STATIC_FORCEINLINE uint32_t 
ngrtos_Atomic_NAND_u32(uint32_t volatile * pulDestination,
  uint32_t ulValue)
{
  uint32_t val;

  ATOMIC_ENTER_CRITICAL();
  do {                                                     
    val = __LDREXW(pulDestination);      
  } while ((__STREXW(~(val&ulValue),pulDestination)) != 0U); 
  ATOMIC_EXIT_CRITICAL();

  return val;
}
/*-----------------------------------------------------------*/

/**
 * Atomic XOR
 *
 * @brief Performs an atomic XOR operation on the specified values.
 *
 * @param [in, out] pulDestination  Pointer to memory location from where value is
 *                                to be loaded and written back to.
 * @param [in] ulValue            Value to be XORed with *pulDestination.
 *
 * @return The original value of *pulDestination.
 */
__STATIC_FORCEINLINE uint32_t 
ngrtos_Atomic_XOR_u32(uint32_t volatile *pulDestination,
  uint32_t ulValue)
{
  uint32_t val;

  ATOMIC_ENTER_CRITICAL();
  do {                                                     
    val = __LDREXW(pulDestination);      
  } while ((__STREXW(val^ulValue,pulDestination)) != 0U); 
  ATOMIC_EXIT_CRITICAL();
  
  return val;
}

/* Use of CMSIS compiler intrinsics for register exclusive access */
/* Atomic 32-bit register access macro to set one or several bits */
#define ngrtos_ATOMIC_SET_BIT(REG, BIT)                             \
  do {                                                       \
    uint32_t val;                                            \
    ATOMIC_ENTER_CRITICAL();  \
    do {                                                     \
      val = __LDREXW((uint32_t *)&(REG)) | (BIT);       \
    } while ((__STREXW(val,(uint32_t *)&(REG))) != 0U); \
    ATOMIC_EXIT_CRITICAL(); \
  } while(0)

/* Atomic 32-bit register access macro to clear one or several bits */
#define ngrtos_ATOMIC_CLEAR_BIT(REG, BIT)                           \
  do {                                                       \
    uint32_t val;                                            \
    ATOMIC_ENTER_CRITICAL();  \
    do {                                                     \
      val = __LDREXW((uint32_t *)&(REG)) & ~(BIT);      \
    } while ((__STREXW(val,(uint32_t *)&(REG))) != 0U); \
    ATOMIC_EXIT_CRITICAL(); \
  } while(0)

/* Atomic 32-bit register access macro to clear and set one or several bits */
#define ngrtos_ATOMIC_MODIFY_REG(REG, CLEARMSK, SETMASK)                   \
  do {                                                                     \
    uint32_t val;                                                          \
    ATOMIC_ENTER_CRITICAL();  \
    do {                                                                   \
      val = (__LDREXW((uint32_t *)&(REG)) & ~(CLEARMSK)) | (SETMASK); \
    } while ((__STREXW(val,(uint32_t *)&(REG))) != 0U);               \
    ATOMIC_EXIT_CRITICAL(); \
  } while(0)

/* Atomic 16-bit register access macro to set one or several bits */
#define ngrtos_ATOMIC_SETH_BIT(REG, BIT)                            \
  do {                                                       \
    uint16_t val;                                            \
    ATOMIC_ENTER_CRITICAL();  \
    do {                                                     \
      val = __LDREXH((uint16_t *)&(REG)) | (BIT);       \
    } while ((__STREXH(val,(uint16_t *)&(REG))) != 0U); \
    ATOMIC_EXIT_CRITICAL(); \
  } while(0)

/* Atomic 16-bit register access macro to clear one or several bits */
#define ngrtos_ATOMIC_CLEARH_BIT(REG, BIT)                          \
  do {                                                       \
    uint16_t val;                                            \
    ATOMIC_ENTER_CRITICAL();  \
    do {                                                     \
      val = __LDREXH((uint16_t *)&(REG)) & ~(BIT);      \
    } while ((__STREXH(val,(uint16_t *)&(REG))) != 0U); \
    ATOMIC_EXIT_CRITICAL(); \
  } while(0)

/* Atomic 16-bit register access macro to clear and set one or several bits */
#define ngrtos_ATOMIC_MODIFYH_REG(REG, CLEARMSK, SETMASK)                  \
  do {                                                                     \
    uint16_t val;                                                          \
    ATOMIC_ENTER_CRITICAL();  \
    do {                                                                   \
      val = (__LDREXH((uint16_t *)&(REG)) & ~(CLEARMSK)) | (SETMASK); \
    } while ((__STREXH(val,(uint16_t *)&(REG))) != 0U);               \
    ATOMIC_EXIT_CRITICAL(); \
  } while(0)

#ifdef __cplusplus
    }
#endif

#endif /* __ngRTOS_ATOM_H__ */

