#include <stdio.h>
#include "led_operations.h"


struct led_operations *p_led_opr = NULL;

int main(int argc, char **argv)
{
    p_led_opr = get_board_led_opr();
    p_led_opr->init(10);
    p_led_opr->ctl(10, 0);

    return 0;
}
