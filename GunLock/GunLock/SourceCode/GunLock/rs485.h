#ifndef __RS485_H__
#define __RS485_H__

typedef struct _RS485_PACKET
{
	 UINT8 Buf[32];
	 UINT8 Cnt;
}RS485_PACKET;


#define UART_485_PIN       1     //P2.1

EXTERN_C BOOL g_bRS485RxEnd;
EXTERN_C RS485_PACKET g_StRS485Packet;
EXTERN_C RS485_PACKET RS485Packet_send;

/*wait the send data over*/
#define	WAIT_RS485_S_END()	{Delayms(1);}
/*wait the rev data over*/
#define WAIT_RS485_R_END()	{while(!g_bRS485RxEnd);Delayms(1);}
#define SET_RS485_R_FLAG(x)	(g_bRS485RxEnd=(x))

/*enable RS485 send data*/
#define	EN_RS485_S()				{P2_DOUT &= ~(1UL<< UART_485_PIN);}
/*enable RS485 rev data*/
#define	EN_RS485_R()				{P2_DOUT |= (1UL<< UART_485_PIN);}


EXTERN_C VOID InitRS485_DR(void);
EXTERN_C VOID UartSend(UINT8 *pBuf,UINT32 unNumOfBytes);
EXTERN_C VOID UartInit(UINT32 unFosc,UINT32 unBaud);

EXTERN_C void rs485_send_packet_clean(void);
EXTERN_C void rs485_packet_addr(void);
EXTERN_C void clean_rev_buf(void);


#endif

