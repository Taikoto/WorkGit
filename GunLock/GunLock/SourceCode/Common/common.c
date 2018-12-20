/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2010 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

#include <stdint.h>
#include "M051.h"
#include "Register_Bit.h"
#include "Macro_SystemClock.h"
#include "Macro_Timer.h"
/*-----------------------------------------------------------------------------
  Function:		Un_Lock_Reg                                                                         
                                                                                                         
  Parameter:        																					   	
	        	None                                     
  Returns:                                                                                                
               	None                                                                                      
  Description:                                                                                            
               	Unlock protected register bits, so as user can access to.                                    
 *-----------------------------------------------------------------------------*/
void Un_Lock_Reg(void)
{
    RegLockAddr = 0x59;
    RegLockAddr = 0x16;
    RegLockAddr = 0x88;
}

/*-----------------------------------------------------------------------------
  Function:		Lock_Reg                                                                         
                                                                                                         
  Parameter:        																					   	
	        	None                                     
  Returns:                                                                                                
               	None                                                                                      
  Description:                                                                                            
               	Lock protected register bits, avoiding unknown errors when 
				access illegally.                                    
 *-----------------------------------------------------------------------------*/
void Lock_Reg(void)
{
    RegLockAddr = 0x00;
}

/*-----------------------------------------------------------------------------
  Function:		PLL_Enable                                                                         
                                                                                                         
  Parameter:        																					   	
	        	None                                     
  Returns:                                                                                                
               	None                                                                                      
  Description:                                                                                            
               	Enable PLL function.                                    
 *-----------------------------------------------------------------------------*/
void PLL_Enable(void)
{
    PLLCON &= ~(PLL_OE|PLL_PD);
}

/*-----------------------------------------------------------------------------
  Function:		NSR_Enable                                                                         
                                                                                                         
  Parameter:        																					   	
	        	None                                     
  Returns:                                                                                                
               	None                                                                                      
  Description:                                                                                            
               	Enable nosie sensitivity reduction function.                                    
 *-----------------------------------------------------------------------------*/
void NSR_Enable(void) //Noise Sensitivity Recuduction
{
    Un_Lock_Reg();
    PORCR = 0x00005AA5;
}

/*-----------------------------------------------------------------------------
  Function:		NSR_Disable                                                                         
                                                                                                         
  Parameter:        																					   	
	        	None                                     
  Returns:                                                                                                
               	None                                                                                      
  Description:                                                                                            
               	Disable nosie sensitivity reduction function.                                    
 *-----------------------------------------------------------------------------*/
void NSR_Disable(void) //Noise Sensitivity Recuduction Disable
{
    Un_Lock_Reg();
    PORCR = 0x00000000;
}



void McuInit(uint32_t unFrequency)
{
	uint32_t u32NR, u32NF, u32NO;
	uint32_t u32PllSrcClk = 12000000, u32PllClk = unFrequency;

    Un_Lock_Reg();

	PWRCON |= XTL12M_EN;
	while((CLKSTATUS & XTL12M_STB) == 0); 		//Wait until 12M clock is stable.	
	/* HCLK时钟选择为外部晶振 */
	CLKSEL0 = (CLKSEL0 & (~HCLK)) | HCLK_12M;
		

	if(unFrequency == 12000000)
	{
	    goto end;
	}
	
	u32NO = 3;
	u32PllClk = u32PllClk << 2;
	
	u32NF = u32PllClk / 1000000;
	u32NR = u32PllSrcClk / 1000000;	
	
	while(1)
	{
		if ((u32NR & 0x01) || (u32NF & 0x01))
		{
			break;
		}
		else
		{
			u32NR >>= 1;
			u32NF >>= 1;
		}	
	} 
	
	PLLCON = (((u32NO<<14) | ((u32NR - 2)<<9) | (u32NF - 2))&0xffff);
	
	while((CLKSTATUS & PLL_STB) == 0);
	
	CLKSEL0 =  (CLKSEL0 & (~HCLK)) | HCLK_PLL;

end:
	Lock_Reg();
}


void Delayus(uint32_t unCnt)
{
      SYST_RVR = unCnt*12;
	  SYST_CVR = 0;
	  SYST_CSR |=1UL<<0;
	  
	  while((SYST_CSR & 1UL<<16)==0); 
}

void Delayms(uint32_t unCnt)
{
#if 0
	 static uint8_t b=1;

	 if(b)
	 {
	    b=0;
	    TMR0_Clock_EN;
		TMR0ClkSource_ex12MHz;
	    TCSR0  = 0x00000001;    //Pre-Scaler
	    
	    setTMR0_PERIOD;
	    setTMR0_IE;             //Timer0 interrupt enable
	    setTMR0_CRST;           //Reset the timer/counter0, after set, this bit will be clear by H/W	 
	 }

	  TCMPR0 = 12000;           //Fosc=12MHz, so 12000000/12000=1000Hz=1ms
	  setTMR0_CEN;              //启动 TMR0

	  while (unCnt != 1)
	  {
	      while ((TISR0&TMR_TIF) != TMR_TIF); //检查 TIF0
	      TISR0 |= TMR_TIF;     //清零 TIF0
	      unCnt --;
	  }

	  clrTMR0_CEN;              //停止TMR0
#else

      SYST_RVR = unCnt*12000;
	  SYST_CVR = 0;
	  SYST_CSR |=1UL<<0;
	  
	  while((SYST_CSR & 1UL<<16)==0); 

#endif
}



