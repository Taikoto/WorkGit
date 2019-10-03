/*
 * mcu_communication.c
 *
 *  Created on: 2019年08月01日
 *      Author: taikoto
 */

#include "evs_communication.h"
#include "ringbuffer.h"
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include "asm/cacheflush.h"



#define DEBUG
#define LOG_TAG "[cameraquickview]"

#ifdef DEBUG
#define KDEBUG(fmt, args...) printk("%s: %s() line: %d "fmt, LOG_TAG, __FUNCTION__, __LINE__, ##args);
#else
#define KDEBUG(fmt, args...)
#endif

#define MIN(x) x-3
#define MAX(x) x+3

int old_steering_angle = 0;

int standard_value[] = {0,11,-11,22,-22,33,-33,45,-45,56,-56,67,-67,78,-78,90,-90,101,-101,112,-112,123,-123,135,-135,\
    146,-146,157,-157,168,-168,180,-180,191,-191,202,-202,213,-213,225,-225,236,-236,247,-247,258,-258,\
	270,-270,281,-281,292,-292,303,-303,315,-315,326,-326,337,-337,348,348,360,-360,371,-371,382,-382,393,-393,405,-405,\
	416,-416,427,-427,438,-438,450,-450,461,-461,467,-467,476,-476,484,-484,493,-493,501,-501,510,-510,518,-518};


struct parse_mcu_data_s *parse_mcu_data;

void alloc_parse_mcu_data_memery(void)
{
    return;
	parse_mcu_data = kmalloc(sizeof(struct parse_mcu_data_s), GFP_KERNEL);
	if (parse_mcu_data == NULL) {
		printk("kmalloc parse_mcu_data %d Bytes memory failed!\n",sizeof(struct parse_mcu_data_s));
		return -1;
	}

	parse_mcu_data->data = kmalloc(sizeof(parse_mcu_data->data), GFP_KERNEL);
	if (parse_mcu_data->data == NULL) {
		printk("kmalloc data %d Bytes memory failed!\n",sizeof(parse_mcu_data->data));
		return -1;
	}
}

void free_parse_mcu_data_memery(void)
{
	return;

	kfree(parse_mcu_data->data);
	kfree(parse_mcu_data);
	parse_mcu_data->data = NULL;
	parse_mcu_data = NULL;
}


int get_smd_state(int *ptr) 
{
	int smd_flag = 0;
    printk("ptr[0] = %d\r\n",ptr[0]);
    switch(ptr[0]) {
		case 1:
			smd_flag = ACTUAL_GEAR_INFO;
	        break;

		case 2:
			smd_flag = STEERING_ANGLE_INFO;
			break;

		case 3:
			smd_flag = RADAR_DISTANCE_INFO;
			break;

		default:
		    smd_flag = NO_INFO;
			break;
	}

	return smd_flag;
}


int car_actual_gear_to_idx(int gear)
{
    int idx = 0;

    if(1 == gear)
		idx = NEUTRAL;
	if(2 == gear)
		idx = REVERSE;
	if(4 == gear)
		idx = PARKING;

	return idx;
}

int car_reverse_status(int gear)
{
    int smd, idx;
	int status = 0;
  
	idx = car_actual_gear_to_idx(gear);

	if(REVERSE == idx) {
		status = 1;
	}

	if(NEUTRAL == idx) {
		status = 2;
	}

	return status;
}

int steering_angle_to_idx(int angle)
{
    int idx = 0;
	int i = 0;

	if(angle > 32767) {
		angle = (int)(angle - 65536)/10;
	} else {
	    angle = (int)(angle/10);
	}
	printk("#####################$$$$$$$$$$$$$ angle = %d $$$$$$$$$$$$$#################\r\n",angle);
	for(i = 0; i < 97; i++) {
		if(angle > MIN(standard_value[i]) && angle <= MAX(standard_value[i])) {
			idx = standard_value[i];
			break;
        }
		if(i==96) 
			idx = old_steering_angle;
	}

    return idx;
}

int radar_distance_to_idx(int distance)
{
    int idx = 0;
	if(distance == NO_OBSTACLE_DETECTED)
		idx = 0;
	if(distance == LESS_THAN_40CM)
		idx = 1;
	if(distance == FROM_40CM_TO_100CM)
		idx = 2;
	if(distance == FROM_100CM_TO_150CM)
		idx = 3;
	if(distance == FROM_40CM_TO_60CM)
		idx = 3;
	
    return idx;
}

int parse_data_idx[5] = {0};
int parse_data[5] = {0};
//unsigned char parse_data_type[5] = {0};


#if 0
int printk_parse_mcu_data(int num)
{
    int i = 0;

	for(i = 0; i < num; i++) {
        printk("%d  %d  %s\r\n",parse_mcu_data[i].data,parse_mcu_data[i].idx,parse_mcu_data[i].type);
		printk("\r\n");
	}
    
    return 0;
}
#else
int printk_parse_mcu_data(int num)
{
    int i = 0;

	for(i = 0; i < num; i++) {
        printk("%d  %d\r\n",parse_data[i],parse_data_idx[i]);
		printk("\r\n");
	}
    
    return 0;
}
#endif


int parse_data_info(int flag, int *ptr)
{
    int tmp = 0;

	switch(flag) {
		case ACTUAL_GEAR_INFO:
			//printk("ACTUAL_GEAR_INFO %d\r\n",ptr[1]);
		    parse_data_idx[0] = car_actual_gear_to_idx(ptr[1]);
	        break;
		case STEERING_ANGLE_INFO:
			//printk("STEERING_ANGLE_INFO %d\r\n",ptr[2]);
		    parse_data_idx[1] = steering_angle_to_idx(ptr[2]);
		    old_steering_angle = parse_data_idx[1];
			break;
		case RADAR_DISTANCE_INFO:
			//printk("RADAR_DISTANCE_INFO %d %d %d\r\n",ptr[3],ptr[4],ptr[5]);
		    parse_data_idx[2] = radar_distance_to_idx(ptr[3]);
			parse_data_idx[3] = radar_distance_to_idx(ptr[4]);
			parse_data_idx[4] = radar_distance_to_idx(ptr[5]);
			break;
		default:
			printk("default\r\n");
		    break;
	}

	//printk_parse_mcu_data(5);
	
    return ptr[0];
}

