#include "SmartM_M0.h"
#include "timer.h"

/****************************************
*函数名称:TMR1Init
*输    入:无
*输    出:无
*功    能:定时器1初始化
******************************************/
VOID TMR1Init(VOID)
{
	PROTECT_REG
	(
	/* 使能TMR1时钟源 */
    APBCLK |= TMR1_CLKEN;
	/* 选择TMR1时钟源为外部晶振12MHz */  
	CLKSEL1 = (CLKSEL1 & (~TM1_CLK)) | TM1_12M; 	

	/* 复位TMR1 */
	IPRSTC2 |= TMR1_RST;
	IPRSTC2 &= ~TMR1_RST;

	/* 选择TMR1的工作模式为周期模式*/	
	TCSR1 &= ~TMR_MODE;
	TCSR1 |= MODE_PERIOD;		
	
	/* 溢出周期 = (Period of timer clock input) * (8-bit Prescale + 1) * (24-bit TCMP)*/
	TCSR1 = TCSR1 & 0xFFFFFF00;		// 设置预分频值 [0~255]
	TCMPR1 = 12000*2;			        // 设置比较值 [0~16777215]

	/* 使能TMR1中断 */
	TCSR1 |= TMR_IE;		
	NVIC_ISER |= TMR1_INT;	

	/* 复位TMR1计数器 */
	TCSR1 |= CRST;	

	/* 使能TMR1 */					
	TCSR1 |= CEN;	
	)
}


/****************************************
*函数名称:TMR2Init
*输    入:无
*输    出:无
*功    能:定时器2初始化
******************************************/
VOID TMR2Init(VOID)
{
  PROTECT_REG
	(
		/* 使能TMR2时钟源 */
	  APBCLK |= TMR2_CLKEN;
		/* 选择TMR2时钟源为外部晶振12MHz */  
		CLKSEL1 = (CLKSEL1 & (~TM2_CLK)) | TM2_12M; 	
	
		/* 复位TMR2 */
		IPRSTC2 |=  TMR2_RST;
		IPRSTC2 &= ~TMR2_RST;
	
		/* 选择TMR2的工作模式为周期模式*/	
		TCSR2 &= ~TMR_MODE;
		TCSR2 |=  MODE_PERIOD;		
		
		/* 溢出周期 = (Period of timer clock input) * (8-bit Prescale + 1) * (24-bit TCMP)*/
		TCSR2  = TCSR2 & 0xFFFFFF00;		// 设置预分频值 [0~255]	此时的分频值为0
		/*(12000*1000)/12000000     2000ms 中断一次*/
		TCMPR2 = 12000*1398;				    // 设置比较值 [0~16777215]

		TCSR2 |= TMR_IE;					//使能TMR2中断
		NVIC_ISER |= TMR2_INT;	
	
		TCSR2 |= CRST;						//复位TMR2计数器				
		TCSR2 |= CEN;						//使能TMR2
	)
}


/****************************************
*函数名称:TMR3Init
*输    入:无
*输    出:无
*功    能:定时器3初始化
******************************************/
VOID TMR3Init(VOID)
{
  PROTECT_REG
	(
		/* 使能TMR3时钟源 */
	  APBCLK |= TMR3_CLKEN;
		/* 选择TMR3时钟源为外部晶振12MHz */  
		CLKSEL1 = (CLKSEL1 & (~TM3_CLK)) | TM3_12M; 	
	
		/* 复位TMR3 */
		IPRSTC2 |=  TMR3_RST;
		IPRSTC2 &= ~TMR3_RST;
	
		/* 选择TMR3的工作模式为周期模式*/	
		TCSR3 &= ~TMR_MODE;
		TCSR3 |=  MODE_PERIOD;		
		
		/* 溢出周期 = (Period of timer clock input) * (8-bit Prescale + 1) * (24-bit TCMP)*/
		TCSR3  = TCSR3 & 0xFFFFFF00;		// 设置预分频值 [0~255]
		TCMPR3 = 12000*1000;				    // 设置比较值 [0~16777215]//时间设置最大时间为1.4s,这里设置为1s

		TCSR3 |= TMR_IE;					//使能TMR3中断
		NVIC_ISER |= TMR3_INT;	
	
		TCSR3 |= CRST;						//复位TMR3计数器				
		TCSR3 |= CEN;						//使能TMR3
	)
}
