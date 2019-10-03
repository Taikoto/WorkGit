/*******************************************************************************
* Copyright (C) 2013 Spansion LLC. All Rights Reserved.
*
* This software is owned and published by:
* Spansion LLC, 915 DeGuigne Dr. Sunnyvale, CA  94088-3453 ("Spansion").
*
* BY DOWNLOADING, INSTALLING OR USING THIS SOFTWARE, YOU AGREE TO BE BOUND
* BY ALL THE TERMS AND CONDITIONS OF THIS AGREEMENT.
*
* This software contains source code for use with Spansion
* components. This software is licensed by Spansion to be adapted only
* for use in systems utilizing Spansion components. Spansion shall not be
* responsible for misuse or illegal use of this software for devices not
* supported herein.  Spansion is providing this software "AS IS" and will
* not be responsible for issues arising from incorrect user implementation
* of the software.
*
* SPANSION MAKES NO WARRANTY, EXPRESS OR IMPLIED, ARISING BY LAW OR OTHERWISE,
* REGARDING THE SOFTWARE (INCLUDING ANY ACOOMPANYING WRITTEN MATERIALS),
* ITS PERFORMANCE OR SUITABILITY FOR YOUR INTENDED USE, INCLUDING,
* WITHOUT LIMITATION, THE IMPLIED WARRANTY OF MERCHANTABILITY, THE IMPLIED
* WARRANTY OF FITNESS FOR A PARTICULAR PURPOSE OR USE, AND THE IMPLIED
* WARRANTY OF NONINFRINGEMENT.
* SPANSION SHALL HAVE NO LIABILITY (WHETHER IN CONTRACT, WARRANTY, TORT,
* NEGLIGENCE OR OTHERWISE) FOR ANY DAMAGES WHATSOEVER (INCLUDING, WITHOUT
* LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION,
* LOSS OF BUSINESS INFORMATION, OR OTHER PECUNIARY LOSS) ARISING FROM USE OR
* INABILITY TO USE THE SOFTWARE, INCLUDING, WITHOUT LIMITATION, ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES OR LOSS OF DATA,
* SAVINGS OR PROFITS,
* EVEN IF SPANSION HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* YOU ASSUME ALL RESPONSIBILITIES FOR SELECTION OF THE SOFTWARE TO ACHIEVE YOUR
* INTENDED RESULTS, AND FOR THE INSTALLATION OF, USE OF, AND RESULTS OBTAINED
* FROM, THE SOFTWARE.
*
* This software may be replicated in part or whole for the licensed use,
* with the restriction that this Disclaimer and Copyright notice must be
* included with each copy of this software, whether used in part or whole,
* at all times.
*****************************************************************************/
/** \file base_types.h
 **
 ** Base type header file defining all general data types that should be used
 ** instead the ANSI C data types
 **
 ** History:
 ** 2009-10-02  0.01  JWa   First version
 ** 2009-10-09  0.02  JWa   en_result_t added
 ** 2009-10-21  0.03  JWa   removed FAR_NULL and NEAR_NULL macros
 **                         (use CPU_TYPE for differentiation)
 ** 2009-10-22  0.04  JWa   ErrorOperationInProgress added to en_result_t
 ** 2010-01-18  0.05  JWa   ErrorInvalidMode and ErrorUninitialized added to en_result_t
 ** 2010-03-19  0.06  JWa   modifiy SVN keywords
 ** 2010-05-26  0.07  JWa   remove mcu.h from include list
 ** 2011-04-27  0.08  JWa   update disclaimer, add macros, extend en_result_t
 ** 2013-10-16  0.09  IAM   Disclaimer updated
 *****************************************************************************/

#ifndef __BASE_TYPE_H__
#define __BASE_TYPE_H__


/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/

/*****************************************************************************/
/* Global pre-processor symbols/macros ('#define')                           */
/*****************************************************************************/
#ifndef TRUE
    /** Value is true (boolean_t type) */
    #define TRUE        ((boolean_t) 1)
#endif

#ifndef FALSE
    /** Value is false (boolean_t type) */
    #define FALSE       ((boolean_t) 0)
#endif

#define OK                            0
#define ERROR                         1

#ifndef  PARA_ERROR

    #define PARA_ERROR                    0xFF
#endif

#ifndef  NULL
    #define  NULL                            0
#endif


typedef enum
{
    false = 0,
    true = !false
} bool;

typedef enum
{
    DISABLE = 0,
    ENABLE = !DISABLE
} FunctionalState;

typedef enum
{
    RESET = 0,
    SET = !RESET
}
FlagStatus, ITStatus, BitStatus;

/**
  * @brief  HAL Lock structures definition
  */
typedef enum
{
    HAL_UNLOCKED = 0x00,
    HAL_LOCKED   = 0x01
} HAL_LockTypeDef;

/**
  * @brief  HAL Status structures definition
  */
typedef enum
{
    HAL_OK       = 0x00,
    HAL_ERROR    = 0x01,
    HAL_BUSY     = 0x02,
    HAL_TIMEOUT  = 0x03
} HAL_StatusTypeDef;


#define U8_MAX     ((u8)255)
#define S8_MAX     ((s8)127)
#define S8_MIN     ((s8)-128)
#define U16_MAX    ((u16)65535u)
#define S16_MAX    ((s16)32767)
#define S16_MIN    ((s16)-32768)
#define U32_MAX    ((u32)4294967295uL)
#define S32_MAX    ((s32)2147483647)
#define S32_MIN    ((s32)-2147483648)

#define BIT0 0x01
#define BIT1 0x02
#define BIT2 0x04
#define BIT3 0x08
#define BIT4 0x10
#define BIT5 0x20
#define BIT6 0x40
#define BIT7 0x80

