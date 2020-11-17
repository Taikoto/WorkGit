/**********************************************************************
 Filename: ib_mcuctl.cpp
 Description: Defines the MCU Control class.
**********************************************************************/

#include "ib_mcuctl.h"
#include "ib_srec.h"
#include "cobs.h"
#include "DataFrame.h"
#include "McuDataFrame.h"

#define DEBUG
#ifdef DEBUG
#define DDD(fmt,arg...) do{printf("[%s,%d]" fmt "\n",__func__,__LINE__ ,##arg);}while(0)
#else
#define DDD(fmt,arg)
#endif

#define MCU_UPDATE_FIRST_BYTE 0x14

#define MCU_UPDATE_READDY 0x71  //MCU进入升级（0x71）指令通知MCU进入升级模式
#define MCU_CMD_EARSE_MCU 0x72 //MCU数据擦除
#define MCU_CMD_UPDATE_MCU   0x73 //开始升级MCU数据
#define MCU_UPDATE_OK     0x74 //升级结束
#define MCU_UPDATE_OVER   0x75 //退出升级模式
#define MCU_RESET_CMD   0x76 //mcu复位指令

#define MCU_RESPONSE     0x01
#define TIMEOUT_TIME     16

#define IB_MCU_PROGRAME_RETRY_TIME  0x5
#define IB_MCU_PROGRAME_RETRY_INTERVAL 200000
#define IB_MCU_UPGRADE_BUF_LEN 128
#define MCU_UPDATE_LINE_LEN    256
#define PRINT_B(buf,len) do{ int i; for(i = 0;i<len;i++) printf("buf[%02d] = 0x%02x\n",i,buf[i]);}while(0)
static bool g_isInit =false;
static int g_step = 0;
static bool g_is_ack_ok = false;
static uint8_t g_update_sn =0x20;
static bool g_is_write_ok = true;

pthread_mutex_t clifd_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t clifd_cond = PTHREAD_COND_INITIALIZER;


 static void send_init_ack(uint8_t cmd, uint8_t smd, uint8_t sn)
  {
 		DataFrame frame;
 		frame.setAck(cmd,smd,sn);
 		Cobs cobs;
 		cobs.setData(frame.getFrame(), frame.getFrameLen());
 		cobs.stuffData();
 		IBMCUControl::getInstance()->TxCmd(cobs.getCobsData(),cobs.getCobsDataLength());

   }

int IBMCUControl::e01_notify(uint8_t *data,int size)
{
	 uint8_t cmd = data[3];
	 if (cmd == 0x0E && data[5] == 0x03)
	 {	 
		 pthread_mutex_lock(&clifd_mutex);
		 m_cur_status = data[6];
		 pthread_cond_signal(&clifd_cond);
		 pthread_mutex_unlock(&clifd_mutex);
	 }
	 return 0;
}

 int IBMCUControl::notify(int cmd,int smd,int sn,int ret,uint8_t *data,int len)
{
	 fprintf(stderr,"notify---data txcmd: cmd =%02x smd =%02x sn =%02x  ret=%02x\n",cmd,smd,sn,ret);
	 fprintf(stderr,"data:");
	 for(int i =0; i <len; i++){
	 	  fprintf(stderr,"%02x ",data[i]);
	   }
	   fprintf(stderr,"\n");

	   //初始化数据
	   if(cmd == 0x00 && smd == 0x01){
		   uint8_t cmd = 0x80;
		  uint8_t smd = 0x01;
		   send_init_ack(cmd,smd,sn);
		   g_isInit = true;
	   }

       int SN = sn;
       int tmpSN = (g_update_sn>>4) &0x0F;
	   //升级数据 更加sn号判断到第几步了
	   if(cmd == 0x12 && smd == 0x00 && ret == 0x00)
	   {
		   if(SN == 0){
			   g_step = 1;
		   }else if(SN == 1){
		   }else if(SN == tmpSN){
		   }else if(SN == 3){
		   }else if(SN == 4){
			   g_step = 4;
		   }
	   }
	  // fprintf(stderr,"--------rec-----g_step- =%d\n ",g_step);
	   if(cmd == 0x12 && smd == 0x01)
	   {
		   uint8_t  tmp ;
		   if(len > 0){
			   tmp = data[0];
		   }
		   if(tmp == 0x81){
			   g_step = 2;
		   }else if(tmp == 0x84){
			   uint8_t  check = data[1];
			   if(check == 1){
				   g_is_write_ok = false;
				   fprintf(stderr,"------0x84--rec--error---g_update_sn =%d  tmpSN=%d\n ",g_update_sn,tmpSN);
			   }
				pthread_mutex_lock(&clifd_mutex);
				g_is_ack_ok = true;
				pthread_cond_signal(&clifd_cond);
				pthread_mutex_unlock(&clifd_mutex);
		   }else if(tmp == 0x85){
			   	   uint8_t  crc = data[1];
			   	   fprintf(stderr,"------0x85--rec---- crc =%d\n ",crc);
			   	   if(crc == 0){
			   		   g_step = 3;
			   	   }
		   }else if(tmp == 0x87){
		   		m_rollback_flag = true;
		   }
	   }
	 return 0;
}



