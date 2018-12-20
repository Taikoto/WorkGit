#ifndef __LED_H__
#define __LED_H__


#define LED_GATE_PIN       2     //P2.2
#define RUN_LED_PIN        0  //P4.0

#define LED_DATA(x)      {if((x))P2_DOUT|=1UL<<LED_GATE_PIN; \
                            else P2_DOUT&=~(1UL<<LED_GATE_PIN);} 	

#define RUN_LED(x)       {if((x))P4_DOUT|=1UL<<RUN_LED_PIN; \
														else P4_DOUT&=~(1UL<<RUN_LED_PIN);}
														
EXTERN_C void LedInit(void);
EXTERN_C void lock_led_on(void);
EXTERN_C void lock_led_off(void);

#endif
