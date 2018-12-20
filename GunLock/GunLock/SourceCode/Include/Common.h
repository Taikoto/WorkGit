/*---------------------------------------------------------------------------------------------------------
                                                                                                      
  Copyright(c) 2010 Nuvoton Technology Corp. All rights reserved.                                         
                                                                                                         
 * 修改人  ：温子祺
 * 修改日期：2011-06-21
 * 修改内容：添加响应的宏和函数                                                                                          
 *---------------------------------------------------------------------------------------------------------*/

#ifndef __COMMON_H__
#define __COMMON_H__
#include <stdio.h>
#include <string.h>
#include "Uart.h"

#ifndef __DEBUG
#define __DEBUG
#endif

#ifdef  __DEBUG
#define DEBUGMSG	 printf
#else
#define DEBUGMSG(x)	 (void)0
#endif

#ifndef LITTLE_ENDPOINT
#define LITTLE_ENDPOINT
#endif

#ifdef  LITTLE_ENDPOINT
#define SWAP16(x)   (x)
#else
#define SWAP16(x)  (((UINT8)(x)<<8)|(UINT8)((x)>>8))
#endif

#define LSB(x)     ((UINT8)(x))
#define MSB(x)     ((UINT8)(((UINT16)(x))>>8)) 

#define __WFI				  __wfi

#define PROTECT_REG(__CODE)		  {Un_Lock_Reg();__CODE;Lock_Reg();}


#define CLOCK_SETUP           1
#define CLOCK_EN              0xF
#define PLL_Engine_Enable     1 
#define PLL_SEL               0x00000000 
#define CLOCK_SEL             0x0   

typedef struct
{
	   void(*fun)(void);
	   char  *s;
}FUNCTION_ARRAY;



void Un_Lock_Reg(void);
void Lock_Reg(void);
void PLL_Enable(void);
void Short_Delay(void);
void NSR_Enable(void);
void NSR_Disable(void);
void McuInit(uint32_t unFrequency);
void Delayus(uint32_t unCnt);
void Delayms(uint32_t unCnt);

#endif
