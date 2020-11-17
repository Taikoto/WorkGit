#include <errno.h>
#include "Protocol.h"
#include "UartInterface.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include "Protocol.h"
#include "cobs.h"
#include "DataFrame.h"
#include <pthread.h>

#define REPREAT_COUNT_MAX 2 //重复的最大次数
#define FRAME_MAX_LEN  256

static uint8_t recv_buf[FRAME_MAX_LEN];
static uint8_t mMcuBufData[FRAME_MAX_LEN * 4];
static uint16_t mMcuBufLen = 0;

notify_callback   g_CallbackFun;  //mcu_service 中设置进来用于通知，上层应用的notify回调

  int  get_one_frame(uint8_t *buf, int *len, int *frm_size)
{
	int i;
	int ret = -1;
	int frame_found = 0;
	uint8_t *p = buf;

	/* check if frame seperater exist */
	for (i = 0; i < *len; i++)
	{
		if (*p == 0)
		{
			p++;
			frame_found = 1;
			break;
		}

		p++;
	}

	/* found a frame */
	if (frame_found)
	{
		ret = 0;
		*frm_size = i + 1;
		*len -= (i + 1);
	}
	return ret;
}



  /* analysis the frame received from MCU */
  static void received_one_frame(uint8_t *buf, int size)
  {
	
	  g_CallbackFun(buf,size);

  	// uint8_t *cobs_dst_buf;
  	// uint8_t cmd, smd, sn, priority,ret;
  	// uint16_t check_sum;
  	// int len;

	// Cobs cobs;
	// cobs.setCobsData(buf,size);
	// cobs.unStuffData();

	// DataFrame frame;
	// frame.setFrameData(cobs.getData(),cobs.getDataLength());

	// if(frame.getFrameLen() ==0){ //ERROR frame
	// 	printf("errro  frame---------lbx\n");
	// 	return;
	// }

	// cmd = frame.getCmd();
	// smd = frame.getSmd();
	// sn =  frame.getSn();
	// int ret2 = frame.getPriorityRet();

	//printf("cmd=%02x smd=%02x sn =%02x,ret=%02x\n  ",cmd,smd,sn,ret2);

	//g_CallbackFun(frame.getCmd(),frame.getSmd(),sn,ret2,frame.getData(),frame.getDataLen());

}

/**
 * 判断数据是否有效
 *uint8_t *buf, int size
 * @param data
 * @return
 */
static bool isValidCheckSum(uint8_t *data, int length)
{
	if (data == NULL || length < PACKAGE_MIN_LENGTH)
	{
		return false;
	}

	uint8_t checkSum = 0;
	for (int i = 2; i < length - 1; i++)
	{
		checkSum += data[i];
	}
	checkSum = (uint8_t)(~checkSum + 1);
	if (checkSum == data[length - 1])
	{
		return true;
	}

	return false;
}

  /**
 *	数据校验
 */
static void checkMcuData()
{
	if (mMcuBufLen > sizeof(mMcuBufData))
	{
		printf("checkMcuData mMcuBufLen:%d.", mMcuBufLen);
	}

	if (mMcuBufLen <= PACKAGE_MIN_LENGTH)
	{
		printf("checkMcuData mMcuBufLen:%d.", mMcuBufLen);
	}

	int bufLen = mMcuBufLen;
	int packIndex = 0;

	uint8_t data_buf[FRAME_MAX_LEN];

	while ((mMcuBufLen - packIndex) >= PACKAGE_MIN_LENGTH)
	{
		// 包头校验
		if ((mMcuBufData[packIndex] == PACKAGE_HEAD1) && (mMcuBufData[packIndex + 1] == PACKAGE_HEAD2))
		{
			int dataLen = mMcuBufData[packIndex + 2];
			int packageLen = dataLen + 4;

			if (mMcuBufLen < packageLen)
			{
				printf("checkMcuData, buflen error! break, packIndex: %d.", packIndex);
				break;
			}
			else
			{
				// 获取单个数据包
				memset(data_buf, 0x00, sizeof(data_buf));
				memcpy(data_buf, mMcuBufData + packIndex, packageLen);

				if (isValidCheckSum(data_buf, packageLen))
				{
					printMcu2App(data_buf, packageLen);
					received_one_frame(data_buf, packageLen);
				}
				else
				{
					MLOGE("checkMcuData checkSum NG!!!!, packIndex: %d, packageLen:%d", packIndex, packageLen);
					printMcu2App(data_buf, packageLen);
				}

				packIndex += packageLen;
				mMcuBufLen -= packageLen;
			}
		}
		else
		{
			MLOGW("checkMcuData, head error, packIndex: %d, mMcuDataBuf[packIndex]:0x%02x", packIndex, mMcuBufData[packIndex]);

			packIndex++;
			mMcuBufLen--;

			if (mMcuBufLen <= 0)
			{
				mMcuBufLen = 0;
				memset(mMcuBufData, 0x00, sizeof(mMcuBufData));
				return;
			}
		}
	}

	if (packIndex > 0)
	{
		memcpy(mMcuBufData, mMcuBufData + packIndex, bufLen - packIndex);
		memset(mMcuBufData + bufLen - packIndex, 0x00, bufLen);
	}
}

