#define LOG_TAG "McuHardware"
#include <cutils/log.h>
#include <errno.h>
#include <linux/fb.h>
#include "UartInterface.h"

	//初始化设备
	int UartInterface::init_dev( MCUIOTYPE uartiotype )
	{
		int ret = 0;
		this->uart_error = false;
		printf("uartinterdace init_dev start\n");
		this->fd= open(MCUUARTPORT, O_RDWR | O_NDELAY | O_NOCTTY);
		if (-1 == this->fd) 
		{
			printf("open serial port %s fail %d", MCUUARTPORT, this->fd);
			this->uart_error = true;
			return this->fd;
		}

		fcntl(this->fd, F_SETFL, 0);//设置文件状态标志值为０
		printf("set baudrate\n");
		ret = uart_set_baudrate(this->fd, GET_BAUDRATEINDEX(BAUDRATEINDEX));//设置波特率
		if(ret == -1)
		{
			printf("uart_set_baudrat %s fail %d", MCUUARTPORT, ret);
			this->uart_error = true;
			return ret;

		}
		printf("set properties\n");
		ret = uart_set_properties(this->fd, SERIAL_DATABITS_EIGHT, SERIAL_STOPBITS_ONE, SERIAL_PARITY_NONE, SERIAL_HARDWARE_UNCTRL);
		if(ret == -1)
		{
			printf("uart_set_properties %s fail %d", MCUUARTPORT, ret);
			this->uart_error = true;
			return ret;
		}	 
		printf("uartinterdace init_dev end\n");

		return 0;
	}
	//发送数据
	int UartInterface::txdata(unsigned char *txbuf, unsigned int txcount)
	{
		unsigned char *write_data = txbuf;
		unsigned int  total_len = 0; 
		int  retval = 0, len = 0;
		unsigned int size = txcount;

		if (this->fd <= 0)
		{
			SLOGI("%s", __FUNCTION__);
		}

		//SLOGI("%s %d", __FUNCTION__, __LINE__);

		for (total_len = 0; total_len < size;)
		{
			len = write(this->fd, &write_data[total_len], size - total_len);
			printf("%s %d len = %d\n", __FUNCTION__, __LINE__,len);
			if (len > 0)
			{
				total_len += len;
			}
			else if (len < 0)
			{
				if (total_len == 0)
				{
					tcflush(this->fd, TCOFLUSH);
					return -1;
				}
				return total_len;
			}
			else
			{
				SLOGI("%s %d---lbx--write error len = %d", __FUNCTION__, __LINE__, len);
				break;
			}
		}
		return 0;
	}
	//读取数据
	int UartInterface::rxdata(unsigned char *rxbuf, unsigned int rxcount)
	{
		if (this->fd <= 0)
		{
			SLOGE("%s", __FUNCTION__);
			return -1;
		}
		int len = read(this->fd, rxbuf, rxcount);
		if (len < 0)
		{
			return -1;
		}
#if 0
		printf("wuguisheng rxdata 111\n");
		int len = 0, retval;
		fd_set readset;
		struct timeval tvTimeOut;

		if ( this->fd <=0 )
		{
			printf("%s error", __FUNCTION__);
		}

		memset(&tvTimeOut, 0x00, sizeof(tvTimeOut));
		FD_ZERO(&readset);//FD_ZERO()清零readset
		FD_SET(this->fd, &readset);//FD_SET()在readset中添加文件描述符
		tvTimeOut.tv_sec  = TIMEOUT_SEC(rxcount, name_arr[GET_BAUDRATEINDEX(BAUDRATEINDEX)]);
		tvTimeOut.tv_usec = TIMEOUT_USEC;

		printf("timeout==== %d\n",tvTimeOut.tv_sec);
		printf("select ...\n");
		retval = select(this->fd+1, &readset, NULL, NULL, NULL);

		//select 将更新这个集合，把其中不可读的套接字去掉
		//只保留符合条件的套节字在这个集合里面
		//retval = select(this->fd + 1, &readset, NULL, NULL, NULL);
		printf("wuguisheng retval = %d\n",retval);
		if (retval < 0)
		{
			return -1;
		}
		else if (retval && FD_ISSET(this->fd, &readset))//FD_ISSET()检查s是否在这个集合里面,有可读可写则返回1，否则为0
		{		
			printf("read start");
			len = read(this->fd, rxbuf, rxcount);
			printf("wuguisheng %s %d len==%d,rxcount=%d,rxbuf=%s\n", __FUNCTION__, __LINE__,len,rxcount,rxbuf);
			//SLOGD("wuguisheng %s %d len==%d,rxcount=%d,rxbuf=%s", __FUNCTION__, __LINE__,len,rxcount,rxbuf);
			//for(int i=0;i<len;i++)
				//SLOGD("%s %d rxbuf[%d]=%x", __FUNCTION__, __LINE__,i,rxbuf[i]);
			if(len < 0)
			{
				return -1;
			}
		}
#endif
		return len; 

	}
	//设置波特率
	int UartInterface::uart_set_baudrate(int handle, int bardrateindex)
	{
		struct termios options;	
		memset(&options, 0x00, sizeof(options));  

		if (bardrateindex >= BAUDRATEINDEXMAX)
		{
			printf(" not support BAUDRATEINDEXMAX %d", BAUDRATEINDEXMAX);
			return -1;
		}

		if (0 != tcgetattr(handle, &options))      
		{       
			printf("%s error", __FUNCTION__);
			return -1;
		}


		tcflush(this->fd, TCIOFLUSH);                    
		cfsetispeed(&options, speed_arr[bardrateindex]);         
		cfsetospeed(&options, speed_arr[bardrateindex]);  

		if (0 != tcsetattr(handle, TCSANOW, &options)) 
		{
			printf("%s error", tcsetattr);
			return -1;
		}

		tcflush(this->fd, TCIOFLUSH);


		return 0;
	}

	int UartInterface::uart_set_properties(int handle, unsigned char databits, unsigned char stopbits,										                                                                     unsigned char parity, unsigned char  flow_control)
	{
		struct termios options;

		if (this->fd <= 0)
		{
			return -1;
		}

		memset(&options, 0x00, sizeof(options));

		if (0 != tcgetattr(handle, &options))		
		{
			return -1;
		}

		options.c_cflag |= (CLOCAL | CREAD);	
		options.c_cflag &= ~CSIZE;

		switch (stopbits) 
		{
			case SERIAL_STOPBITS_ONE:			
				options.c_cflag &= ~CSTOPB;
				break;
			case SERIAL_STOPBITS_TWO:			
				options.c_cflag |= CSTOPB;
				break;
			default:
				return -1; 
		}

		switch (databits) 
		{
			case SERIAL_DATABITS_SEVEN:			
				options.c_cflag |= CS7;
				break;
			case SERIAL_DATABITS_EIGHT:			
				options.c_cflag |= CS8;
				break;
			default:
				return -1;
		}

		switch (flow_control) 
		{
			case SERIAL_HARDWARE_UNCTRL:
				options.c_cflag &= ~CRTSCTS;                
				options.c_iflag &= ~(IXON | IXOFF | IXANY);
				break;
			case SERIAL_HARDWARE_XONXOFF:
				options.c_iflag |= IXON | IXOFF | IXANY;    
				break;
			case SERIAL_HARDWARE_CTRL:
				options.c_cflag |= CRTSCTS;                 
				options.c_iflag &= ~(IXON | IXOFF | IXANY);
				break;
			default:
				return -1;
		}
		options.c_iflag &= IGNCR;					

		switch (parity)
		{		
			case SERIAL_PARITY_NONE: 					
				options.c_cflag &= ~PARENB;
				break;
			case SERIAL_PARITY_ODD: 					
				options.c_cflag |= PARENB;
				options.c_cflag |= PARODD;
				break;
			case SERIAL_PARITY_EVEN:					
				options.c_cflag |= PARENB;
				options.c_cflag &= ~PARODD;
				break;
			case SERIAL_PARITY_MARK:					
				options.c_cflag &= ~PARENB;
				options.c_cflag |= CSTOPB;
				break;
			case SERIAL_PARITY_SPACE:					
				options.c_cflag &= ~PARENB;
				options.c_cflag &= ~CSTOPB;
				break;
			default:
				return -1;
		}

		options.c_oflag &= ~OPOST;						

		options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
		options.c_cc[VMIN] = 0; 					
		options.c_cc[VTIME] = 0;					
		tcflush(this->fd, TCIFLUSH);

		if (0 != tcsetattr(handle, TCSANOW, &options)) 
		{
			return -1;
		}

		return 0;
	}

	int UartInterface::uart_close(int handle)
	{
		close(handle);	
		return 0;
	}
