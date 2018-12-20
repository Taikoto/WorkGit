#ifndef __LOCK_H__
#define __LOCK_H__

extern UINT8 lock_cmd;

#define LOCK_GATE_PIN      2     //P3.2 

#define POWER_CONTROL_PIN  0     //P2.0

#define GUN_STATUS_PIN     3     //P3.3


													
#define LOCK_DATA(x)      {if((x))P3_DOUT|=1UL<<LOCK_GATE_PIN; \
														else P3_DOUT&=~(1UL<<LOCK_GATE_PIN);} 

#define POWER_DATA(x)     {if((x))P2_DOUT|=1UL<<POWER_CONTROL_PIN; \
                            else P2_DOUT&=~(1UL<<POWER_CONTROL_PIN);}

#define GUN_DATA(x)      {if((x))P3_DOUT|=1UL<<GUN_STATUS_PIN; \
														else P3_DOUT&=~(1UL<<GUN_STATUS_PIN);}
														
EXTERN_C void LockInit(void);														
EXTERN_C int gun_status(void);
EXTERN_C int lock_status(void);
EXTERN_C int lock_control_pin_status(void);
EXTERN_C void lock_open(void);
EXTERN_C void lock_close(void);

#endif
