/*---------------------------------------------------------------------------------------------------------*/
/*                                                                                                         */
/* Copyright(c) 2010 Nuvoton Technology Corp. All rights reserved.                                         */
/*                                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdint.h>
#include "M051.h"
#include "Register_Bit.h"
#include "Uart.h"
#include "Macro_SystemClock.h"

/*-----------------------------------------------------------------------------
  Function:		UART_Init                                                                         
                                                                                                         
  Parameter:        																					   	
	        	None                                     
  Returns:                                                                                                
               	None                                                                                      
  Description:                                                                                            
               	Initialize UART0, 115200bps, 8N1.                                    
 *-----------------------------------------------------------------------------*/
void UartInit(uint32_t unFosc,uint32_t unBaud)
{
    /* Step 1. GPIO initial */ 
    P3_MFP &= ~(P31_TXD0 | P30_RXD0);   
    P3_MFP |= (TXD0 | RXD0);    		//P3.0 --> UART0 RX
										//P3.1 --> UART0 TX

    /* Step 2. Enable and Select UART clock source*/
    UART0_Clock_EN;         //UART Clock Enable, APBCLK[16]:1
    UARTClkSource_ex12MHZ;  //UART Clock is ext12MHz, CLKSEL1[25,24]: 00
    CLKDIV &= ~(15<<8); 	//UART Clock DIV Number = 0;

    /* Step 3. Select Operation mode */
    IPRSTC2 |= UART0_RST;   //Reset UART0
    IPRSTC2 &= ~UART0_RST;  //Reset end
    UA0_FCR |= TX_RST;      //Tx FIFO Reset
    UA0_FCR |= RX_RST;      //Rx FIFO Reset


    UA0_LCR &= ~PBE;     	//Parity Bit Disable
	UA0_LCR &= ~WLS;
    UA0_LCR |= WL_8BIT;     //8 bits Data Length 
    UA0_LCR &= NSB_ONE;     //1 stop bit

    /* Step 4. Set BaudRate to 115200*/
    UA0_BAUD |= DIV_X_EN;   //Mode2:DIV_X_EN = 1
    UA0_BAUD |= DIV_X_ONE;  //Mode2:DIV_X_ONE =1
    
    /* For XTAL = 12 MHz */
    UA0_BAUD |= ((unFosc / unBaud) -2);	//Set BaudRate to 115200;  UART_CLK/(A+2) = 115200, UART_CLK=12MHz
    
    /* For XTAL = 11.0592 MHz */
    //UA0_BAUD |= ((11059200 / 115200) -2); //Set BaudRate to 115200;  UART_CLK/(A+2) = 115200, UART_CLK=12MHz

	/* Enable Interrupt */
	//UA0_IER	|= (RDA_IEN	| THRE_IEN | RLS_IEN);

	//NVIC_ISER |= UART0_INT;	
}

/*-----------------------------------------------------------------------------
  Function:		Send_Data_To_PC                                                                         
                                                                                                         
  Parameter:        																					   	
	        	c:	char is going to send to PC.                                     
  Returns:                                                                                                
               	None                                                                                      
  Description:                                                                                            
               	Sent a char to PC.                                    
 *-----------------------------------------------------------------------------*/
void Send_Data_To_PC (uint8_t c)
{
    UA0_THR = (uint8_t) c;
    while ((UA0_FSR&TX_EMPTY) != 0x00); //check Tx Empty
}

/*-----------------------------------------------------------------------------
  Function:		Receive_Data_From_PC                                                                         
                                                                                                         
  Parameter:        																					   	
	        	None                                     
  Returns:                                                                                                
               	A char                                                                                      
  Description:                                                                                            
               	Receive a char from PC.                                    
 *-----------------------------------------------------------------------------*/
uint8_t Receive_Data_From_PC(void)
{
    while ((UA0_FSR&RX_EMPTY) != 0x00); //check Rx Empty
    return ((uint8_t)UA0_RBR);
}


