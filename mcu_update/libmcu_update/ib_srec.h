/**********************************************************************
 Filename: ib_srec.h
 Description: Declare the s19 utility class.
              The s19 file should be transferred by below command:
              srec_cat src.s19 -o dst.s19 -Line_Length 46
                       -Address_length 3 -HEAder "The title"
**********************************************************************/

#ifndef _IB_SREC_H_
#define _IB_SREC_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IB_SREC_MAX_LINE_LENGTH    256
#define IB_SREC_MAX_TITLE_LENGTH   64
#define IB_SREC_MAX_LDATA_LENGTH   40
#define MCU_UPGRADE_ADDR  			0x0000  //起始地址
#define MCU_UPGRADE_BUF_LENGTH  	3*1024*1024 //最大3M大小
#define MCU_ADDRESS_MAX_LEN   1024*512 //MCU地址个数 最大值

typedef enum
{
    ENUM_IB_SREC_LINE_TYPE_start = 0x01,
    ENUM_IB_SREC_LINE_TYPE_data = 0x02,
    ENUM_IB_SREC_LINE_TYPE_S5 = 0x04,
    ENUM_IB_SREC_LINE_TYPE_stop = 0x08
} ENUM_IB_SREC_LINE_TYPE;

#define IB_SREC_LINE_ALL_TYPE (ENUM_IB_SREC_LINE_TYPE_start | \
                               ENUM_IB_SREC_LINE_TYPE_data | \
                               ENUM_IB_SREC_LINE_TYPE_stop )

class IBSrec
{
public:
    IBSrec(const char *filename);
    ~IBSrec();

    int CheckFile();
    const unsigned char *GetTitle() { return m_title; };
    int GetData(unsigned char *buf, unsigned int length,int index);
	int GetUpGradeAddr(int index);
	int GetUpradeLen(int index);
	int GetUpradeLenOver(int index);;//获取index之前数据长度
    int GetUpradeTotalLine();
    void Reset();
    unsigned GetDataLineNumber() { return m_data_line_num; };
    int GetMCU_MAX_Addr();

private:
    void StrimCRLF(char *line);
    unsigned char GetByte(const char *data);
    int CheckChar(char c);
    int CheckCheckSum(const char *line);
    int CheckS0Line(const char *line);
	 int CheckS1Line(const char *line);
    int CheckS2Line(const char *line);
    int CheckLine(const char *line);


    FILE *m_file;
    unsigned char m_title[IB_SREC_MAX_TITLE_LENGTH];
    unsigned int m_line_types;
    unsigned int m_data_line_num;
	unsigned char *bindata;
	unsigned int bindatalen;
	int m_mcu_address_index;


	unsigned int dataupgraded;
	int mcu_max_address;//MCU最大的地址
	int mcu_address[MCU_ADDRESS_MAX_LEN];//存放mcu写入地址
	int mcu_data_len[MCU_ADDRESS_MAX_LEN];//存放mcu写入地址
	
};

#endif
