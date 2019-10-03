/*
 * mcu_communication.c
 *
 *  Created on: 2019年08月01日
 *      Author: taikoto
 */

#include "mcu_communication.h"
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

int get_smd_state(unsigned char *buf) 
{
	int smd_flag = 0;

    switch(buf[1]) {
		case SMD_GEAR_SELECTION:
			smd_flag = ACTUAL_GEAR_INFO;
	        break;

	    case SMD_PERF_VEHICLE_SPEED:
			smd_flag = PERF_VEHICLE_SPEED_INFO;
			break;

		case SMD_STEERING_ANGLE:
			smd_flag = STEERING_ANGLE_INFO;
			break;

		case SMD_RADAR_DISTANCE_INFO:
			smd_flag = RADAR_DISTANCE_INFO;
			break;

		case SMD_RADAR_SIDE_ZONE_STATE:
			smd_flag = RADAR_SIDE_ZONE_STATE_INFO;
			break;

		default:
		    smd_flag = NO_INFO;
			break;
	}

	return smd_flag;
}

int car_actual_gear_to_idx(unsigned char *ptr)
{
    int idx = 0;

    if((0x00 == ptr[3])&&(0x01 == ptr[4]))
		idx = NEUTRAL;
	if((0x00 == ptr[3])&&(0x10 == ptr[4]))	
		idx = CAR_1ST_GEAR;
	if((0x00 == ptr[3])&&(0x20 == ptr[4]))
		idx = CAR_2ND_GEAR;
	if((0x00 == ptr[3])&&(0x40 == ptr[4]))
        idx = CAR_3RD_GEAR;
	if((0x00 == ptr[3])&&(0x80 == ptr[4]))
		idx = CAR_4T_GEAR;
	if((0x10 == ptr[3])&&(0x00 == ptr[4]))
		idx = CAR_5T_GEAR;
	if((0x20 == ptr[3])&&(0x00 == ptr[4]))
		idx = CAR_6T_GEAR;
	if((0x40 == ptr[3])&&(0x00 == ptr[4]))
		idx = CAR_7T_GEAR;
	if((0x80 == ptr[3])&&(0x00 == ptr[4]))
		idx = CAR_8T_GEAR;
	if((0x00 == ptr[3])&&(0x02 == ptr[4]))
		idx = REVERSE;
	if((0x00 == ptr[3])&&(0x04 == ptr[4]))
		idx = PARKING;

	return idx;
}

int car_reverse_status(unsigned char *buf)
{
    int smd, idx;
	int status = 0;
    smd = get_smd_state(buf);
	idx = car_actual_gear_to_idx(buf);

	if((ACTUAL_GEAR_INFO == smd) && (REVERSE == idx)) {
		status = 1;
	}

	if((ACTUAL_GEAR_INFO == smd) && (NEUTRAL == idx)) {
		status = 2;
	}

	return status;
}

