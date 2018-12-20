#include "SmartM_M0.h"
#include "led.h"

void LedInit(void)
{
	P2_PMD &= ~(3UL<<(LED_GATE_PIN <<1));//P2.2
	P2_PMD |= 1UL<<(LED_GATE_PIN <<1);//IO模式，P2.2设置为输出模式
	
	P4_PMD	&=	~(3UL	<<(RUN_LED_PIN	<<	1));	//IO 模式控制，P4.0
	P4_PMD |= 1UL<<(RUN_LED_PIN <<1); //IO模式，P4.0设置为输出模式
}


void lock_led_on(void)//the led on the board
{
	LED_DATA(0);
}


void lock_led_off(void)//the led on the board
{
	LED_DATA(1);
}
