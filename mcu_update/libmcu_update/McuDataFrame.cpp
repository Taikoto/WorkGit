#include "McuDataFrame.h"
#include <string.h>
#include <cutils/log.h>

// namespace android
// {

#define FIRST_POS       0   //同步字节1
#define SECOND_POS      1   //同步字节2
#define DATA_LEN_POS    2   //命令长度
#define CMD_LEN_POS     3   //命令种类
#define HCMD_LEN_POS    4   //命令高字节
#define LCMD_LEN_POS    5   //命令低字节
#define DATA_START_POS  6   //数据开始位置
#define FRAME_HEAD_LEN  3	//数据包头长度
#define CHECKSUM_SIZE   1   //校验数据的大小
#define ALL_CMD_LEN     3   //3个命令长度

#define PACKAGE_MIN_LENGTH  0x05		// 最小包长
#define PACKAGE_HEAD1 		0xFF		// 第一帧
#define PACKAGE_HEAD2  		0x66		// 第二帧

/**
 * McuDataFrame
 */
McuDataFrame::McuDataFrame() : 
    mCMD(0), mHCMD(0), mLCMD(0), mDATA(0), mDataLen(0), mCheckSum(0), mFrameLen(0)
{
	memset(mFrame, 0, MCU_DATA_FRAME_BUFFER);
}

/**
 * 清除数据包中的所有数据
 */
void McuDataFrame::clearData()
{
	mCMD = 0;
	mHCMD = 0;
	mLCMD = 0;
	mDATA = 0;
	mDataLen = 0;
	mCheckSum = 0;
	mFrameLen = 0;
	memset(mFrame, 0, MCU_DATA_FRAME_BUFFER);
}

McuDataFrame::~McuDataFrame()
{
}

/**
 * 创建一个数据帧或者应答帧格式详见协议
 */
// void McuDataFrame::setData(uint8_t CT, uint8_t CMD, uint8_t SMD, uint8_t SN, uint8_t PRIORITY_RET, uint8_t *DATA, uint8_t DataLen)
bool McuDataFrame::setData(uint8_t CMD, uint8_t HCMD, uint8_t LCMD, uint8_t *DATA, uint8_t DataLen)
{
    MLOGD("[%s %d] %02x %02x %02x %d.", __FUNCTION__, __LINE__, CMD, HCMD, LCMD, DataLen);

	clearData();

    // 数据帧长度 == 固定帧头长度 6 + 数据长度 + 校验长度 
	int len = DATA_START_POS + DataLen + CHECKSUM_SIZE;

    // 数据帧大小校验
	if (MCU_DATA_FRAME_BUFFER < (len))
	{
		return false;
	}

	mFrameLen = len;
    mDataLen = DataLen;
	mCMD = CMD;
	mHCMD = HCMD;
	mLCMD = LCMD;
	
    mFrame[FIRST_POS] = 0xff;
    mFrame[SECOND_POS] = 0x66;
	mFrame[DATA_LEN_POS] = mDataLen + FRAME_HEAD_LEN;   // 3 个命令 + 数据长度
    mFrame[CMD_LEN_POS] = mCMD;
    mFrame[HCMD_LEN_POS] = mHCMD;
    mFrame[LCMD_LEN_POS] = mLCMD;

	mDATA = mFrame + DATA_START_POS;
	
	if (NULL != DATA)
	{
		memcpy(mDATA, DATA, mDataLen);
	}
	else
	{
		mDataLen = 0;
	}
    
	mCheckSum = checkSum();
	memcpy(mFrame + DATA_START_POS + mDataLen, &mCheckSum, CHECKSUM_SIZE);   //写入校验值

    printApp2Mcu(mFrame, mFrameLen);

	return true;
}
/**
 * 设置一个原始的数据包，存放从串口接收的数据
 *@param  uint8_t *Frame 数据帧
 *@param  uint8_t FrameLen 数据帧长度
 */
bool McuDataFrame::setFrameData(const uint8_t *Frame, uint8_t FrameLen)
{
	clearData();

	int len = FrameLen;
	if (MCU_DATA_FRAME_BUFFER < len)
	{
		MLOGD("setFrameData  fail  DATA_FRAME_BUFFER >= len %d", len);
		return false;
	}

    if (len < PACKAGE_MIN_LENGTH)
	{
		MLOGD("setFrameData FrameLen:%d.", len);
        return false;
	}

    if ((Frame[FIRST_POS] != PACKAGE_HEAD1) && (Frame[SECOND_POS] != PACKAGE_HEAD2))
    {
        MLOGD("setFrameData PACKAGE_HEAD1:%d, PACKAGE_HEAD2:%d.", Frame[FIRST_POS], Frame[SECOND_POS]);
        return false;
    }

	mFrameLen = FrameLen;
	memcpy(mFrame, Frame, FrameLen);
    
	mCMD = mFrame[CMD_LEN_POS];
	mHCMD = mFrame[HCMD_LEN_POS];
    mLCMD = mFrame[LCMD_LEN_POS];

	mDATA = mFrame + DATA_START_POS;
	mDataLen = mFrame[DATA_LEN_POS] - ALL_CMD_LEN;

	int index = FrameLen - CHECKSUM_SIZE;
	mCheckSum = mFrame[index];
	uint16_t checksum = checkSum();

	if (mCheckSum != checksum)
	{
		clearData();
		MLOGD("setFrameData  fail  mCheckSum != checksum ");
		return false;
	}

	return true;
}

/**
 * 计算从 CT到DATA的校验值
 * @return 返回CRC校验值
 */
uint8_t McuDataFrame::checkSum()
{
    if (mFrameLen < DATA_LEN_POS) return 0;

	uint8_t checkSum = 0;
	for (int i = DATA_LEN_POS; i < mFrameLen - 1; i++)
	{
		checkSum += mFrame[i];
	}
	checkSum = (uint8_t)(~checkSum + 1);
	
    return checkSum;
}

/**
 * 是否需要分发到socket
 */
bool McuDataFrame::isSocketData()
{
	// if (mCMD == 0) return false;

	// int count = sizeof(g_socket_cmd)/sizeof(g_socket_cmd[0]);

	// for (int i = 0; i < count; i++)
	// {
	// 	if (mCMD == g_socket_cmd[i])
	// 	{
	// 		return true;
	// 	}	
	// }
	
	return true;
}

// } /* namespace android */