int steering_angle_to_idx(int angle)
{
    int idx = 0;

	if(angle > 32767) {
		angle = (angle - 65536);
	} else {
	    angle = angle;
	}
	
	if(angle > 0 && angle <= 150)
		idx = 0;
    if(angle > 150 && angle <= 300)
		idx = 1;
    if(angle > 450 && angle <= 600)
		idx = 2;
    if(angle > 600 && angle <= 750)
		idx = 3;
    if(angle > 750 && angle <= 900)
		idx = 4;

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

unsigned char parse_data_idx[5] = {0};
unsigned char parse_data[5] = {0};
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

#if 0
unsigned char parse_data_info(int flag, unsigned char *ptr)
{
    struct parse_mcu_data_s data[5] = {{0,0,"actual_gear"},{0,0,"steering_angle"},{0,0,"radar_r_distance"},{0,0,"radar_m_distance"},{0,0,"radar_l_distance"}};
    int tmp = 0;	
	
	parse_mcu_data = data;
	
	tmp = ptr[3];
	tmp = (tmp << 8) | ptr[4];
	
    switch(flag) {
		case ACTUAL_GEAR_INFO:
			printk("ACTUAL_GEAR_INFO %d %d\r\n",ptr[4],ptr[3]);
	        parse_mcu_data[0].data = tmp;
		    parse_mcu_data[0].idx = 0;
	        break;
		case STEERING_ANGLE_INFO:
			printk("STEERING_ANGLE_INFO %d %d\r\n",ptr[4],ptr[3]);
			parse_mcu_data[1].data = tmp;
		    parse_mcu_data[1].idx = steering_angle_to_idx(tmp);
			break;
		case RADAR_DISTANCE_INFO:
			printk("RADAR_DISTANCE_INFO %d %d %d\r\n",ptr[11],ptr[10],ptr[9]);
			parse_mcu_data[2].data = ptr[11];
		    parse_mcu_data[2].idx = radar_distance_to_idx(ptr[11]);
		    parse_mcu_data[3].data = ptr[10];
			parse_mcu_data[3].idx = radar_distance_to_idx(ptr[10]);
		    parse_mcu_data[4].data = ptr[9];
			parse_mcu_data[4].idx = radar_distance_to_idx(ptr[9]);
			break;
		case PERF_VEHICLE_SPEED_INFO:
			printk("PERF_VEHICLE_SPEED_INFO %d %d\r\n",ptr[4],ptr[3]);
			break;
		case RADAR_SIDE_ZONE_STATE_INFO:
			printk("RADAR_SIDE_ZONE_STATE_INFO %d %d\r\n",ptr[4],ptr[3]);
			break;
		default:
			printk("default\r\n");
		    break;
	}

	printk_parse_mcu_data(5);
	
    return ptr[1];
}
#else
unsigned char parse_data_info(int flag, unsigned char *ptr)
{
    int tmp = 0;	
	
	tmp = ptr[3];
	tmp = (tmp << 8) | ptr[4];
	
    switch(flag) {
		case ACTUAL_GEAR_INFO:
			printk("ACTUAL_GEAR_INFO %d %x %x\r\n",tmp,ptr[3],ptr[4]);
	        parse_data[0] = tmp;
		    parse_data_idx[0] = car_actual_gear_to_idx(ptr);
			//parse_data_type[0] = "actual_gear";
	        break;
		case STEERING_ANGLE_INFO:
			printk("STEERING_ANGLE_INFO %d %x %x\r\n",tmp,ptr[3],ptr[4]);
			parse_data[1] = tmp;
		    parse_data_idx[1] = steering_angle_to_idx(tmp);
			//parse_data_type[1] = "steering_angle";
			break;
		case RADAR_DISTANCE_INFO:
			printk("RADAR_DISTANCE_INFO %d %d %d\r\n",ptr[11],ptr[10],ptr[9]);
			parse_data[2] = ptr[11];
		    parse_data_idx[2] = radar_distance_to_idx(ptr[11]);
			//parse_data_type[2] = "radar_r_distance";
		    parse_data[3] = ptr[10];
			parse_data_idx[3] = radar_distance_to_idx(ptr[10]);
			//parse_data_type[3] = "radar_m_distance";
		    parse_data[4] = ptr[9];
			parse_data_idx[4] = radar_distance_to_idx(ptr[9]);
			//parse_data_type[4] = "radar_l_distance";
			break;
		case PERF_VEHICLE_SPEED_INFO:
			printk("PERF_VEHICLE_SPEED_INFO %d %d\r\n",ptr[4],ptr[3]);
			break;
		case RADAR_SIDE_ZONE_STATE_INFO:
			printk("RADAR_SIDE_ZONE_STATE_INFO %d %d\r\n",ptr[4],ptr[3]);
			break;
		default:
			printk("default\r\n");
		    break;
	}

	//printk_parse_mcu_data(5);
	
    return ptr[1];
}

#endif