void *reader_function(void *){
	int len;
	int total_len = 0;
	int frame_size;
	int readLen = -1;
    while (1)
	{

		/* receive raw data from UART */
		len = sizeof (recv_buf) - total_len;
		//printf("wugs---------reader---function------buflen=%d-- tao=%d len=%d\n", sizeof (recv_buf),total_len,len);
		readLen = IoProtocol::getInstance()->mcuport->rxdata(recv_buf, sizeof(recv_buf));
		if(readLen > 0)
		{
			if (mMcuBufLen + readLen > (FRAME_MAX_LEN * 4))
			{
				printf("data buff is outside mMcuBufLen:%d readLen:%d.", mMcuBufLen, readLen);
				mMcuBufLen = 0;
			}

			// 数据读取缓冲区
			memcpy(mMcuBufData + mMcuBufLen, recv_buf, readLen);
			mMcuBufLen += readLen;
			checkMcuData();
		}
		else
		{
			usleep(10 * 1000);	
		}
#if 0
	    if(readLen < 1)
        {
			total_len = 0;
            continue;
        }
		total_len += readLen;
		while (1)
		{
			if (0 == get_one_frame(recv_buf, &total_len, &frame_size))
			{
				received_one_frame(recv_buf, frame_size);
				if (total_len > 0)
					memmove(recv_buf, recv_buf+frame_size, total_len);

			}else
			{
				//printf("-----total_len =%d  frame_size =%d",total_len,frame_size);
				  // ALOGD("%s %d total_len =%d ",__FUNCTION__,__LINE__,total_len);
				//dumpData(recv_buf,total_len);
				break;
			}
		}
#endif
    }
    return NULL;
}


IoProtocol * IoProtocol::sInstance = NULL;
IoProtocol * IoProtocol::getInstance(){
		if(sInstance == NULL){
			sInstance =  new IoProtocol();
		}
	return sInstance;
}

IoProtocol::IoProtocol()
{
	pthread_t reader;
	mcuport = new UartInterface(EUARTIO);
	pthread_create(&reader, NULL, reader_function, NULL);
}

void IoProtocol::setCallBack(notify_callback callBackFun)
{
	g_CallbackFun = callBackFun;
}

//传输的数据和长度
int IoProtocol::txdata(unsigned char *txbuf, unsigned int len)
{
	return mcuport->txdata(txbuf,len);
}


int IoProtocol::getvalidFrame(unsigned char *data,int datalen,mcu_frame *mframe)
{
	unsigned char *datapos = data;
	unsigned char checksum = 0;

	if(datalen < 4){
		 fprintf(stderr,"--- getvalid frame len < 4-----\n");
		return -1;
	}
	mframe->mCT = (datapos[0]>>7)&0x1;
	mframe->mCMD = (datapos[0]&0x7F);
	mframe->mSMD = datapos[1];
	mframe->mPRIORITY_RET = datapos[2]&0x0F;
	mframe->mSN =  (datapos[2]>>4)&0x0F;
	mframe->mtype = datapos[3];
	int reslut =  datapos[3]&0x0F;

   if(datapos[0] == 0xA2  && reslut == 0x00){
	   return 0;
   }else if(datapos[0] == 0x12){
	   return 0;
   }
	return -1;
}


/**
 * 从指定的数据中找到一帧cobs编码数据
 *@param uint8_t *buf 输入的数据指针
 *@param	int *len  输入的数据长度
 *@param int *frm_size 返回找到的cobs编码数据包长度
 *@return int 0表示找到数据包
 *						-1 表示没有找到数据包
 */
  int  IoProtocol::get_one_frame(uint8_t *buf, int *len, int *frm_size)
{
	int i;
	int ret = -1;
	int frame_found = 0;
	uint8_t *p = buf;

	/* check if frame seperater exist */
	for (i = 0; i < *len; i++)
	{
		if (*p == 0)
		{
			p++;
			frame_found = 1;
			break;
		}

		p++;
	}

	/* found a frame */
	if (frame_found)
	{
		ret = 0;
		*frm_size = i + 1;
		*len -= (i + 1);
	}

	return ret;
}


/* analysis the frame received from MCU */
  DataFrame IoProtocol::received_one_frame(uint8_t *buf, int size)
{
	uint8_t *cobs_dst_buf;
	uint8_t cmd, smd, sn, priority,ret;
	uint16_t check_sum;
	int len;
	Cobs cobs;
	cobs.setCobsData(buf,size);
	cobs.unStuffData();
	cobs_dst_buf = cobs.getData();

	//fprintf(stderr,"recdata uncobs: ");
	///for(int i =0; i <cobs.getDataLength(); i++){
		//fprintf(stderr,"%02x ",cobs_dst_buf[i]);
	//}
	//fprintf(stderr,"\n");

	DataFrame frame;
	frame.setFrameData(cobs.getData(),cobs.getDataLength());
    return frame;

}

  DataFrame* IoProtocol::rxdata( )
{
	static uint8_t recv_buf[FRAME_MAX_LEN];
	int len;
	int total_len = 0;
	int frame_size;
	int readLen = -1;
	int	ret = 0,repeatCount =0;
	DataFrame frame;
	memset(&frame,0,sizeof(frame));

    do
	{

		/* receive raw data from UART */
		len = sizeof (recv_buf) - total_len;
		readLen =  mcuport->rxdata(recv_buf + total_len, len);
		fprintf(stderr,"rec orgdata--------readlen: %d  repeatCount=%d\n",readLen,repeatCount);
	    if(readLen == -1)
        {
			total_len = 0;
            continue;
        }
		total_len += readLen;

		//fprintf(stderr,"rec orgdata: ");
		//for(int i =0; i <readLen; i++){
		//	fprintf(stderr,"%02x ",recv_buf[i]);
		//}
		//fprintf(stderr,"\n");

		if (0 == get_one_frame(recv_buf, &total_len, &frame_size))
		{
			frame = received_one_frame(recv_buf, frame_size);
			return &frame;
		}
    }while(repeatCount++ < REPREAT_COUNT_MAX);
    return NULL;
}
