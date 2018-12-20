#ifndef __74LS164_H__
#define __74LS164_H__

#define SEG_PORT_P0           P0_DOUT

#define LS164_DATA_PIN      4
#define LS164_CLK_PIN       5

#define LS164_DATA(x)      {if((x))P0_DOUT|=1UL<<LS164_DATA_PIN; \
                            else P0_DOUT&=~(1UL<<LS164_DATA_PIN);} 
							        
#define LS164_CLK(x)       {if((x))P0_DOUT|=1UL<<LS164_CLK_PIN ; \
                            else P0_DOUT&=~(1UL<<LS164_CLK_PIN);} 

EXTERN_C VOID LS164Init(VOID);
EXTERN_C VOID LS164Send_B2(UINT8 d);


#endif

