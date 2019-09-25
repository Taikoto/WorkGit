#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#define DEV_NAME "fast_car_reverse"
#define DEV_PATH "/dev/fast_car_reverse"
#define REVERSE_IOC_MAGIC  'z'

struct mcu_data_t{
      int smd_info;
      int actual_gear;
      int steering_angle;
      int r_radar_distance;
      int m_radar_distance;
      int l_radar_distance;
};

#define REVERSE_IOC_TEST_DATA  _IOW(REVERSE_IOC_MAGIC, 1, int *)
#define REVERSE_IOC_MCU_DATA  _IOW(REVERSE_IOC_MAGIC, 2, struct mcu_data_t *)
#define REVERSE_IOC_RMCU_DATA  _IOR(REVERSE_IOC_MAGIC, 3, struct mcu_data_t *)


struct mcu_data_t car_mcu_data;

enum task_info_type {
   NO_INFO = 0,
   ACTUAL_GEAR_INFO = 1,
   STEERING_ANGLE_INFO = 2,
   RADAR_DISTANCE_INFO = 3,    
};

enum parameter_type {
	PT_PROGRAM_NAME = 0,
        SMD_INFO,
	ACTUAL_GEAR,
	STEERING_ANGLE,
	R_RADAR_DISTANCE,
	M_RADAR_DISTANCE,
	L_RADAR_DISTANCE,
	PT_NUM
};

void usage(void)
{
	printf("you should input as:\n");
	printf("\t select_test[smd][actual_gear][steering_angle][r_radar_distance][m_radar_distance][l_radar_distance]\n");
}


int main(int argc, char **argv) 
{
	int ret = 0;
	int fp = 0;
        int prama = 0;
        prama = 100;
	
	if(argc != PT_NUM) {
		usage();
		return -1;
	}
	
#if 0	
	int ramdata[6];
	memset(ramdata,0x07, 6* sizeof(int));
	car_mcu_data.addr = 1;
	car_mcu_data.len = 2;
	car_mcu_data.prv =(int *) ramdata;
#endif
        car_mcu_data.smd_info = 0;
	car_mcu_data.actual_gear = 0;
	car_mcu_data.steering_angle = 0;
	car_mcu_data.r_radar_distance = 0;
	car_mcu_data.m_radar_distance = 0;
	car_mcu_data.l_radar_distance = 0;
        
        car_mcu_data.smd_info = atoi(argv[SMD_INFO]);
	car_mcu_data.actual_gear = atoi(argv[ACTUAL_GEAR]);
	car_mcu_data.steering_angle = atoi(argv[STEERING_ANGLE]);
	car_mcu_data.r_radar_distance = atoi(argv[R_RADAR_DISTANCE]);
	car_mcu_data.m_radar_distance = atoi(argv[M_RADAR_DISTANCE]);
	car_mcu_data.l_radar_distance = atoi(argv[L_RADAR_DISTANCE]);
	
	printf("mcu data ioctl test...\n");

	if ((ret = (fp = open(DEV_PATH, O_RDWR))) < 0) 
	{
		printf("ioctl test error retcode = %d\n", ret);
		exit(0);
	}

	//ret = ioctl(fp,REVERSE_IOC_TEST_DATA,&prama);
	//printf("ret = %d\r\n",ret);
	//printf("prama = %d\n",prama);
	
	ret = ioctl(fp,REVERSE_IOC_MCU_DATA,&car_mcu_data);//11111
	printf("ret = %d\r\n",ret);
	
	//ret = ioctl(fp,REVERSE_IOC_RMCU_DATA,&car_mcu_data);
	//printf("ret = %d\r\n",ret);
 
	printf("this is kernel data:  22222\r\n");
        printf("smd_info = %d\n",car_mcu_data.smd_info);
	printf("actual_gear = %d\n",car_mcu_data.actual_gear);
	printf("steering_angle = %d\n",car_mcu_data.steering_angle);
	printf("r_radar_distance = %d\n",car_mcu_data.r_radar_distance);
	printf("m_radar_distance = %d\n",car_mcu_data.m_radar_distance);
	printf("l_radar_distance = %d\n",car_mcu_data.l_radar_distance);

	return ret;
}
