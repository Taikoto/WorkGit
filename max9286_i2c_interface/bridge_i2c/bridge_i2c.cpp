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

#define BRIDGE     'T'
#define BRIDGE_I2C_CAMERA_ID_FRONT     _IO(BRIDGE,0)
#define BRIDGE_I2C_CAMERA_ID_REAR      _IO(BRIDGE,1)
#define BRIDGE_I2C_CAMERA_ID_LEFT      _IO(BRIDGE,2)
#define BRIDGE_I2C_CAMERA_ID_RIGHT     _IO(BRIDGE,3)
#define BRIDGE_I2C_CAMERA_ID_RELEASE   _IO(BRIDGE,4)

int fp = 0;

typedef unsigned char       uint8_t;
typedef unsigned short int  uint16_t;
typedef unsigned int        uint32_t;

typedef enum {
	CAMERA_ID_FRONT = 0,
	CAMERA_ID_REAR,
	CAMERA_ID_LEFT,
	CAMERA_ID_RIGHT,
	RELEASE
} CameraId;

typedef enum {
	I2C_READ = 1,
	I2C_WRITE = 2
} I2cCmdType;

struct I2cCmd {
	I2cCmdType cmdType; //类型
	uint8_t regCmdParam[2]; //参数（填充寄存器地址,如寄存器地址是16bit，则高8bit下标为0；）
	//举例：寄存器地址0x1234，则vec[0] = 0x12,vec[1] = 0x34
	uint8_t valCmdParam[255]; //参数（写填充数据，从下标0开始发送；读填充长度，如长度大于255，则高字节下标为0）
	//举例：读长度为0x1234，则vec[0] = 0x12,vec[1] = 0x34
	uint8_t valsize;
	uint8_t size;

}; //寄存器参数和数据参数只有一个byte时只填充一个byte。

struct I2cCmdVec {
	I2cCmd Vec;
	uint8_t size;
};

typedef enum {
	NO = 0,
	OK,
} EvsResult;

struct I2cCmdResult {
	EvsResult result;
	uint8_t cmdResponse[255]; //返回值（写不填充，读填充数据，最先读取的数据放入下标为0，以此类推）
	uint8_t size;
};

//1、请求事务锁
EvsResult requestI2cLock(CameraId id, uint32_t timeoutMs);/* generates (EvsResult result)*/;
//2、释放事务锁
EvsResult releaseI2cLock(CameraId id);/* generates (EvsResult result);*/
//3、执行事务命令
I2cCmdResult executeI2cCmd(CameraId id,I2cCmd cmds,int length);/* generates (vec<I2cCmdResult> result);*/

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
	//REG,
	//VAL,
	PT_NUM
};

void usage(void)
{
	printf("you should input as:\n");
	printf("\t bridge_i2c [cmd]\n");
	printf("\t [1] read camera sensor regs value\n");
	printf("\t [2] write camera sensor regs value\n");
}

/*
void dump_buf(char *buf)
{
	int i = 0;

	for(i = 0; i < strlen(buf)+1; i++) {
		printf("buf[%d] = %x,",i,buf[i]);
	}

	printf("\n");
}
*/