IBMCUControl::IBMCUControl()
{
	  IoPort =  IoProtocol::getInstance();
	  IoPort->setCallBack(e01_notify);
	  m_bin_CRC = 0;
	  m_bindata = (unsigned char *)(new char[MCU_UPGRADE_BUF_LENGTH]);
	  m_mcudata = new char* [20000];
	  for(int i = 0; i < 20000; i++){
		  m_mcudata[i] = new char[100];
	  }
	  m_reset_flag=false;
	  m_rollback_flag=false;
	  m_cur_index = 0;
	  m_cur_status = 0;
	  m_last_frame_mcudata_len = 0;
	  return;
}


IBMCUControl * IBMCUControl::sInstance = NULL;
bool IBMCUControl::m_rollback_flag = false;
IBMCUControl * IBMCUControl::getInstance(){
		if(sInstance == NULL){
			sInstance =  new IBMCUControl();
		}
	return sInstance;
}


int IBMCUControl::UpgradeCheck(const char *filename)
{
	return 0;
}

uint8_t IBMCUControl::getSN(int lastsn)
{
	uint8_t buf[6]={0x20,0x50,0x60,0x70,0x80,0x90};
	int i =0;
     for(i = 0; i < sizeof(buf); i++){
    	 if( lastsn == buf[i])
    		 break;
     }
     i++;
     if(i>=sizeof(buf)){
    	 return buf[0];
     }else{
    	return  buf[i];
     }
}

int IBMCUControl::UpgradeLine(unsigned char *buf, unsigned int len,int address)
{
	fprintf(stderr,"address=%02x,len=%d ,data=",address,len);
	for(int j = 0; j < len;j++){
		fprintf(stderr,"%02x",buf[j]);
	}
	fprintf(stderr,"\n");

	int ret;
	int retry = 0;
	int dataLen = len;
	 unsigned char checksum = 0;
	 //升级数据长度 头（4byte）+长度(1byte)+地址高(1byte)+ 地址中(1byte)+地址低(1byte)+数据(16byte)+CheckSum(1byte)
	 unsigned char upgradbuf[128];
	 memset(upgradbuf,0,sizeof(upgradbuf));
	 g_update_sn = getSN(g_update_sn);
	 upgradbuf[0] = 0x12;
	 upgradbuf[1] = 0x00;
	 upgradbuf[2] = g_update_sn;
	 upgradbuf[3] = MCU_CMD_UPDATE_MCU;
	 for(int i=0; i < len; i++){
    	upgradbuf[4+i] = buf[i]; // S2后面的所有 mcu data
    }
	 fprintf(stderr,"\n g_update_sn===%02x\n:",g_update_sn);
	 int total = len + 4;
	DataFrame dataframe;
	uint16_t checksum2= dataframe.checksum(upgradbuf,total);
	uint8_t hightCS =(checksum2>>8) ;
	uint8_t lowCS =(checksum2 <<8) >>8;
	upgradbuf[total]=hightCS;
	upgradbuf[total+1]=lowCS;

	fprintf(stderr,"\n upgraddata:");
	for(int j = 0; j < total+2;j++){
			fprintf(stderr,"%02x",upgradbuf[j]);
		}
	fprintf(stderr,"\n");

	Cobs cobs;
	cobs.setData(upgradbuf, total+2);
	cobs.stuffData();

    TxCmd(cobs.getCobsData(),cobs.getCobsDataLength());

	return 0;
}

