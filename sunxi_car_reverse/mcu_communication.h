#ifndef __MCU_COMMUNICATION_H__
#define __MCU_COMMUNICATION_H__

//#include "base_types.h"

//#include "cameraquickview.h"

#define CMD_VEHICLE_INFO                     0X20

/*@start========REVERSE CMD========== */

#define SMD_GEAR_SELECTION                   0X1E //档位状态

#define SMD_STEERING_ANGLE                   0X11 //方向盘转角

#define SMD_RADAR_DISTANCE_INFO              0X1F //雷达信息


#define SMD_PERF_VEHICLE_SPEED               0X0F //车速

#define SMD_RADAR_SIDE_ZONE_STATE            0X20 //雷达扇区状态


#define SMD_REVERSE_INFO                     0X19
#define SMD_REVERSE_STATUS                   0X00
#define REVERSE_STATUS_EXIT                  0X01
#define REVERSE_STATUS_ENTER                 0X02
/*@end========REVERSE CMD========== */

/*@start========SPEED INFO SMD========== */
#define SMD_SPEED_INFO                       0X05
#define SPEED_VALID                          0X0
#define SPEED_INVALID                        0X1
/*@end========SPEED INFO SMD========== */

/*@start========WHEEL CMD========== */
#define SMD_WHEEL_ANGLE                      0X0F
#define WHELL_ANGLE_VALID                    0X0
#define WHELL_ANGLE_INVALID                  0X1
/*@end========WHEEL CMD========== */

/*@start========RADAR CMD========== */
#define SMD_RADAR_INFO                       0X12
#define RADAR_NO_OBSTACLE_DETECTED           0X0
#define RADAR_DISTANCE_LESS_400              0X1
#define RADAR_DISTANCE_400_TO_600            0X2
//不报警
#define RADAR_WARNING_0_0                    0X0
//长鸣区报警
#define RADAR_WARNING_1_0                    0X1
//间歇音1报警（报警75ms，不报警175ms）
#define RADAR_WARNING_75_175                 0X2
//间歇音2报警（报警75ms，不报警425ms）；
#define RADAR_WARNING_75_425                 0X3
//间歇音3报警（预留）；
#define RADAR_WARNING_3                      0X4
//长鸣区前后同时报警（前角报警0.5s，后角报警0.5s）
#define RADAR_WARNING_FRONT_BACK             0X5
//错误3S报警（长鸣3s）
#define RADAR_WARNING_3_SECOND_ERROR         0X6
//系统无错误提示（鸣叫0.5s）（预留）；
#define RADAR_WARNING_NOT_ERROR              0X7
//系统故障
#define RADAR_SYSTEM_NOT_FAILURE             0X0
#define RADAR_SYSTEM_FAILURE                 0X1
/*@end========RADAR CMD========== */

/*@start========MCU BOARD KEY INFO(MCU-->MPU)========== */
#define SMD_BOARD_KEY_INFO                   0X19
#define BOARD_KEY_NO_PUSH                    0X0
#define BOARD_KEY_PUSH                       0X1
#define BOARD_KEY_LONG_PUSH                  0X2
/*@end========MCU BOARD KEY INFO(MCU-->MPU)========== */


//HU坐标显示
#define SMD_SCREEN_COORDINATE                0X23

#define SMD_REQ_VECHILE_STATUS_INFO          0X29

//轨迹线
#define PAC_MODE_NORMAL                      0X0
#define PAC_MODE_HORIZONTAL                  0X1
#define PAC_MODE_CLOSE_GUIDE_LINE            0X2


/*@start=======RESPONSE CMD=============*/
#define CMD_MCU_RESPONSE                     0X8E
#define RESPONES_SUCCESS                     0X00
#define RESPONES_FAIL                        0X01
#define RESPONES_UNKWON_CMD                  0X02
#define RESPONES_PARM_ERROR                  0X03
/*@end=======RESPONSE CMD=============*/


/*@start=======RESPONSE CMD=============*/
#define CMD_MCU_RESPONSE                     0X8E
#define RESPONES_SUCCESS                     0X00
#define RESPONES_FAIL                        0X01
#define RESPONES_UNKWON_CMD                  0X02
#define RESPONES_PARM_ERROR                  0X03
/*@end=======RESPONSE CMD=============*/



/**
 *超速警告
 */
#define CMD_OVER_SPEED_WARNNING              0XEE
#define SMD_OVER_SPEED_AVM                   0X01
#define SMD_OVER_SPEED_BLANDZONE             0X02
#define SMD_UPLOAD_PICTURE                   0X03
#define SMD_DISABLE_TOUCH                    0X04

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

int get_smd_state(unsigned char *buf);
int car_reverse_status(unsigned char *buf);
unsigned char parse_data_info(int flag, unsigned char *ptr);



	/**
     * 把修改的数据数组inData发送给MCU
	 * @param cmd MCU命令
	 * @param smd MCU子命令
	 * @param inData 传入给MCU数据的数组
	 * @param inLen 传入给MCU数据的长度
	 */
	//int sendDataToMcu(int cmd, int smd, uint8_t *inData, int inLen);
	/**
	 * 根据cmd和smd来查询对应的数据，返回数据保存在长度为outLen的数组outData
	 * @param cmd MCU命令
	 * @param smd MCU子命令
	 * @param outData 传入给MCU填充数据的数组
	 * @param outLen 传入给MCU赋值的数据长度
	 */
	//int queryDataFromMcu(int cmd, int smd, uint8_t *outData, int outLen);

	/**
	 * 获取当前倒车状态
	 *
	 * @return true -- car in reversed ; false -- car exit reversed
	 */
	//int getReversedStatus(void);

	/**
	 * 从MCU获取保存的ACC状态
	 */
	//int getAccStatus(void);

	/**
	 * 查询当前雷达信息
	 * @param outData 传入雷达数据的数组（长度为3），MCU查询后把雷达数据填充进去后返回
	 *
	 * @return 设置命令代码， <=0失败，>0成功
	 */
	//int getRadarInfo(uint8_t *outData, int length);


	/**
	 * 打开关闭轨迹引导线
	 *
	 * @param line PAC_MODE_NORMAL -- 打开； PAC_MODE_CLOSE_GUIDE_LINE -- 关闭
	 */
	//int setGuideLine(int line);


#endif /* __MCU_COMMUNICATION_H__ */