//1、请求事务锁
EvsResult requestI2cLock(CameraId id, uint32_t timeoutMs)
{
	uint32_t u;
	int err = 0;
	CameraId a;

	a = id;
	u = timeoutMs;

	EvsResult ret = OK;
	switch(id) {
		case CAMERA_ID_FRONT:
			err = ioctl(fp,BRIDGE_I2C_CAMERA_ID_FRONT);
		break;
		case CAMERA_ID_REAR:
			err = ioctl(fp,BRIDGE_I2C_CAMERA_ID_REAR);
		break;
		case CAMERA_ID_LEFT:
			err = ioctl(fp,BRIDGE_I2C_CAMERA_ID_LEFT);
		break;
		case CAMERA_ID_RIGHT:
			err = ioctl(fp,BRIDGE_I2C_CAMERA_ID_RIGHT);
		break;
		default:
			printf("error CameraId\n");
		break;
	}

	printf("CameraId = %d, ret = %d\r\n",id, err);

	return ret;
}
//2、释放事务锁
EvsResult releaseI2cLock(CameraId id)
{
	int err = 0;
	CameraId a;
	a = id;

	EvsResult ret = OK;
	err = ioctl(fp,BRIDGE_I2C_CAMERA_ID_RELEASE);
	
	return ret;
}
//3、执行事务命令
I2cCmdResult executeI2cCmd(CameraId id,I2cCmd *cmds,int length)
{
	uint8_t buf[64];
	uint8_t i,j = 0;
	uint8_t len = 0;
	uint8_t ret = 0;
	CameraId a;
	//I2cCmdVec I2Cvec;
	I2cCmdResult cmdResVec;
	a = id;

	//printf("executeI2cCmd valCmdParam length = %d\n",length);
	for(i = 0; i < length; i++) {		
		if(cmds[i].cmdType == I2C_READ) {

			len = cmds[i].valCmdParam[0]; // length
			memset(buf,0,len+2);			
			/*reg_H,reg_L*/
			buf[0] = cmds[i].regCmdParam[0];
			buf[1] = cmds[i].regCmdParam[1];
			/**/			

			ret = read(fp,buf,len + 2);
			for(j = 0; j < len; j++) {
				cmdResVec.cmdResponse[j] = buf[j];
			}
		} else if(cmds[i].cmdType == I2C_WRITE) {
			len = cmds[i].valsize; // length //代替 vector 大小
			memset(buf,0,len+2);
			buf[0] = cmds[i].regCmdParam[0];
			buf[1] = cmds[i].regCmdParam[1];
			//printf("[%d]write len = %d\n",i,len);
			//len = sizeof(cmds[i].valCmdParam) / sizeof(cmds[i].valCmdParam[0]);
			//printf("valCmdParam = %d\n",len);
			for(j = 0; j < len; j++) {
				buf[j+2] = cmds[i].valCmdParam[j];
				//printf("[buf]0x%x,,",cmds[i].valCmdParam[j]);
			}			
			write(fp,buf,len+2);
		} else {
			printf("error cmds \n");
		}
	}

	return cmdResVec;
}

void Step0(CameraId cId)
{
	/* request transaction lock */
	EvsResult result = requestI2cLock(cId, 1000);
	if(OK != result) {
		//error
	}

	printf("Step0 start \n");
	/* read test*/
	I2cCmd cmdRead[1];
	cmdRead[0].cmdType = I2C_READ;
	cmdRead[0].regCmdParam[0] = 0x00; //reg addr 0x00   /*?  regCmdParam[1] ?*/

	cmdRead[0].regCmdParam[0] = 0x00;

	cmdRead[0].valCmdParam[0] = 0x01; //1Byte
	I2cCmd cmdVec[1];
	cmdVec[0] = cmdRead[0];
	I2cCmdResult cmdResVec = executeI2cCmd(cId, cmdVec,1);
	printf("0x%x,,",cmdResVec.cmdResponse[0]);
	/* release transaction lock */
	result = releaseI2cLock(cId);
	if(OK != result) {
		//error
	}

	printf("[Step0 finished] \n");
}

void Step1(CameraId cId)
{
	/* request transaction lock */
	EvsResult result = requestI2cLock(cId, 1000);
	if(OK != result) {
		//error
	}

	printf("Step1 start \n");
	/* write test*/
	I2cCmd cmdWrite[7];
	I2cCmdVec I2Cvec;
	//I2Cvec.Vec = cmdWrite;
	I2Cvec.size = 7;
	cmdWrite[0].cmdType = I2C_WRITE;
	cmdWrite[0].regCmdParam[0] = 0xa1; //reg addr 0xa10a
	cmdWrite[0].regCmdParam[1] = 0x0a;
	cmdWrite[0].valCmdParam[0] = 0x00; //1Byte, data = 0x00
	cmdWrite[0].valsize = 1;
	cmdWrite[1].cmdType = I2C_WRITE;
	cmdWrite[1].regCmdParam[0] = 0xa1; //reg addr 0xa11e
	cmdWrite[1].regCmdParam[1] = 0x1e;
	cmdWrite[1].valCmdParam[0] = 0x7f; //1Byte, data = 0x7f
	cmdWrite[1].valsize = 1;
	cmdWrite[2].cmdType = I2C_WRITE;
	cmdWrite[2].regCmdParam[0] = 0xa1; //reg addr 0xa11d
	cmdWrite[2].regCmdParam[1] = 0x1d;
	cmdWrite[2].valCmdParam[0] = 0x00; //1Byte, data = 0x00
	cmdWrite[2].valsize = 1;
	cmdWrite[3].cmdType = I2C_WRITE;
	cmdWrite[3].regCmdParam[0] = 0xa1; //reg addr 0xa110
	cmdWrite[3].regCmdParam[1] = 0x10;
	cmdWrite[3].valCmdParam[0] = 0x80; //1Byte, data = 0x80
	cmdWrite[3].valsize = 1;
	cmdWrite[4].cmdType = I2C_WRITE;
	cmdWrite[4].regCmdParam[0] = 0xa1; //reg addr 0xa10f
	cmdWrite[4].regCmdParam[1] = 0x0f;
	cmdWrite[4].valCmdParam[0] = 0x18; //1Byte, data = 0x18
	cmdWrite[4].valsize = 1;
	cmdWrite[5].cmdType = I2C_WRITE;
	cmdWrite[5].regCmdParam[0] = 0xa1; //reg addr 0xa10e
	cmdWrite[5].regCmdParam[1] = 0x0e;
	cmdWrite[5].valCmdParam[0] = 0x4c; //1Byte, data = 0x4c
	cmdWrite[5].valsize = 1;
	cmdWrite[6].cmdType = I2C_WRITE;
	cmdWrite[6].regCmdParam[0] = 0xa1; //reg addr 0xa10d
	cmdWrite[6].regCmdParam[1] = 0x0d;
	cmdWrite[6].valCmdParam[0] = 0x00; //1Byte, data = 0x00
	cmdWrite[6].valsize = 1;
	I2cCmd cmdVec[7];

	for(int i = 0; i < 7; i++) {
		cmdVec[i] = cmdWrite[i];
	}

	I2cCmdResult cmdResVec = executeI2cCmd(cId,cmdVec,7);
	for(int i = 0; i < 7; i++) {
		printf("0x%x,,",cmdResVec.cmdResponse[i]);
	}

	/* release transaction lock */
	result = releaseI2cLock(cId);
	if(OK != result) {
		//error
	}

	printf("Step1 finished \n");
}