int IBMCUControl::SendCmd(unsigned char *buf, unsigned int len,int cmd,int smd,int sn,int cmdtype)
{
	uint8_t databuf[128];
	memset(databuf,0,sizeof(databuf));
	m_smd = smd;
	m_cmd=cmd;
	m_sn = sn;
	if(buf == NULL && len == 0){

		fprintf(stderr,"sendcmd m_smd=%02x cmd=%02x sn=%02x cmdtype=%02x\n",m_smd,m_cmd,m_sn,cmdtype);

		databuf[0]=cmd;
		databuf[1]=smd;
		databuf[2]=sn;
		databuf[3]=cmdtype;
		DataFrame dataframe;
		uint16_t checksum = dataframe.checksum(databuf,4);
		uint8_t hightCS =(checksum>>8) ;
		uint8_t lowCS =(checksum <<8) >>8;
		databuf[4]=hightCS;
		databuf[5]=lowCS;
		Cobs cobs;
		cobs.setData(databuf, 6);
		cobs.stuffData();
		TxCmd(cobs.getCobsData(),cobs.getCobsDataLength());
		return 0;
	}else{
		databuf[0]=cmd;
		databuf[1]=smd;
		databuf[2]=sn;
		databuf[3]=cmdtype;
		memcpy(&databuf[4],buf,len);
		int totalLen =4 + len;
		DataFrame dataframe;
		uint16_t checksum = dataframe.checksum(databuf,totalLen);
		uint8_t hightCS =(checksum>>8) ;
		uint8_t lowCS =(checksum <<8) >>8;
		databuf[totalLen]=hightCS;
		databuf[totalLen+1]=lowCS;
		Cobs cobs;
		cobs.setData(databuf, totalLen+2);
		cobs.stuffData();
		TxCmd(cobs.getCobsData(),cobs.getCobsDataLength());
	   return 0;
	}
	return 0;
}

void IBMCUControl::TxCmd(unsigned char *buf, unsigned int len)
{
	int ret;
	int retry = 0;

	for(retry = 0; retry < IB_MCU_PROGRAME_RETRY_TIME; retry++)
	{
		ret = IoPort->txdata(buf,len);
		// fprintf(stderr,"txcmd---ret=%d, retry=%d\n",ret,retry);
		if(ret < 0)
		{
			usleep(IB_MCU_PROGRAME_RETRY_INTERVAL);
			continue;
		}else
			break;
	}
}

int IBMCUControl::reset()
{
	McuDataFrame frame;
	uint8_t data = 2;
    frame.setData(0x06,0x00,0x8E,&data,1);
	TxCmd(frame.getFrame(),frame.getFrameLen());
	fprintf(stderr, " ----mcu reset-----!\r\n");
	return 0;
}

int IBMCUControl::restart()
{
	if(!g_isInit){
		initMCU();
	}
	fprintf(stderr, "\r\nTell MCU restart...\r\n");

	uint8_t databuf[10];
	databuf[0]=0x00;
	databuf[1]=0x02;
	databuf[2]= 0x02;
	databuf[3]=0x01;
	databuf[4]=0x00;
	int totalLen=5;
	DataFrame dataframe;
	uint16_t checksum = dataframe.checksum(databuf,totalLen);
	uint8_t hightCS =(checksum>>8) ;
	uint8_t lowCS =(checksum <<8) >>8;
	databuf[totalLen]=hightCS;
	databuf[totalLen+1]=lowCS;
	Cobs cobs;
	cobs.setData(databuf, totalLen+2);
	cobs.stuffData();
	TxCmd(cobs.getCobsData(),cobs.getCobsDataLength());
	return 0;
}

