#ifndef __I2C_H__
#define __I2C_H__


#define	SDA_PIN	4		//P3.4	SDA
#define	SCL_PIN		5		//p3.5	SCL

#define EEPROM_SLA              0xA0
#define EEPROM_WR               0x00
#define EEPROM_RD               0x01

#define I2C_CLOCK               13
#define PAGE_SIZE               8

#define	SDA(x)					{if((x)) P3_DOUT	|= 1UL<<SDA_PIN;\
													else	P3_DOUT &= ~(1UL<<SDA_PIN);}
																		
#define	SCL(x)					{if((x))	P3_DOUT	|=	1UL<<SCL_PIN;\
													else	P3_DOUT	&=	~(1UL<<SCL_PIN);}


EXTERN_C BOOL I2CWrite(UINT32 unAddr,UINT8 *pucData,UINT32 unLength);
EXTERN_C BOOL I2CRead(UINT32 unAddr,UINT8 *pucData,UINT32 unLength);
EXTERN_C VOID I2CInit(VOID);
EXTERN_C void Timed_Write_Cycle(void);


#endif