void Step2(CameraId cId)
{
	/* request transaction lock */
	EvsResult result = requestI2cLock(cId, 1000);
	if(OK != result) {
	//error
	}

	printf("Step2 start \n");
	/* read test*/
	I2cCmd cmdRead[3];
	I2cCmdVec I2Cvec;
	//I2Cvec.Vec = cmdRead;
	I2Cvec.size = 7;
	cmdRead[0].cmdType = I2C_READ;
	cmdRead[0].regCmdParam[0] = 0xe0; //reg addr 0xe06e
	cmdRead[0].regCmdParam[1] = 0x6e;
	cmdRead[0].valCmdParam[0] = 0x01; //1Byte
	cmdRead[0].valsize = 1;
	cmdRead[1].cmdType = I2C_READ;
	cmdRead[1].regCmdParam[0] = 0xe0; //reg addr 0xe06f
	cmdRead[1].regCmdParam[1] = 0x6f;
	cmdRead[1].valCmdParam[0] = 0x01; //1Byte
	cmdRead[1].valsize = 1;
	cmdRead[2].cmdType = I2C_READ;
	cmdRead[2].regCmdParam[0] = 0xe0; //reg addr 0xe070
	cmdRead[2].regCmdParam[1] = 0x70;
	cmdRead[2].valCmdParam[0] = 0x01; //1Byte
	cmdRead[2].valsize = 1;
	I2cCmd cmdVec[3];
	for(int i = 0; i < 3; i++) {
		cmdVec[i] = cmdRead[i];
	}

	I2cCmdResult cmdResVec = executeI2cCmd(cId, cmdVec,3);
	for(int i = 0; i < 3; i++) {
		printf("0x%x,,",cmdResVec.cmdResponse[i]);
	}
	/* release transaction lock */
	result = releaseI2cLock(cId);
	if(OK != result) {
		//error
	}

	printf("Step2 finished \n");
}