int IBMCUControl::openlcd()
{
	if(!g_isInit){
		initMCU();
	}
	fprintf(stderr, "\r\nTell MCU open LCD in MASTER CLEAR mode...\r\n");

	uint8_t databuf[10];
	databuf[0]=0x08;
	databuf[1]=0x09;
	databuf[2]= 0x02;
	databuf[3]=0x02;
	//databuf[4]=0x00;
	int totalLen=4;
	DataFrame dataframe;
	uint16_t checksum = dataframe.checksum(databuf,totalLen);
	uint8_t hightCS =(checksum>>8) ;
	uint8_t lowCS =(checksum <<8) >>8;
	databuf[totalLen]=hightCS;
	databuf[totalLen+1]=lowCS;
	Cobs cobs;
	cobs.setData(databuf, totalLen+2);
	cobs.stuffData();
	TxCmd(cobs.getCobsData(),cobs.getCobsDataLength());
	return 0;
}


int IBMCUControl::exit_update_mode()
{
	if(!g_isInit){
		initMCU();
	}
	int cmd = 0x12,smd=0x0,sn=0x40;
	int count=0;
	while(1)
	{
		count++;
		if(count >= IB_MCU_PROGRAME_RETRY_TIME){
			fprintf(stderr, "MCU_UPDATE_OVER----failed---.\r\n");
			return -1;
		}
		if(g_step < 4)
		{
			SendCmd(NULL,0,cmd,smd,sn,MCU_UPDATE_OVER);
			usleep(300*1000);
		}else{
			fprintf(stderr, "exit_update_mode----ok----.\r\n");
			break;
		}
	}
	return 0;
}

void IBMCUControl::test(IBSrec srec)
{

	unsigned char buf[IB_MCU_UPGRADE_BUF_LEN];
		memset(buf,0,IB_MCU_UPGRADE_BUF_LEN);
		int line = srec.GetUpradeTotalLine();
		fprintf(stderr,"line====%d\n",line);

		for(int i = 0; i < line; i++)
		{
			int address = srec.GetUpGradeAddr(i);
			int len = srec.GetUpradeLen(i);
			srec.GetData(buf,len,i);
			//UpgradeLine(buf,len,address);
			fprintf(stderr,"address=%02x,len=%d ,data=",address,len);
             for(int j = 0; j < len;j++){
            	 fprintf(stderr,"%02x",buf[j]);
             }
             fprintf(stderr,"\n");

		}//for
}

int IBMCUControl::initMCU()
{
	uint8_t initBuf[20]={0x00,0x00,0x02,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x00,0x00,0x00};

	DataFrame dataframe;
	uint16_t checksum = dataframe.checksum(initBuf,18);
	uint8_t hightCS =(checksum>>8) ;
	uint8_t lowCS =(checksum <<8) >>8;
	initBuf[18]=hightCS;
	initBuf[19]=lowCS;
	Cobs cobs;
	cobs.setData(initBuf, 20);
	cobs.stuffData();
	int count = 0;
	while(1){
		count++;
		if(count >= IB_MCU_PROGRAME_RETRY_TIME){
			fprintf(stderr, "MCU_UPDATE_int---failed---.\r\n");
			return -1;
		}
       if(!g_isInit)
       {
    	   TxCmd(cobs.getCobsData(),cobs.getCobsDataLength());
    	   usleep(1000*1000);
       }else{
    	   printf("******** init mcu is ok-------\n");
    	  // uint8_t cmd = 0x80;
    	  // uint8_t smd = 0x01;
    	  // send_init_ack(cmd,smd,0);
    	  break;
       }
	}
	return 0;
}

