#include "SmartM_M0.h"
#include "HX711.h"

void Hx711Init(void)
{
	P2_PMD	&=	~(3UL	<<(D_DATA_PIN	<<	1));	//IO 模式控制，P2.3
	P2_PMD	|=	(3UL	<<(D_DATA_PIN <<	1));	//IO模式，P2.3设置为双向模式
	
	P2_PMD	&=	~(3UL	<<	(D_CLK_PIN	<<	1));	//IO	模式控制，P2.4
	P2_PMD	|=	(1UL <<(D_CLK_PIN	<<	1));				//IO模式，P2.4设置为输出模式
}


unsigned long HX711_Read(void)	//增益128
{
		
		unsigned long count; 
		unsigned char i; 

		Delayus(8);
  	SCK(0); 
  	count=0; 
	
  	while((P2_PIN &	(1 << 3))); 
  	for(i=0;i<24;i++)
		{ 
				SCK(1); 
				count=count<<1; 
				SCK(0); 
				if((P2_PIN	&	(1 << 3)))
				{
					count++; 
				}
				
		}
	
		SCK(1); 
    count=count^0x800000;//第25个脉冲下降沿来时，转换数据
		//Delayus(10);
		SCK(0);  
		return(count);
}