void Step3(CameraId cId)
{
	/* request transaction lock */
	EvsResult result = requestI2cLock(cId, 1000);
	if(OK != result) {
		//error
	}

	printf("Step3 start \n");
	/* write test*/
	I2cCmd cmdWrite[7];
	cmdWrite[0].cmdType = I2C_WRITE;
	cmdWrite[0].regCmdParam[0] = 0xa1; //reg addr 0xa10a
	cmdWrite[0].regCmdParam[1] = 0x0a;
	cmdWrite[0].valCmdParam[0] = 0x00; //1Byte, data = 0x00
	cmdWrite[0].valsize = 1;
	cmdWrite[1].cmdType = I2C_WRITE;
	cmdWrite[1].regCmdParam[0] = 0xa1; //reg addr 0xa11e
	cmdWrite[1].regCmdParam[1] = 0x1e;
	cmdWrite[1].valCmdParam[0] = 0x7f; //1Byte, data = 0x7f
	cmdWrite[1].valsize = 1;
	cmdWrite[2].cmdType = I2C_WRITE;
	cmdWrite[2].regCmdParam[0] = 0xa1; //reg addr 0xa11d
	cmdWrite[2].regCmdParam[1] = 0x1d;
	cmdWrite[2].valCmdParam[0] = 0x00; //1Byte, data = 0x00
	cmdWrite[2].valsize = 1;
	cmdWrite[3].cmdType = I2C_WRITE;
	cmdWrite[3].regCmdParam[0] = 0xa1; //reg addr 0xa110
	cmdWrite[3].regCmdParam[1] = 0x10;
	cmdWrite[3].valCmdParam[0] = 0x80; //1Byte, data = 0x80
	cmdWrite[3].valsize = 1;
	cmdWrite[4].cmdType = I2C_WRITE;
	cmdWrite[4].regCmdParam[0] = 0xa1; //reg addr 0xa10f
	cmdWrite[4].regCmdParam[1] = 0x0f;
	cmdWrite[4].valCmdParam[0] = 0x18; //1Byte, data = 0x18
	cmdWrite[4].valsize = 1;
	cmdWrite[5].cmdType = I2C_WRITE;
	cmdWrite[5].regCmdParam[0] = 0xa1; //reg addr 0xa10e
	cmdWrite[5].regCmdParam[1] = 0x0e;
	cmdWrite[5].valCmdParam[0] = 0x4c; //1Byte, data = 0x4c
	cmdWrite[5].valsize = 1;
	cmdWrite[6].cmdType = I2C_WRITE;
	cmdWrite[6].regCmdParam[0] = 0xa1; //reg addr 0xa10d
	cmdWrite[6].regCmdParam[1] = 0x0d;
	cmdWrite[6].valCmdParam[0] = 0x00; //1Byte, data = 0x00
	cmdWrite[6].valsize = 1;
	I2cCmd cmdVec[7];
	for(int i = 0; i < 7; i++) {
		cmdVec[i] = cmdWrite[i];
	}

	I2cCmdResult cmdResVec = executeI2cCmd(cId, cmdVec,7);
	for(int i = 0; i < 7; i++) {
		printf("0x%x,,",cmdResVec.cmdResponse[i]);
	}	
	/* release transaction lock */
	result = releaseI2cLock(cId);
	if(OK != result) {
		//error
	}

	printf("Step3 finished \n");
}

void Step4(CameraId cId)
{
	//uint8_t i = 0;
	/* request transaction lock */
	EvsResult result = requestI2cLock(cId, 1000);
	if(OK != result) {
		//error
	}

	printf("Step4 start \n");
	/* write test*/
	I2cCmd cmdTest1[3];
	cmdTest1[0].cmdType = I2C_WRITE;
	cmdTest1[0].regCmdParam[0] = 0x81; //reg addr 0x8181
	cmdTest1[0].regCmdParam[1] = 0x81;
	cmdTest1[0].valCmdParam[0] = 0x00; //1Byte, data = 0x00
	cmdTest1[0].valsize = 1;
	cmdTest1[1].cmdType = I2C_WRITE;
	cmdTest1[1].regCmdParam[0] = 0xe4; //reg addr 0xE400
	cmdTest1[1].regCmdParam[1] = 0x00;
	cmdTest1[1].valCmdParam[0] = 0x81; //10Byte, data = 0x81 0x00 0x00 0x06 0x12 0x03 0x20 0x00 0x00 0x20
	cmdTest1[1].valCmdParam[1] = 0x00;
	cmdTest1[1].valCmdParam[2] = 0x00;
	cmdTest1[1].valCmdParam[3] = 0x06;
	cmdTest1[1].valCmdParam[4] = 0x12;
	cmdTest1[1].valCmdParam[5] = 0x03;
	cmdTest1[1].valCmdParam[6] = 0x20;
	cmdTest1[1].valCmdParam[7] = 0x00;
	cmdTest1[1].valCmdParam[8] = 0x00;
	cmdTest1[1].valCmdParam[9] = 0x20;
	cmdTest1[1].valsize = 10;
	cmdTest1[2].cmdType = I2C_WRITE;
	cmdTest1[2].regCmdParam[0] = 0x81; //reg addr 0x8160
	cmdTest1[2].regCmdParam[1] = 0x60;
	cmdTest1[2].valCmdParam[0] = 0x01; //1Byte, data = 0x01
	cmdTest1[2].valsize = 1;

	I2cCmd cmdVec1[3];
	for(int i = 0; i < 3; i++) {
		cmdVec1[i] = cmdTest1[i];
	}

	I2cCmdResult cmdResVec1 = executeI2cCmd(cId, cmdVec1,3);
	for(int i = 0; i < 3; i++) {
		printf("0x%x,,",cmdResVec1.cmdResponse[i]);
	}
	/* read test */
	int nTryTime = 0;
	do {
		usleep(10000);//10ms
		I2cCmd cmdTest2[1];
		cmdTest2[0].cmdType = I2C_READ;
		cmdTest2[0].regCmdParam[0] = 0x81; //reg addr 0x8180
		cmdTest2[0].regCmdParam[1] = 0x80;
		cmdTest2[0].valCmdParam[0] = 0x01; //1Byte
		cmdTest2[0].valsize = 1;

		I2cCmd cmdVec2[1];
		cmdVec2[0] = cmdTest2[0];
		I2cCmdResult cmdResVec2 = executeI2cCmd(cId, cmdVec2,1);
		printf("[0x8180]0x%x,,",cmdResVec2.cmdResponse[0]);
		if(0x99 == cmdResVec2.cmdResponse[0]) {
			printf("the 32th success \n");
			I2cCmd cmdTest3[1];
			cmdTest3[0].cmdType = I2C_READ;
			cmdTest3[0].regCmdParam[0] = 0xe7; //reg addr 0xE700
			cmdTest3[0].regCmdParam[1] = 0x00;
			cmdTest3[0].valCmdParam[0] = 0x20; //32Byte
			cmdTest3[0].valsize = 1;
			I2cCmd cmdVec3[1];
			cmdVec3[0] = cmdTest3[0];
			I2cCmdResult cmdResVec3 = executeI2cCmd(cId, cmdVec3,1);
			printf("[0xE700]:\n");
			for(int i = 0; i < 8; i++) {
				for(int j = 0; j < 4; j++) {
					printf("0x%3x,,",cmdResVec3.cmdResponse[j+i*4]);
				}
				printf("\n");
			}

			//!!!!!!data check, send data to xiangfangkang@hikauto.com
			//cmdResVec3[0].cmdResponse;
			//!!!!!!data check, send data to xiangfangkang@hikauto.com
			break;
		}

		//continue to retry
		if(nTryTime++ >= 100) {
			//error
			break;
		}
	} while(1);

	/* release transaction lock */
	result = releaseI2cLock(cId);
	if(OK != result) {
		//error
	}

	printf("Step4 finished \n");
}