int IBMCUControl::ReadyUpdata()
{
	McuDataFrame frame;
	uint8_t data = 1;
    frame.setData(0x0e,0x00,0x01,&data,1);
	TxCmd(frame.getFrame(),frame.getFrameLen());
	return 0;
}

int IBMCUControl::update_ok()
{
	int cmd = 0x12,smd=0x00,sn=0x32;
		int count=0;
		while(1)
		{
			count++;
			if(count >= IB_MCU_PROGRAME_RETRY_TIME){
				fprintf(stderr, "MCU_UPDATE_OK----failed---.\r\n");
				return -1;
			}
			if(g_step < 3)
			{
				//SendCmd(NULL,0,cmd,smd,sn,MCU_UPDATE_OK);

				usleep(300*1000);
			}else{
				fprintf(stderr, "MCU_UPDATE_OK----ok----.\r\n");
				break;
			}
		}
		return 0;
}

int IBMCUControl::EarseData()
{
	fprintf(stderr, "MCU_CMD_EARSE_MCU----g_step=%d---.\r\n",g_step);
	unsigned char buf[IB_MCU_UPGRADE_BUF_LEN];
	memset(buf,0,IB_MCU_UPGRADE_BUF_LEN);
	int cmd = 0x12,smd=0x0,sn=0x12;
	buf[0] = (m_bin_count >> 8) &0xff;//block高位
	buf[1] = m_bin_count&0xff;//block地址中位

	fprintf(stderr, "MCU_CMD_EARSE_MCU----block= %02x%02x---.\r\n",buf[0],buf[1]);
	fprintf(stderr, "MCU_CMD_EARSE_MCU----g_step=%d---.\r\n",g_step);

	int count=0;
	while(1)
	{
		count++;
		if(count >= IB_MCU_PROGRAME_RETRY_TIME){
			fprintf(stderr, "MCU_CMD_EARSE_MCU----failed---.\r\n");
			return -1;
		}
		if(g_step == 1)
		{
			SendCmd(buf,2,cmd,smd,sn,MCU_CMD_EARSE_MCU);
			sleep(3);
		}else{
			fprintf(stderr, "MCU_CMD_EARSE_MCU----ok----.\r\n");
			break;
		}
	}
	return 0;
}

int IBMCUControl::sendCRC()
{

		unsigned char buf[IB_MCU_UPGRADE_BUF_LEN];
		memset(buf,0,IB_MCU_UPGRADE_BUF_LEN);
		int cmd = 0x12,smd=0x00,sn=0x32;
		buf[0] = (m_bin_CRC>> 24) &0xff;//
		buf[1] = (m_bin_CRC>> 16) &0xff;//
		buf[2] = (m_bin_CRC>> 8) &0xff;//
		buf[3] = m_bin_CRC &0xff;//

		int count=0;
		while(1)
		{
			count++;
			if(count >= IB_MCU_PROGRAME_RETRY_TIME){
				fprintf(stderr, "--MCU_UPDATE_OK--failed---.\r\n");
				return -1;
			}
			if(g_step < 3){
				SendCmd(buf,4,cmd,smd,sn,MCU_UPDATE_OK);
				usleep(300*1000);
			}else{
				fprintf(stderr, "MCU_UPDATE_OK----ok---.\r\n");
				break;
			}
		}
		return 0;
}

