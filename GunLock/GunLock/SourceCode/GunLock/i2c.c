#include "SmartM_M0.h"
#include "i2c.h"

/*****************************************
*函数名称:Timed_Write_Cycle
*输    入:无
*输    出:无
*功    能:同步的写周期
******************************************/
void Timed_Write_Cycle(void)                   
{
	while (I2STATUS != 0x18)
  {
		//启动
    I2CON |= STA;                          
    I2CON |= SI; 
    while ((I2CON & SI) != SI);
    I2CON &= ((~STA) & (~SI));

		//设备地址
    I2DAT = EEPROM_SLA | EEPROM_WR;
    I2CON |= SI; 
    while ((I2CON & SI) != SI);
  }
		
	//停止
  I2CON |= STO;
  I2CON |= SI;
  while (I2CON & STO);
}



/*****************************************
*函数名称:I2CInit
*输    入:无
*输    出:无
*功    能:I2C初始化
******************************************/
VOID I2CInit(VOID)
{
	P3_PMD &= ~(Px4_PMD | Px5_PMD);
	P3_PMD |= (Px4_OD | Px5_OD);               	//使能I2C0引脚

  P3_MFP &= ~(P34_T0_I2CSDA | P35_T1_I2CSCL);
	P3_MFP |= (I2CSDA | I2CSCL);     			      //选择P3.4,P3.5作为I2C0功能引脚   
    
	APBCLK |= I2C0_CLKEN;                       //使能I2C0时钟
  /*baud rate 214KHz   12000000/[4*(13+1)]*/
  I2CLK = I2C_CLOCK;
  /*波特率设置为 125KHz/s  50/[4*(99+1)]*/
		
  I2CON |= ENSI;                             	//使能I2C
}


/*****************************************
*函数名称:I2CWrite
*输    入:unAddr   写地址
          pucData  写数据
		      unLength 写长度
*输    出:TRUE/FALSE
*功    能:AT24C0X写数据
******************************************/
BOOL I2CWrite(UINT32 unAddr,UINT8 *pucData,UINT32 unLength)
{
	UINT32 i;

  I2CON |= STA;          				//启动
  I2CON |= SI;                       	

  while ((I2CON & SI) != SI);	       

  I2CON &= ((~STA)&(~SI));           	

  if (I2STATUS != 0x08)	            
  {
		return FALSE;
  }

  //进入读写控制操作 
  I2DAT = EEPROM_SLA | EEPROM_WR;
  I2CON |= SI;
  while ((I2CON & SI) != SI);

  if (I2STATUS != 0x18)              
  {
		return FALSE;
  }

	//写地址
  I2DAT = unAddr;
  I2CON |= SI; 
  while ((I2CON & SI) != SI);
  if (I2STATUS != 0x28)              
  {
	  return FALSE;
  }
	
	//写数据
  for(i=0; i<unLength; i++)
	{
		I2DAT = *(pucData+i);
		I2CON |= SI; 
		while ((I2CON & SI) != SI);
		if (I2STATUS != 0x28)              
		{
			return FALSE;
		}	
	}

  //停止
  I2CON |= STO;
  I2CON |= SI;
  while (I2CON & STO);               

  Timed_Write_Cycle();               
                                       							
	return TRUE;												 
}


/*****************************************
*函数名称:I2CRead
*输    入:unAddr   读地址
          pucData  读数据
		      unLength 读长度
*输    出:TRUE/FALSE
*功    能:AT24C0X读数据
******************************************/
BOOL I2CRead(UINT32 unAddr,UINT8 *pucData,UINT32 unLength)
{
	UINT32 i;

  I2CON |= STA;          			    //启动
  I2CON |= SI;                       	

  while ((I2CON & SI) != SI);	        

  I2CON &= ((~STA)&(~SI));           	

  if (I2STATUS != 0x08)	            
  {
		return FALSE;
  }
		
  //进入读写控制操作  
  I2DAT = EEPROM_SLA | EEPROM_WR;
  I2CON |= SI;
  while ((I2CON & SI) != SI);

  if (I2STATUS != 0x18)              
  {
		return FALSE;
  }

	//写入读地址
  I2DAT = unAddr;
  I2CON |= SI; 
  while ((I2CON & SI) != SI);
  if (I2STATUS != 0x28)              
  {
		return FALSE;
  }
	
  // 重新启动
  I2CON |= STA;          
  I2CON |= SI;                               	
  while ((I2CON & SI) != SI);                	
  I2CON &= ((~STA)&(~SI));                   	

  if (I2STATUS != 0x10)                      	
  {
		return FALSE;
  }

  //进入读操作  
  I2DAT = EEPROM_SLA | EEPROM_RD;
  I2CON |= SI;
  while ((I2CON & SI) != SI);	    

  if (I2STATUS != 0x40)              
  {
		while (1);
  }

  //读取数据
	I2CON |= AA;

  for(i=0; i<unLength; i++)
	{
		I2CON |= SI; 
		while ((I2CON & SI) != SI);
		
	  if (I2STATUS != 0x50)              
	  {
			return FALSE;
		}
		
		*(pucData+i) = I2DAT;	
	} 

  //发送NACK到AT24C02，执行断开连接操作
  I2CON &= (~AA);
  I2CON |= SI; 
  while ((I2CON & SI) != SI);
        
  //停止
  I2CON |= STO;
  I2CON |= SI;
  while (I2CON & STO);                      

	return TRUE;
}
