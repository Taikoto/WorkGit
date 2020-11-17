/**********************************************************************
 Filename: ib_srec.cpp
 Description: Defines the s19 utility class.
**********************************************************************/

#include "ib_srec.h"
#include <string.h>
IBSrec::IBSrec(const char *filename)
{
    memset(m_title, 0, IB_SREC_MAX_TITLE_LENGTH);
    m_line_types = 0;
    m_data_line_num = 0;
    m_file = fopen(filename, "r");
    if(m_file == NULL)
    {
        fprintf(stderr, "Fail to open file: %s.\r\n", filename);
        return;
    }

	bindata = (unsigned char *)(new char[MCU_UPGRADE_BUF_LENGTH]);
	memset(mcu_address, 0, sizeof(mcu_address));
	memset(mcu_data_len, 0, sizeof(mcu_data_len));
	m_mcu_address_index = 0;
    return;
}

IBSrec::~IBSrec()
{
    if(m_file)
    {
        if(fclose(m_file))
        {
            fprintf(stderr, "Fail to close file.\r\n");
        }
    }

	delete [] bindata;
	
    return;
}

void IBSrec::StrimCRLF(char *line)
{
    int length = strlen(line);
    while((length > 0) && ((line[length-1] == '\n') || (line[length-1] == '\r')))
    {
        line[length-1] = '\0';
        length --;
    }
}

unsigned char IBSrec::GetByte(const char *data)
{
    char c[3];
    c[0] = data[0];
    c[1] = data[1];
    c[2] = '\0';
    
    return (unsigned char)strtoul(c, NULL, 16);
}

int IBSrec::CheckChar(char c)
{
    if((c >= '0') && (c <= '9'))
    {
        return 0;
    }
    
    if((c >= 'a') && (c <= 'f'))
    {
        return 0;
    }

    if((c >= 'A') && (c <= 'F'))
    {
        return 0;
    }
    
    return -1;
}

int IBSrec::CheckCheckSum(const char *line)
{
    int len = strlen(line);
    int i;
    unsigned int sum = 0;
    for(i = 2; i < len; i += 2)
    {
        sum += GetByte(&(line[i]));
    }
    
    if((sum & 0xFF) != 0xFF)
    {
        return -1;
    }
    
    return 0;
}

int IBSrec::CheckS0Line(const char *line)
{
    int len = strlen(line);
    int i;
    if(len < 10)
    {
        return -1;
    }
    
    memset(m_title, 0, IB_SREC_MAX_TITLE_LENGTH);
    
    for(i = 8; i < len - 2; i += 2)
    {
        m_title[(i-8)/2] = GetByte(&(line[i]));
    }
    
    return 0;
}

int IBSrec::CheckS2Line(const char *line)
{
	//line_S1 = S1 + length(2) + address(6) + data(length*2-8)+ checksum(2)
	unsigned char linebuf[IB_SREC_MAX_LDATA_LENGTH];
	int lineaddress,dataoffset,datalen;
	//len = length*2 + S1(2) + 2(0xd 0xa)
	int len = strlen(line) - 2;
	//len = length*2 + S1(2)
    int i;
	memset(linebuf, 0, IB_SREC_MAX_LDATA_LENGTH);

	if(len/2 > IB_SREC_MAX_LDATA_LENGTH)
	{
		return -1;
	}

	//convert start at length
    for(i = 2; i < strlen(line); i += 2)
    {
        linebuf[(i-2)/2] = GetByte(&(line[i]));
    }
	//linebuf = length(1) + address(3) + data(length-4) + checksum(1)
	//datalen	=	linebuf[0] - 4;
    datalen	=	linebuf[0] +1;
	//dataaddress
	lineaddress	=	linebuf[1];
	lineaddress	=	(lineaddress << 8) + linebuf[2];
	lineaddress	=	(lineaddress << 8) + linebuf[3];

	mcu_max_address = lineaddress;//最后一行为最大地址

	if(lineaddress < MCU_UPGRADE_ADDR)
	{
		//invalid line
		return 0;
	}

	if(datalen >0){
		//存放MCU地址
		int mcu_address_index =m_mcu_address_index;
		if(MCU_ADDRESS_MAX_LEN > mcu_address_index && mcu_address_index >= 0 ){
			mcu_address[mcu_address_index] = lineaddress; //存放MCU的地址
			mcu_data_len[mcu_address_index] = datalen;
			m_mcu_address_index++;
		}
		//fprintf(stderr, "mcuaddress=%02x ，data=",mcu_address[mcu_address_index]);
	}

	memcpy(&bindata[bindatalen],&linebuf[0],datalen);

	bindatalen = bindatalen + datalen; //数据的总长度

    return 0;
}