int IBMCUControl::upgrade_Bin_Line(unsigned char *buf, unsigned int len,int index)
{
	int ret;
	int retry = 0;
	int dataLen = len;
	unsigned char checksum = 0;
	unsigned char upgradbuf[512];
	memset(upgradbuf,0,sizeof(upgradbuf));
	g_update_sn = getSN(g_update_sn);
	upgradbuf[0] = 0x12;
	upgradbuf[1] = 0x00;
	upgradbuf[2] = g_update_sn;
	upgradbuf[3] = MCU_CMD_UPDATE_MCU;
    index++;
	uint8_t hightBlock = (index>>8) &0xff;//block高位
	uint8_t lowBlock =  index&0xff;//block地址中位
	upgradbuf[4] = hightBlock;
	upgradbuf[5] = lowBlock;

	for(int i=0; i < len; i++){
		upgradbuf[6+i] = buf[i];
	}

	int total = len + 6;
	DataFrame dataframe;
	uint16_t checksum2= dataframe.checksum(upgradbuf,total);
	uint8_t hightCS =(checksum2>>8) ;
	uint8_t lowCS =(checksum2 <<8) >>8;
	upgradbuf[total]=hightCS;
	upgradbuf[total+1]=lowCS;

	//fprintf(stderr,"\n upgraddata:");
	//for(int j = 0; j < total+2;j++){
		//fprintf(stderr,"%02x",upgradbuf[j]);
	//}
	//fprintf(stderr,"\n");


	unsigned char cobsbuf[512];
	memset(cobsbuf,0,512);

	Cobs cobs;
	int len2  = cobs.StuffData(upgradbuf, total+2,cobsbuf);

	//for(int k = 0; k< len2;k++){
		//	fprintf(stderr,"%02x",cobsbuf[k]);
	//	}
	//	fprintf(stderr,"\n");
	fprintf(stderr,"m_bin_current_index=%d ",index);
	TxCmd(cobsbuf,len2);
	return 0;
}

int IBMCUControl::getDataFromBin()
{
	FILE *fp;
	m_bin_count = 0;
	char buf[MCU_UPDATE_LINE_LEN];
	fp = fopen(m_filename,"rb");
	if (fp == NULL)
	{
		fprintf(stderr,"read mcu filename failed--.\r\n");
	}

	fseek(fp, 0, SEEK_SET);
	
	while (fgets(buf, IB_SREC_MAX_LINE_LENGTH, fp))
	{
		int length = strlen(buf);
		while((length > 0) && ((buf[length-1] == '\n') || (buf[length-1] == '\r')))
		{
			buf[length-1] = '\0';
			length --;
		}
		memcpy(m_mcudata[m_bin_count],buf,strlen(buf));
		m_bin_count++;
	}
	fseek(fp, 0, SEEK_SET);
	fclose(fp);
	remove(m_filename);//删除文件
	fprintf(stderr,"m_bin_count = %d\n",m_bin_count);
	return 0;
}

//#include "ui.h"
//extern RecoveryUI* ui;

int IBMCUControl::sendPkgToMcu(bool preIndex)
{
	if (preIndex)
	{
		m_cur_index = m_cur_index> 0?m_cur_index-1:0;
	}
	fprintf(stderr,"sendPkgToMcu m_cur_index = %d\n",m_cur_index);
	if (m_cur_index >= 0 && m_cur_index < m_bin_count)
	{
		char* buffer = m_mcudata[m_cur_index];
		int length = strlen(buffer);
		if(m_cur_index == m_bin_count-1)
		{
			//保留最后一帧数据，等待soc重启前发送，因为mcu收到该数据会立即断电
			m_last_frame_mcudata = buffer;
			m_last_frame_mcudata_len = length;
			m_cur_update = false;
		}else
		{
			sendUpgradeData(buffer,length,0);
		}
		m_cur_index++;
	}
	return 0;
}

/**
 * 发送最后一帧数据
 **/
int IBMCUControl::sendLastPkgToMcu(){
	if(m_last_frame_mcudata_len >0){
		sendUpgradeData(m_last_frame_mcudata,m_last_frame_mcudata_len,1);
	}
	return 0;
}

int IBMCUControl::sendUpgradeData(char* buffer,int length,int last)
{
	McuDataFrame frame;
	uint8_t upgradbuf[length+1];
	memset(upgradbuf,0,sizeof(upgradbuf));
	upgradbuf[0] = last;
	for (int i = 1; i < length+1; i++)
	{
		upgradbuf[i] = buffer[i-1];
	}
	frame.setData(0x0E,0x00,0x02,upgradbuf,length+1);
	TxCmd(frame.getFrame(),frame.getFrameLen());
	return 0;
}

