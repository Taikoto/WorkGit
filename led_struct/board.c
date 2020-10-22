#include <stdio.h>
#include "led_operations.h"


static int board_led_init (int which) /* 初始化LED, which-哪个LED */	   
{
	
	printf("%s %s line %d, led %d\n", __FILE__, __FUNCTION__, __LINE__, which);
	return 0;
}

static int board_led_ctl (int which, char status) /* 控制LED, which-哪个LED, status:1-亮,0-灭 */
{
	printf("%s %s line %d, led %d, %s\n", __FILE__, __FUNCTION__, __LINE__, which, status ? "on" : "off");
	return 0;
}

static struct led_operations board_led_opr = {
	.init = board_led_init,
	.ctl  = board_led_ctl,
};

struct led_operations *get_board_led_opr(void)
{
	return &board_led_opr;
}