//line = type + length + address + data + checksum + 0xd +0xa + '\0'
int IBSrec::CheckS1Line(const char *line)
{
	  fprintf(stderr, " line s1 :%s ",line);
	//line_S1 = S1 + length(2) + address(4) + data(length*2-6)+ checksum(2)
	unsigned char linebuf[IB_SREC_MAX_LDATA_LENGTH];
	int lineaddress,dataoffset,datalen;
	//len = length*2 + S1(2) + 2(0xd 0xa)
	int len = strlen(line) - 2;
	//len = length*2 + S1(2)
    int i;
	memset(linebuf, 0, IB_SREC_MAX_LDATA_LENGTH);

	if(len/2 > IB_SREC_MAX_LDATA_LENGTH)
	{
		return -1;
	}

	//convert start at length
    for(i = 2; i < strlen(line); i += 2)
    {
        linebuf[(i-2)/2] = GetByte(&(line[i]));
    }
	//linebuf = length(1) + address(2) + data(length-3) + checksum(1)
	datalen	=	linebuf[0] - 3;//数据长度
	//dataaddress
	lineaddress	=	linebuf[1]<<8;
	lineaddress	|=	linebuf[2];
	fprintf(stderr, ",addr :%02x",lineaddress);

	if(lineaddress < MCU_UPGRADE_ADDR)
	{
		//invalid line
		return 0;
	}
	
	bindatalen = bindatalen + datalen; //数据的总长度
	fprintf(stderr, ",bindatalen :%d\n",bindatalen);
	if(datalen >0)
	{
		//存放MCU地址
		int mcu_address_index = m_mcu_address_index;
		if(MCU_ADDRESS_MAX_LEN > mcu_address_index && mcu_address_index >= 0 )
		{
			mcu_address[mcu_address_index] = lineaddress; //存放MCU的地址
			mcu_data_len[mcu_address_index] = datalen;
		}
		memcpy(&bindata[bindatalen],&linebuf[3],datalen);
		m_mcu_address_index++;
	}
    return 0;
}

