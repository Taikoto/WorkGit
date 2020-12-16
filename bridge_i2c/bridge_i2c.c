/*************************************************************************************
帧头   数据方向   返回帧长度   数据长度   数据   Checksum
1Byte  1Byte      1Byte        1Byte      NByte  1Byte

帧头：0x5A
数据方向：0x05，主机发送到显示屏；0x06：显示屏反馈到主机；
数据（N Byte）中 N 的值为帧格式中的数据长度的值；

1. 置位显示屏升级标志位。
2. 查询当前 Bootloader 状态
主机发送：5A 05 0B 06 B0 01 01 00 00 00 22（查询当前 Bootloader 状态）
显示屏返回：5A 06 0B 06 B0 02 01 01 00 00 25（Bootloader 当前处于 Ready 状
态，表示 Bootloader 进入成功）
或 5A 06 0B 06 B0 02 01 02 00 00 26（Bootloader 当前处于 Blank 状态，表示擦除
成功）
或 5A 06 0B 06 B0 02 01 03 00 00 27（Bootloader 当前处于 Programming 状态，
表示进入编程模式成功）
或 5A 06 0B 06 B0 02 01 04 00 00 28（Bootloader 当前处于 Programme
Completed 状态）跳至第 7 步。无响应则执行下一步。
3. 申请进入 Bootloader 权限
主机发送：5A 05 07 02 31 01 9A
显示屏返回：5A 06 07 02 31 01 9B
4. 输入 Access Key 1
主机发送：5A 05 07 04 31 03 A5 5A 9D
显示屏返回：5A 06 07 02 31 03 9D
5. 输入 Access Key 2
主机发送：5A 05 07 04 31 04 C3 3C 9E
显示屏返回：5A 06 07 02 31 04 9E
6. 等待 2s，查询当前 Bootloader 状态，重试 3 次超时退出
主机发送：5A 05 0B 06 B0 01 01 00 00 00 22（查询当前 Bootloader 状态）
显示屏返回：5A 06 0B 06 B0 02 01 01 00 00 25（Bootloader 当前处于 Ready 状
态，表示 Bootloader 进入成功）
7. 擦除 APP 区域
主机发送指令 1：5A 05 0B 06 B0 01 02 00 00 00 23（擦除 APP 区域）
显示屏返回：NA
间隔 1s，
主机发送指令 2：5A 05 0B 06 B0 01 01 00 00 00 22（查询当前 Bootloader 状态）
显示屏返回：5A 06 0B 06 B0 02 01 02 00 00 26（Bootloader 当前处于 Blank 状
态，表示擦除成功），否则重发指令 1，重试 3 次超时退出。
8. 开始编程
主机发送指令 1：5A 05 0B 06 B0 01 03 00 00 00 24（进入编程模式）
显示屏返回：NA
间隔 100ms，
主机发送指令 2：5A 05 0B 06 B0 01 01 00 00 00 22（查询当前 Bootloader 状态）
显示屏返回：5A 06 0B 06 B0 02 01 03 00 00 27（Bootloader 当前处于
Programming 状态，表示进入编程模式成功），否则重发指令 1，重试 3 次超时退
出。
9. 逐行发送烧录文件行 [Note 1]
主机发送：将每一行的 S0/S1/S2…中的”S”转换成对应的 ASCII 码，再将转换后的
那一行按照十六进制发送到显示屏
显示屏返回：5A 06 0B 06 B0 03 01 00 00 00 25（写入成功）
5A 06 0B 06 B0 03 FF 00 00 00 23（写入失败，重试 3 次退出）
10. 校验 flash checksum [Note 2]
主机发送指令 1：5A 05 0B 06 B0 01 04 <XX XX> 00 <CS>（校验 flash
checksum，XX XX 表示全部 flash 区域的校验和）
显示屏返回：NA
间隔 100ms，
主机发送指令 2：5A 05 0B 06 B0 01 01 00 00 00 22（查询当前 Bootloader 状态）
显示屏返回：5A 06 0B 06 B0 02 01 04 00 00 28（Bootloader 当前处于
Programme Completed 状态，表示 flash 校验成功，否则重发指令 1，重试 3 次超
时退出
11. 复位(用于检测显示屏是否升级完成，正常复位，如无响应则复位成功)
主机发送指令 1：5A 05 0B 06 B0 01 05 00 00 00 26（复位命令）
显示屏返回：NA
间隔 200ms，
主机发送指令 2：5A 05 0B 06 B0 01 01 00 00 00 22（查询当前 Bootloader 状态）
显示屏返回：5A 06 0B 06 B0 02 01 05 00 00 29（Bootloader 当前处于等待复位的
状态）
12.升级成功，清除显示屏升级标志位。
************************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <signal.h>
#include "pthread.h"

#define DEV_NAME "bridge_i2c"
#define DEV_PATH "/dev/bridge_i2c"

#define BRIDGE     '0xFE'
#define BRIDGE_GET_FIRMWARE_VERSION    _IO(BRIDGE,0xE0)
#define BRIDGE_CHECK_CURRENT_STATUS    _IO(BRIDGE,0xE1)
#define BRIDGE_REQUIRE_ENTER_RIGHTS    _IO(BRIDGE,0xE2)
#define BRIDGE_SOFTWARE_RESET          _IO(BRIDGE,0xE3)
#define BRIDGE_REQUIRE_FORCE_UPDATE    _IO(BRIDGE,0xE4)
//Question Display Status
char question_display_status[9] = {0x34,0x07,0x81,0x00,0x00,0x00,0x00,0x00,0x82};
char display_status[11] = {0};
//0x48,0x49,0x4B
/*2. 查询当前 Bootloader 状态*/
char check_cur_bootloader_status[13] = {0x34,0x0B,0x5A,0x05,0x0B,0x06,0xB0,0x01,0x01,0x00,0x00,0x00,0x22};
char cur_bootloader_status[11] = {0};
/*3. 申请进入 Bootloader 权限*/
char require_enter_bootloader_rights[9] = {0x34,0x07,0x5A,0x05,0x07,0x02,0x31,0x01,0x9A};
/*4. 输入 Access Key 1 5. 输入 Access Key 2*/
char input_access_key_1[9] = {0x5A,0x05,0x07,0x04,0x31,0x03,0xA5,0x5A,0x9D};
char input_access_key_2[9] = {0x5A,0x05,0x07,0x04,0x31,0x04,0xC3,0x3C,0x9E};
/*6. 等待 2s，查询当前 Bootloader 状态，重试 3 次超时退出*/
/*7. 擦除 APP 区域*/
char erase_app_1[11] = {0x5A,0x05,0x0B,0x06,0xB0,0x01,0x02,0x00,0x00,0x00,0x23};
char erase_app_2[11] = {0x5A,0x05,0x0B,0x06,0xB0,0x01,0x01,0x00,0x00,0x00,0x22};
/*8. 开始编程*/
char start_program_1[13] = {0x34,0x0B,0x5A,0x05,0x0B,0x06,0xB0,0x01,0x03,0x00,0x00,0x00,0x24};
char start_program_2[13] = {0x34,0x0B,0x5A,0x05,0x0B,0x06,0xB0,0x01,0x01,0x00,0x00,0x00,0x22};
/*9. 逐行发送烧录文件行 [Note 1]*/
/*10. 校验 flash checksum [Note 2]*/
char check_flash_checksum[11] = {0};
/*11. 复位*/
char software_reset[13] = {0x34,0x0B,0x5A,0x05,0x0B,0x06,0xB0,0x01,0x05,0x00,0x00,0x00,0x26};