int IBMCUControl::StartUpdataFromBin()
{
	unsigned char buf[MCU_UPDATE_LINE_LEN];
	for(int i = 0; i < m_bin_count; i++){
		if(!g_is_write_ok){
			return -1;
		}
		memset(buf,0,MCU_UPDATE_LINE_LEN);
		int index =i *MCU_UPDATE_LINE_LEN;
		memcpy(buf,&m_bindata[index],MCU_UPDATE_LINE_LEN);
		int res = upgrade_Bin_Line(buf,MCU_UPDATE_LINE_LEN,i);
		if(res < 0)
			return -1;
		
		if(i % 50 == 0) {
			printf("\n[%s]: %d blocks Updated, %d left...\n", 
				"S111 RECOVERY", 
				i, 
				m_bin_count - i - 1);
		}
		
		if(!g_is_ack_ok){
			//fprintf(stderr, "StartUpdataFromBin: pthread_mutex_lock\r\n");
			pthread_mutex_lock(&clifd_mutex);
			//fprintf(stderr, "StartUpdataFromBin: pthread_cond_wait\r\n");
			pthread_cond_wait(&clifd_cond, &clifd_mutex);
			g_is_ack_ok=false;
			fprintf(stderr, "StartUpdataFromBin: pthread_mutex_unlock\r\n");
			pthread_mutex_unlock(&clifd_mutex);
		}else{
			fprintf(stderr, "StartUpdataFromBin: g_is_ack_ok=false\r\n");
			g_is_ack_ok=false;
		}
		
	}
	return 0;
}
int IBMCUControl::StartUpdata(IBSrec *srec)
{

	unsigned char buf[IB_MCU_UPGRADE_BUF_LEN];
	memset(buf,0,IB_MCU_UPGRADE_BUF_LEN);
	int line = srec->GetUpradeTotalLine();
	fprintf(stderr, "MCU_CMD_UPGRAD_MCU----line=%d\r\n",line);
	int count = 0;
	m_sn = 0x20;
	for(int i = 0; i < line; i++)
	{
		count = 0;
		while(1)
		{
			count++;
			if(count >= IB_MCU_PROGRAME_RETRY_TIME){
				fprintf(stderr, "MCU_CMD_UPGRAD_MCU----failed---.\r\n");
				return -1;
			}
			if(!g_is_ack_ok){
				int address = srec->GetUpGradeAddr(i);
				int len = srec->GetUpradeLen(i);
				srec->GetData(buf,len,i);
				UpgradeLine(buf,len,address);
				//usleep(30*1000);
				pthread_mutex_lock(&clifd_mutex);
				pthread_cond_wait(&clifd_cond, &clifd_mutex);
				pthread_mutex_unlock(&clifd_mutex);
			}else{
				g_is_ack_ok = false;
				break;
			}
		}//while
	}//for
	return 0;

}

int IBMCUControl::checkFile()
{
	m_srec =  new IBSrec(m_filename);
	if(m_srec->CheckFile() < 0)
		return -1;
	return 0;

}

void* send_function(void *argv)
{	
	IBMCUControl* control = (IBMCUControl*)argv;
	while (m_cur_update)
	{
		pthread_mutex_lock(&clifd_mutex);
		pthread_cond_wait(&clifd_cond, &clifd_mutex);
		switch (m_cur_status)
		{
		case 0x02:
			control->sendPkgToMcu(false);
			break;
		case 0x03:
			control->sendPkgToMcu(true);
			break;
		default:
			break;
		}
		pthread_mutex_unlock(&clifd_mutex);
	}
	return NULL;
}

int IBMCUControl::Upgrade(const char *filename)
{
	int ret;
	m_filename = filename;
	int res = getDataFromBin();
	if(res < 0) 
	{
		fprintf(stderr, "----check mcufile Failed\r\n");
		return -1;
	}
	fprintf(stderr, "wugs Start upgrading...\r\n");	
	ReadyUpdata();//通知mcu进入升级模式
	send_function(this);

	//pthread_t send;
	//pthread_create(&send,NULL,send_function,this);
	
	return 0;
}
