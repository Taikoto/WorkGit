#ifndef IOPROTOCOL_H__
#define IOPROTOCOL_H__

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <asm/types.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include "UartInterface.h"
#include "DataFrame.h"
#include "Debug.h"

#define PACKAGE_MIN_LENGTH  0x05		// 最小包长
#define PACKAGE_HEAD1 		0xFF		// 第一帧
#define PACKAGE_HEAD2  		0x66		// 第二帧

struct mcu_frame{
	 unsigned char mCT;//命令类型
	unsigned char mCMD;//命令主ID;
	unsigned char mSMD;//命令子ID;
	unsigned char mSN;//流水号;0-15
	unsigned char mPRIORITY_RET;//优先级
	unsigned char *mDATA;//有效数据
	unsigned char mDataLen;//数据长度
	unsigned char mtype;//优先级

};

#define MIN_FRAME_LEN	13
//typedef int (*notify_callback)(int cmd,int smd,int sn,int ret,uint8_t *data,int len);
//typedef int (*notify_callback)(int head1,int head2,int length,int type,int type1,int type2,uint8_t *data,int check);
typedef int (*notify_callback)(uint8_t *data,int size);
class IoProtocol{
 public:
  IoProtocol();
  ~IoProtocol();
  void setCallBack(notify_callback callBackFun);
	//pack protocol data and send on uart
  int txdata(unsigned char *txbuf, unsigned int len);
	//get data on uart and parse protocol data
  DataFrame *rxdata( );
  static IoProtocol * getInstance();
  static IoProtocol * sInstance;
  UartInterface *mcuport;
 private:
 	int getvalidFrame(unsigned char *data,int datalen,mcu_frame *mframe);
  	unsigned char mcudata[128+12];
	unsigned char Rxdata[128];
	int RxLen;

	DataFrame received_one_frame(uint8_t *buf, int size);
	int get_one_frame(uint8_t *buf, int *len, int *frm_size);
};

#endif