enum cmd_type {
	GET_FIRMWARE_VERSION = 1,
    CHECK_CURRENT_STATUS,
	REQUIRE_ENTER_RIGHTS,
    SOFTWARE_RESET,
    REQUIRE_FORCE_UPDATE
};

enum parameter_type {
    PT_PROGRAM_NAME = 0,
    CMD_INFO,
    PT_NUM
};

void usage(void)
{
    printf("you should input as:\n");
    printf("\t bridge_i2c [cmd]\n");
	printf("\t [1] get flash firmware version\n");
	printf("\t [2] check current bootloader status\n");
	printf("\t [3] require enter bootloader rights\n");
	printf("\t [4] software reset\n");
	printf("\t [5] require force firmware update\n");
}

void dump_buf(char *buf)
{
	int i = 0;
	
	for(i = 0; i < strlen(buf)+1; i++) {
		printf("buf[%d] = %x,",i,buf[i]);
	}
	printf("\n");
}

int upgrade_program(int fp, int cmd)
{
	int ret = 0;
	int retry = 0;
	
	switch(cmd)
	{
		case GET_FIRMWARE_VERSION:
			printf("get flash firmware version\n");
            ret = ioctl(fp,BRIDGE_GET_FIRMWARE_VERSION);
			for(retry = 0; retry < 3; retry++) {
                ret = write(fp,question_display_status,9);
			}
			ret = read(fp,display_status,11);
			dump_buf(display_status);            			
		break;

		case CHECK_CURRENT_STATUS:
			printf("check current bootloader status\n");
			ret = ioctl(fp,BRIDGE_CHECK_CURRENT_STATUS);
			memset(cur_bootloader_status,0,11);
            ret = write(fp,check_cur_bootloader_status,13);
			ret = read(fp,cur_bootloader_status,11);
			dump_buf(cur_bootloader_status);
	    break;

		case REQUIRE_ENTER_RIGHTS:
		    printf("require enter bootloader rights\n");
			ret = ioctl(fp,BRIDGE_REQUIRE_ENTER_RIGHTS);
			memset(cur_bootloader_status,0,11);
	        ret = write(fp,require_enter_bootloader_rights,9);
			ret = read(fp,cur_bootloader_status,11);
			dump_buf(cur_bootloader_status);
		break;

		case SOFTWARE_RESET:
		    printf("software reset\n");
			ret = ioctl(fp,BRIDGE_SOFTWARE_RESET);
	        ret = write(fp,software_reset,13);
			sleep(1);
			ret = read(fp,cur_bootloader_status,11);
			dump_buf(cur_bootloader_status);
		break;
		case REQUIRE_FORCE_UPDATE:
		    ret = ioctl(fp,BRIDGE_REQUIRE_FORCE_UPDATE);
		    printf("require force firmware update\n");
		break;
			
		default:
			printf("need correct cmd!\n");
		return -1;
	}
	
	return ret;
}


int main(int argc, char **argv) 
{
	int ret = 0;
	int fp = 0;
	unsigned int cmd = 0;
	int status = 0;
	
    if(argc != PT_NUM) {
        usage();
        return -1;
    }
	
	cmd = atoi(argv[CMD_INFO]);
	
	if ((ret = (fp = open(DEV_PATH, O_RDWR))) < 0) 
	{
		printf("ioctl test error retcode = %d\n", ret);
		exit(0);
	}

    ret = upgrade_program(fp,cmd);
	
	printf("ret = %d\r\n",ret);	 
    //close(fp);
	return ret;
}