void DemoMain(int cmd)
{
	CameraId cId = CAMERA_ID_FRONT;
	//cannot increment expression of enum type 'CameraId'
	switch(cmd) {
		case 0:
			cId = CAMERA_ID_FRONT;
	//for(cId = CAMERA_ID_FRONT; cId <= CAMERA_ID_RIGHT; cId++) 
		{
			Step0(cId);
			usleep(1000);//1ms
			Step1(cId);
			usleep(5000);//5ms
			Step2(cId);
			usleep(10000);//10ms
			Step3(cId);
			usleep(5000);//5ms
			Step4(cId);
			usleep(10000);//10ms
		}
		break;
		case 1:
			cId = CAMERA_ID_REAR;
		{
			Step0(cId);
			usleep(1000);//1ms
			Step1(cId);
			usleep(5000);//5ms
			Step2(cId);
			usleep(10000);//10ms
			Step3(cId);
			usleep(5000);//5ms
			Step4(cId);
			usleep(10000);//10ms
		}
		break;
		case 2:
			cId = CAMERA_ID_LEFT;
		{
			Step0(cId);
			usleep(1000);//1ms
			Step1(cId);
			usleep(5000);//5ms
			Step2(cId);
			usleep(10000);//10ms
			Step3(cId);
			usleep(5000);//5ms
			Step4(cId);
			usleep(10000);//10ms
		}
		break;
		case 3:
			cId = CAMERA_ID_RIGHT;
		{
			Step0(cId);
			usleep(1000);//1ms
			Step1(cId);
			usleep(5000);//5ms
			Step2(cId);
			usleep(10000);//10ms
			Step3(cId);
			usleep(5000);//5ms
			Step4(cId);
			usleep(10000);//10ms
		}
		break;
		default:
			printf("unknown cmd: 0x%08x\n", cmd);
			//err = -ENOIOCTLCMD;
		break;
	}
}

int main(int argc, char **argv) 
{
	int ret = 0;
	//int fp = 0;
	unsigned int cmd = 0;

	if(argc != PT_NUM) {
		usage();
		return -1;
	}

	cmd = atoi(argv[CMD_INFO]);

	if ((ret = (fp = open(DEV_PATH, O_RDWR))) < 0) {
		printf("ioctl test error retcode = %d\n", ret);
		exit(0);
	}

	DemoMain(cmd);

	printf("ret = %d\r\n",ret);	 
	//close(fp);
	return ret;
}

