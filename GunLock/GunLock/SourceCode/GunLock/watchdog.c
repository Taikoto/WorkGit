#include "SmartM_M0.h"
#include "watchdog.h"

/****************************************
*函数名称:WatchDogInit
*输    入:无
*输    出:无
*功    能:看门狗初始化
******************************************/
VOID WatchDogInit(VOID)
{
	PROTECT_REG
	(	
	  /* 使能看门狗时钟 */	
		APBCLK |= WDT_CLKEN;		   
	
		/* 设置看门狗时钟源为10K */
		CLKSEL1 = (CLKSEL1 & (~WDT_CLK)) | WDT_10K;
					
		/* 使能看门狗定时器复位功能 */
		WTCR |= WTRE;	
	
		/* 设置看门狗超时间隔为1740.8ms */
		WTCR &= ~WTIS;
		WTCR |= TO_2T14_CK;	//(2^14+1024)*(1000000/10000)=17408*100=1740800us=1.7408s
	
		/* 使能看门狗中断 */			
		WTCR |= WTIE;
		NVIC_ISER |= WDT_INT;
	
		/* 使能看门狗 */
		WTCR |= WTE;
	
		/* 复位看门狗计数值 */
		WTCR |= CLRWTR;		
	)	
}
