#ifndef __HX711_H__
#define __HX711_H__

#define D_DATA_PIN  3  //P2.3
#define D_CLK_PIN   4  //P2.4

#define	DOUT(x)					{if((x)) P2_DOUT	|= 1UL<<D_DATA_PIN;\
													else	P2_DOUT &= ~(1UL<<D_DATA_PIN);}
																		
#define	SCK(x)					{if((x))	P2_DOUT	|=	1UL<<D_CLK_PIN;\
													else	P2_DOUT	&=	~(1UL<<D_CLK_PIN);}

EXTERN_C void Hx711Init(void);
EXTERN_C unsigned long HX711_Read(void);
												
												
#endif
