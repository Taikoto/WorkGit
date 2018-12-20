#include "SmartM_M0.h"
#include "rs485.h"


BOOL g_bRS485RxEnd = TRUE;
RS485_PACKET g_StRS485Packet = {0};
RS485_PACKET RS485Packet_send = {0};



/****************************************
*函数名称:UartInit
*输    入:unFosc 	 晶振频率
          unBaud	 波特率
*输    出:无
*功    能:串口初始化
******************************************/
VOID UartInit(UINT32 unFosc,UINT32 unBaud)
{
	P3_MFP &= ~(P31_TXD0 | P30_RXD0);   
  P3_MFP |= (TXD0 | RXD0);    		//P3.0 使能为串口0接收  P3.1 使能为串口0发送
										
  UART0_Clock_EN;         //串口0时钟使能
  UARTClkSource_ex12MHZ;  //串口时钟选择为外部晶振
  CLKDIV &= ~(15<<8); 	//串口时钟分频为0

	IPRSTC2 |= UART0_RST;   //复位串口0
  IPRSTC2 &= ~UART0_RST;  //复位结束
  UA0_FCR |= TX_RST;      //发送FIFO复位
  UA0_FCR |= RX_RST;      //接收FIFO复位
	
	UA0_LCR &= ~PBE;     	//校验位功能取消
	UA0_LCR &= ~WLS;
  UA0_LCR |= WL_8BIT;     //8位数据位
  UA0_LCR &= NSB_ONE;     //1位停止位

	UA0_BAUD |= DIV_X_EN|DIV_X_ONE;   //设置波特率分频
    
  UA0_BAUD |= ((unFosc / unBaud) -2);	//波特率设置  UART_CLK/(A+2) = 115200, UART_CLK=12MHz 

	UA0_IER	|= RDA_IEN;					//接收数据中断使能
	NVIC_ISER |= UART0_INT;	 			//使能串口0中断
}


VOID InitRS485_DR(void)
{
	/*翻转脚初始化*/
	P2_PMD &= ~(3UL<<(UART_485_PIN<<1));//IO模式控制,P2.1
	P2_PMD |= 1UL<<(UART_485_PIN<<1);//IO模式，P2.1设置为输出模式
	
  //UA0_FUN_SEL = RS485_EN;        //设置为485功能  
  //UA0_FCR |= (RX_DIS);           //禁止接收器接 
	//UA0_RS485_CSR |= RS485_NMM;    //设置为485普通操作模式  
  //UA0_RS485_CSR |= RS485_AUD;    //设置为控制自动方向模
	///* 开启UART0中断 */  
  //UA0_IER |= RDA_IEN;           //开启可接受数据中断和  
  //UA0_IER |= RLS_IEN;           //接收器上中断状态使能,是一个错误中断，见M0手册
}


/****************************************
*函数名称:UartSend
*输    入:pBuf 			 发送数据缓冲区
          unNumOfBytes	 发送字节总数
*输    出:无
*功    能:串口发送数据
******************************************/
VOID UartSend(UINT8 *pBuf,UINT32 unNumOfBytes)
{
	UINT32 i;
	/*wait the rs485 rev data over*/
	WAIT_RS485_R_END();
	/*enable rs485 send data */
	EN_RS485_S();
		
	for(i=0; i<unNumOfBytes; i++)
	{	
		/*move data to send rigister*/
		UA0_THR = *(pBuf+i);
					
    while ((UA0_FSR&TX_EMPTY) == 0x00); //检查发送FIFO是否为空	 
	}

	/*wait rs485 send data over	*/
	WAIT_RS485_S_END();
		
	/*enable rs485 rev data */
	EN_RS485_R();
	Delayms(5);
}


void clean_rev_buf(void)
{
	/*clean the rev buf*/
	memset(g_StRS485Packet.Buf, 0, sizeof(g_StRS485Packet.Buf));
  /*set count zero  to rev next data count*/
	g_StRS485Packet.Cnt = 0;
}


void rs485_send_packet_clean(void)
{
	/*clean the rev buf*/
	memset(RS485Packet_send.Buf, 0, sizeof(RS485Packet_send.Buf));
	/*set count zero  to rev next data count*/
	RS485Packet_send.Cnt = 0;
}


void rs485_packet_addr(void)
{
	rs485_send_packet_clean();
		
	/*目标地址*/
	RS485Packet_send.Buf[RS485Packet_send.Cnt++] = 85;	
}
