/**************************************************************************************************
  Filename:       hal_mcu.h
  Revised:        $Date: 2012-07-13 06:58:11 -0700 (Fri, 13 Jul 2012) $
  Revision:       $Revision: 30922 $

  Description:    Describe the purpose and contents of the file.


  Copyright 2006-2010 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

#ifndef _HAL_MCU_H
#define _HAL_MCU_H

/*
 *  Target : Texas Instruments CC2540 (8051 core)
 *
 */


/* ------------------------------------------------------------------------------------------------
 *                                           Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "OSAL_Defs.h"
#include "OSAL_Types.h"


/*********************************************************************
*/
// Internal (MCU) RAM addresses
#define MCU_RAM_BEG 0x0100
#define MCU_RAM_END RAMEND
#define MCU_RAM_LEN (MCU_RAM_END - MCU_RAM_BEG + 1)

// Internal (MCU) heap size
#if !defined( INT_HEAP_LEN )
#define INT_HEAP_LEN  1024  // 1.00K
#endif

// Memory Allocation Heap
#if defined( EXTERNAL_RAM )
#define MAXMEMHEAP EXT_RAM_LEN   // Typically, 32K
#else
#define MAXMEMHEAP INT_HEAP_LEN  // Typically, 0.70-1.50K
#endif


/* ------------------------------------------------------------------------------------------------
 *                                        Target Defines
 * ------------------------------------------------------------------------------------------------
 */
#define HAL_ASSERT(expr)
#define HAL_ASSERT_FORCED()
#define HAL_ASSERT_STATEMENT(statement)
#define HAL_ASSERT_DECLARATION(declaration)


/* ------------------------------------------------------------------------------------------------
 *                                     Compiler Abstraction
 * ------------------------------------------------------------------------------------------------
 */

/* ---------------------- IAR Compiler ---------------------- */
#ifdef __IAR_SYSTEMS_ICC__


#define HAL_COMPILER_IAR
#define HAL_MCU_LITTLE_ENDIAN()         __LITTLE_ENDIAN__
#define _PRAGMA(x)                      _Pragma(#x)
#define HAL_ISR_FUNC_DECLARATION(f,v)   _PRAGMA(vector=v) __near_func __interrupt void f(void)
#define HAL_ISR_FUNC_PROTOTYPE(f,v)     _PRAGMA(vector=v) __near_func __interrupt void f(void)
#define HAL_ISR_FUNCTION(f,v)           HAL_ISR_FUNC_PROTOTYPE(f,v); HAL_ISR_FUNC_DECLARATION(f,v)

/* ---------------------- Keil Compiler ---------------------- */
#elif defined __KEIL__

#define HAL_COMPILER_KEIL
#define HAL_MCU_LITTLE_ENDIAN()         0
#define HAL_ISR_FUNC_DECLARATION(f,v)   void f(void) interrupt v
#define HAL_ISR_FUNC_PROTOTYPE(f,v)     void f(void)
#define HAL_ISR_FUNCTION(f,v)           HAL_ISR_FUNC_PROTOTYPE(f,v); HAL_ISR_FUNC_DECLARATION(f,v)

/* ----------------------- ARM Compiler ---------------------- */
#elif defined ( __CC_ARM   )
#define HAL_COMPILER_ARM
#define HAL_MCU_LITTLE_ENDIAN()         0
#define HAL_ISR_FUNC_DECLARATION(f,v)   void f(void) interrupt v
#define HAL_ISR_FUNC_PROTOTYPE(f,v)     void f(void)
#define HAL_ISR_FUNCTION(f,v)           HAL_ISR_FUNC_PROTOTYPE(f,v); HAL_ISR_FUNC_DECLARATION(f,v)

/* ------------------ Unrecognized Compiler ------------------ */
#else
#error "ERROR: Unknown compiler."
#endif


/* ------------------------------------------------------------------------------------------------
 *                                        Interrupt Macros
 * ------------------------------------------------------------------------------------------------
 */
#ifdef __IAR_SYSTEMS_ICC__
#define HAL_ENABLE_INTERRUPTS()         st( EA = 1; )
#define HAL_DISABLE_INTERRUPTS()        st( EA = 0; )
#define HAL_INTERRUPTS_ARE_ENABLED()    (EA)

typedef unsigned char halIntState_t;
#define HAL_ENTER_CRITICAL_SECTION(x)   st( x = EA;  HAL_DISABLE_INTERRUPTS(); )
#define HAL_EXIT_CRITICAL_SECTION(x)    st( EA = x; )
#define HAL_CRITICAL_STATEMENT(x)       st( halIntState_t _s; HAL_ENTER_CRITICAL_SECTION(_s); x; HAL_EXIT_CRITICAL_SECTION(_s); )

/* IAR library uses XCH instruction with EA. It may cause the higher priority interrupt to be
 * locked out, therefore, may increase interrupt latency.  It may also create a lockup condition.
 * This workaround should only be used with 8051 using IAR compiler. When IAR fixes this by
 * removing XCH usage in its library, compile the following macros to null to disable them.
 */
#define HAL_ENTER_ISR()               { halIntState_t _isrIntState = EA; HAL_ENABLE_INTERRUPTS();
#define HAL_EXIT_ISR()                  EA = _isrIntState; }

#else
// TODO

#define HAL_ENABLE_INTERRUPTS()
#define HAL_DISABLE_INTERRUPTS()
#define HAL_INTERRUPTS_ARE_ENABLED()    (0)

  typedef unsigned char halIntState_t;
#define HAL_ENTER_CRITICAL_SECTION(x)
#define HAL_EXIT_CRITICAL_SECTION(x)
#define HAL_CRITICAL_STATEMENT(x)

#define HAL_ENTER_ISR()
#define HAL_EXIT_ISR()
#endif /* __IAR_SYSTEMS_ICC__ */

/**************************************************************************************************
 */
#endif

/* vim: set ts=2 sw=2 tw=0 list : */