#define BIT0N 0xFE
#define BIT1N 0xFD
#define BIT2N 0xFB
#define BIT3N 0xF7
#define BIT4N 0xEF
#define BIT5N 0xDF
#define BIT6N 0xBF
#define BIT7N 0x7F


/** Returns the minimum value out of two values */
#define MIN_VAL( X, Y )  ((X) < (Y) ? (X) : (Y))

/** Returns the maximum value out of two values */
#define MAX_VAL( X, Y )  ((X) > (Y) ? (X) : (Y))

/** Returns the dimension of an array */
#define DIM( X )  (sizeof(X) / sizeof(X[0]))

/*****************************************************************************/
/* Global type definitions ('typedef')                                       */
/*****************************************************************************/
typedef long                          int32_t;
typedef short                         int16_t;
typedef char                          int8_t;

typedef unsigned long                 uint32_t;
typedef unsigned short                uint16_t;
typedef unsigned char                 uint8_t;

typedef signed long                   s32;
typedef signed short                  s16;
typedef char                          s8;

typedef unsigned long long int        u64;
typedef unsigned long                 u32;
typedef unsigned short                u16;
typedef unsigned char                 u8;

typedef signed long  const            sc32;  /* Read Only */
typedef signed short const            sc16;  /* Read Only */
typedef signed char  const            sc8;   /* Read Only */

typedef volatile signed long          vs32;
typedef volatile signed short         vs16;
typedef volatile signed char          vs8;

typedef volatile signed long  const   vsc32;  /* Read Only */
typedef volatile signed short const   vsc16;  /* Read Only */
typedef volatile signed char  const   vsc8;   /* Read Only */

typedef unsigned long                 DWORD;
typedef unsigned short                WORD;
typedef unsigned char                 BYTE;

typedef unsigned long  const          uc32;  /* Read Only */
typedef unsigned short const          uc16;  /* Read Only */
typedef unsigned char  const          uc8;   /* Read Only */

typedef volatile unsigned long        vu32;
typedef volatile unsigned short       vu16;
typedef volatile unsigned char        vu8;

typedef volatile unsigned long  const vuc32;  /* Read Only */
typedef volatile unsigned short const vuc16;  /* Read Only */
typedef volatile unsigned char  const vuc8;   /* Read Only */

/** logical datatype (only values are TRUE and FALSE) */
typedef unsigned char                  boolean_t;

/** single precision floating point number (4 byte) */
typedef float                         float32_t;

/** double precision floating point number (8 byte) */
typedef double                        float64_t;

/** ASCCI character for string generation (8 bit) */
typedef char                          char_t;

typedef unsigned long                 UBaseType_t;


typedef u8 EcuM_StateType;
/* @req EcuM4067 */
typedef u8 EcuM_UserType;

typedef u8 Std_ReturnType;

/****************************************************************************/
/* typedefs for basic numerical types; MISRA-C 2004 rule 6.3(req). */

/*! typedef for line numbers in assertions and return from QF_run() */
typedef int int_t;

/*! typedef for enumerations used for event signals */
typedef int enum_t;

/*! IEEE 754 32-bit floating point number, MISRA-C 2004 rule 6.3(req) */
/**
* @note QP does not use floating-point types anywhere in the internal
* implementation, except in QS software tracing, where utilities for
* output of floating-point numbers are provided for application-level
* trace records.
*/

/* Fastest minimum-width types. WG14/N843 C99 Standard, Section 7.18.1.3 */
typedef signed   int  int_fast8_t;   /*!< fast at-least  8-bit signed   int */
typedef unsigned int  uint_fast8_t;  /*!< fast at-least  8-bit unsigned int */
typedef signed   int  int_fast16_t;  /*!< fast at-least 16-bit signed   int */
typedef unsigned int  uint_fast16_t; /*!< fast at-least 16-bit unsigned int */
typedef signed   long int_fast32_t;  /*!< fast at-least 32-bit signed   int */
typedef unsigned long uint_fast32_t; /*!< fast at-least 32-bit unsigned int */

/** function pointer type to void/void function*/
typedef  void (*func_ptr_t)(void);
typedef  void (*HAL_Callback_Func)(void *);
typedef  void (*HAL_Callback_Func1)(u32);
typedef  void (*HAL_Callback_Func2)(void);
typedef  void (*HAL_Callback_Func3)(u8 *pData, u16 Len);
typedef  u8(*HAL_Callback_Func4)(void *);

/** generic error codes */
typedef enum en_result
{
    Ok                          = 0,  ///< No error
    Error                       = 1,  ///< Non-specific error code
    ErrorAddressAlignment       = 2,  ///< Address alignment does not match
    ErrorAccessRights           = 3,  ///< Wrong mode (e.g. user/system) mode is set
    ErrorInvalidParameter       = 4,  ///< Provided parameter is not valid
    ErrorOperationInProgress    = 5,  ///< A conflicting or requested operation is still in progress
    ErrorInvalidMode            = 6,  ///< Operation not allowed in current mode
    ErrorUninitialized          = 7,  ///< Module (or part of it) was not initialized properly
    ErrorBufferFull             = 8,  ///< Circular buffer can not be written because the buffer is full
    ErrorTimeout                = 9   ///< A requested operation could not be completed
} en_result_t;

/*****************************************************************************/
/* Global variable declarations ('extern', definition in C source)           */
/*****************************************************************************/

/*****************************************************************************/
/* Global function prototypes ('extern', definition in C source)             */
/*****************************************************************************/

#endif /* __BASE_TYPE_H__ */
