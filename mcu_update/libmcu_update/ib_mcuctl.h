/**********************************************************************
 Filename: ib_mcuctl.h
 Description: Declare the MCU Control class.
**********************************************************************/

#ifndef _IB_MCUCTL_H_
#define _IB_MCUCTL_H_
#include <pthread.h>
#include "ib_srec.h"
#include "Protocol.h"

#define IB_MCU_UPGRADE_BUF_LEN_LINE   16  //每行的MCU的数据

static int m_cur_status;//当前收到的升级状态
static bool m_cur_update = true;//是否升级

class IBMCUControl 
{
public:
    IBMCUControl();
     int Upgrade(const char *filename);
     int reset();
	 int restart();
	 int openlcd();
     int initMCU();
      int WaitUpgradeReady();
      int checkFile();
     static int notify(int cmd,int smd,int sn,int ret,uint8_t *data,int len);
     static int e01_notify(uint8_t *data,int size);
     static IBMCUControl * getInstance();
      static IBMCUControl * sInstance;
      void TxCmd(unsigned char *buf, unsigned int len);
      int exit_update_mode();
      int ReadyUpdata();//进入升级模式
      int sendPkgToMcu(bool preIndex);
      int sendLastPkgToMcu();
      IBSrec *m_srec;
      
protected:
public:
	  bool m_reset_flag;
	  static bool m_rollback_flag;
      
private:
   void test(IBSrec srec);
    int UpgradeLine(unsigned char *buf, unsigned int len,int address);
    int SendCmd(unsigned char *buf, unsigned int len,int cmd,int smd,int sn,int cmdtype);
    int EarseData();
    int sendCRC();
    uint8_t getSN(int lastsn);
    int StartUpdata(IBSrec *srec);
    int StartUpdataFromBin();
    int getDataFromBin();
    int update_ok();
    int UpgradeCheck(const char *filename);
    int upgrade_Bin_Line(unsigned char *buf, unsigned int len,int index);
    
    int sendUpgradeData(char* buffer,int length,int last);

    const char *m_filename;
	IoProtocol *IoPort;
	int m_smd;
	int m_cmd;
	int m_sn;
	unsigned int m_bin_CRC;
	int m_bin_count ;//多少个２５６数据
    int m_cur_index;//当前发送帧id
	unsigned char *m_bindata;//bin文件数据
    char **m_mcudata;//e01 mcu升级文件数据
    char *m_last_frame_mcudata;//e01 最后一帧MCU数据
    int m_last_frame_mcudata_len;//e01 最后一帧MCU数据长度
    
};

#endif
