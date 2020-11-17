#ifndef _MCU_DATA_FRAME_H_
#define _MCU_DATA_FRAME_H_
#include <stdint.h>
#include <stddef.h>
#include "Debug.h"

// namespace android
// {
#define MCU_DATA_FRAME_BUFFER 256

static const uint8_t g_socket_cmd[] = {0};

class McuDataFrame
{
public:
    McuDataFrame();
    virtual ~McuDataFrame();
    bool setData(uint8_t CMD, uint8_t HCMD, uint8_t LCMD, uint8_t *DATA, uint8_t DataLen);
    bool setFrameData(const uint8_t *Frame, uint8_t FrameLen);
    bool isSocketData();
    void clearData();
    
    uint16_t getCheckSum() const
    {
        return mCheckSum;
    }

    uint8_t getCmd() const
    {
        return mCMD;
    }

    uint8_t getHCmd() const
    {
        return mHCMD;
    }

    uint8_t getLCmd() const
    {
        return mLCMD;
    }

    uint8_t *getData() const
    {
        return mDATA;
    }

    uint8_t getDataLen() const
    {
        return mDataLen;
    }

    uint8_t *getFrame()
    {
        return mFrame;
    }

    uint8_t getFrameLen() const
    {
        return mFrameLen;
    }

private:
    uint8_t checkSum();

    uint8_t mCMD;  //命令主ID;
    uint8_t mHCMD; //命令子ID;
    uint8_t mLCMD; //命令子ID;

    uint8_t *mDATA;    //有效数据
    uint8_t mDataLen;  //数据长度
    uint8_t mCheckSum; //CRC校验值

    uint8_t mFrame[MCU_DATA_FRAME_BUFFER]; //数据
    uint8_t mFrameLen;                 //数据
};

// } /* namespace android */
#endif /* DATAFRAME_H_ */