//line = type + length + address + data + checksum
//type =  S0,S1,S2,S8,S9
//length = address + data + checksum
//checksum = 0xff - (byte)(length + address + data)
int IBSrec::CheckLine(const char *line)
{
    int i;
    unsigned char len;
    
    if(line[0] != 'S')
    {
        fprintf(stderr, "Not a valid srec start charecter.\r\n");
        return -1;
    }
    
    if(strlen(line) < 6)
    {
        fprintf(stderr, "Too small line length.\r\n");
        return -1;
    }
    
    if(strlen(line) % 2)
    {
        fprintf(stderr, "Line length odd.\r\n");
        return -1;
    }
    
    for(i = 1; i < strlen(line); i++)
    {
        if(CheckChar(line[i]) < 0)
        {
            fprintf(stderr, "Invalid character in the line.\r\n");
            return -1;
        }
    }
    
    len = GetByte(&(line[2]));
    if(strlen(line) != (len * 2 + 4))
    {
        fprintf(stderr, "Invalid line length.\r\n");
        return -1;
    }
    
    if(CheckCheckSum(line) < 0)
    {
        fprintf(stderr, "Invalid check sum.\r\n");
        return -1;
    }
    
    switch(line[1])
    {
        case '0':
            if(CheckS0Line(line) < 0)
            {
                fprintf(stderr, "Invalid S0 line.\r\n");
                return -1;
            }
            m_line_types |= ENUM_IB_SREC_LINE_TYPE_start;
            break;
		case '1':
			if(CheckS1Line(line) < 0)
            {
                fprintf(stderr, "Invalid S1 line.\r\n");
                return -1;
            }
            m_data_line_num ++;
            m_line_types |= ENUM_IB_SREC_LINE_TYPE_data;
			break;
        case '2':
            if(CheckS2Line(line) < 0)
            {
                fprintf(stderr, "Invalid S2 line.\r\n");
                return -1;
            }
            m_data_line_num ++;
            m_line_types |= ENUM_IB_SREC_LINE_TYPE_data;
            break;
        case '5':
            m_line_types |= ENUM_IB_SREC_LINE_TYPE_S5;
            break;
        case '8':
            m_line_types |= ENUM_IB_SREC_LINE_TYPE_stop;
            break;
		case '9':
			m_line_types |= ENUM_IB_SREC_LINE_TYPE_stop;
			break;
        default:
            fprintf(stderr, "Unsupported line type.\r\n");
            return -1;
    }
    
    return 0;
}

int IBSrec::CheckFile()
{
    int line_count = 0;
    char line[IB_SREC_MAX_LINE_LENGTH];
    
    if(!m_file)
    {
        return -1;
    }
    
    fseek(m_file, 0, SEEK_SET);
    m_data_line_num = 0;

    while(fgets(line, IB_SREC_MAX_LINE_LENGTH, m_file))
    {
        line_count ++;
        StrimCRLF(line);
      //  fprintf(stderr, "Line: %s \r\n", line);
        if(CheckLine(line) < 0)
        {
            fprintf(stderr, "Line %d invalid.\r\n", line_count);
            fseek(m_file, 0, SEEK_SET);
            return -1;
        }
    }

    fseek(m_file, 0, SEEK_SET);

    if((m_line_types & IB_SREC_LINE_ALL_TYPE) != IB_SREC_LINE_ALL_TYPE)
    {
        fprintf(stderr, "Incomplete srec file.\r\n");
        return -1;
    }
    
    return 0;
}

int IBSrec::GetUpradeTotalLine()
{
	return m_mcu_address_index;
}

//获取最大的地址 即 S2标签的最后一行的地址
int IBSrec::GetMCU_MAX_Addr()
{
	//fprintf(stderr," mcu_max_addres==0x%x\n",mcu_max_address);
	return mcu_max_address;
}


int IBSrec::GetUpGradeAddr(int index)
{

	if(index <= m_mcu_address_index){
		return mcu_address[index];
	}
	return 0;

}

int IBSrec::GetUpradeLenOver(int index)//获取index之前数据长度
{
	if(index <= m_mcu_address_index)
	{
		int len = 0;
		for(int i = 0 ; i < index; i++){
			len =len+mcu_data_len[i];
		}
		return len;
	}
	return 0;
}
int IBSrec::GetUpradeLen(int index)
{
	if(index <= m_mcu_address_index)
	{
		return mcu_data_len[index];
	}
	return -1;
}

int IBSrec::GetData(unsigned char *buf, unsigned int length,int index)
{
	int len,percent;
	int overlen = GetUpradeLenOver(index);

	fprintf(stderr,"bindatalen =%d,overlen =%d,length=%d  index=%d\n",bindatalen,overlen,length,index);

	if(overlen+length > bindatalen){
		return -1;
	}

	memcpy(buf, &bindata[overlen],length);
	return length;
}


void IBSrec::Reset()
{
    if(!m_file)
    {
        return;
    }

    fseek(m_file, 0, SEEK_SET);
    
    return;
}
