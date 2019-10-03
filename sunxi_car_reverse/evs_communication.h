#ifndef __EVS_COMMUNICATION_H__
#define __EVS_COMMUNICATION_H__

enum task_info_type {
   NO_INFO = 0,
   ACTUAL_GEAR_INFO = 1,
   STEERING_ANGLE_INFO = 2,
   RADAR_DISTANCE_INFO = 3,
   PERF_VEHICLE_SPEED_INFO = 4,
   RADAR_SIDE_ZONE_STATE_INFO = 5,    
};

enum car_actual_gear {
	NEUTRAL = 0,
	CAR_1ST_GEAR = 1,
	CAR_2ND_GEAR = 2,
	CAR_3RD_GEAR = 3,
	CAR_4T_GEAR = 4,
	CAR_5T_GEAR = 5,
	CAR_6T_GEAR = 6,
	CAR_7T_GEAR = 7,
	CAR_8T_GEAR = 8,
	REVERSE = 9,
	PARKING = 10,
};

enum car_radar_distance {
	NO_OBSTACLE_DETECTED = 0,
	LESS_THAN_40CM = 1,
	FROM_40CM_TO_100CM = 2,
	FROM_100CM_TO_150CM = 3,
	FROM_40CM_TO_60CM = 3,
};

enum car_steering_angle {
	M0_ANGLE = 0,
	L11_ANGLE = 1,
	L22_ANGLE = 2,
	R11_ANGLE = 3,
	R22_ANGLE = 4,
};

struct parse_mcu_data_s {
	unsigned char *data;
	unsigned char idx;
	unsigned char type[50];	
};

void alloc_parse_mcu_data_memery(void);
void free_parse_mcu_data_memery(void);

int get_smd_state(int *ptr);
int car_reverse_status(int gear);
int parse_data_info(int flag, int *ptr);


#endif /* __EVS_COMMUNICATION_H__ */

