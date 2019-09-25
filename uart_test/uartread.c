#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <utils/Log.h>

enum parameter_type {
	PT_PROGRAM_NAME = 0,
	PT_DEV_NAME,
	PT_CYCLE,
	PT_NUM
}; 

#define DBG(string,args...) \
    do {\
        printf("%s, %s()%u---",__FILE__,__FUNCTION__,__LINE__);\
        printf(string,##args);\
        printf("\n"); \
	} while(0)
	
void usage(void)
{
	printf("you should input as:\n");
	printf("\t select_test[/dev/name][Cycle Cnt]\n");
}

int OpenDev(char *name)
{
	int fd = open(name, O_RDWR);
	if(-1==fd)
	    DBG("can't open(%s)",name);

    return fd;
}

void set_speed(int fd, int speed) {
	int i;
	int status;
	struct termios Opt = {0};
	int speed_arr[] = {B115200,B9600,};
	int name_arr[] = {115200,9600,};
	
	tcgetattr(fd,&Opt);
	
	for(i = 0; i < (int)(sizeof(speed_arr)/sizeof(int)); i++) {
		if(speed == name_arr[i])
		    break;
	}
	
	tcflush(fd,TCIOFLUSH);
	cfsetispeed(&Opt,speed_arr[i]);
	cfsetospeed(&Opt,speed_arr[i]);
	
	Opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	Opt.c_oflag &= ~OPOST;
	
	status = tcsetattr(fd,TCSANOW,&Opt);
	if(status != 0) {
		DBG("tcsetattr fd");
		return;
	}
	
	tcflush(fd, TCIOFLUSH);
}

int set_Parity(int fd, int databits, int stopbits, int parity)
{
	struct termios options;
	
	if(tcgetattr(fd,&options) != 0) {
		perror("setup serial 1");
		return -1;
	}
	
	options.c_cflag &= ~CSIZE;
	
	switch(databits) {
		case 7:
			options.c_cflag |= CS7;
			break;
		case 8:
			options.c_cflag |= CS8;
			break;
		default:
		    fprintf(stderr,"unsupported data size\n");
			return -1;		
	}
	
	switch(parity) {
		case 'n':
		case 'N':
			options.c_cflag &= ~PARENB;
			options.c_iflag &= ~INPCK;
			break;
		case 'o':
		case 'O':
			options.c_cflag |= (PARODD | PARENB);
			options.c_iflag |= INPCK;
			break;
		case 'e':
		case 'E':
			options.c_cflag |= PARENB;
			options.c_cflag &= ~PARODD;
			options.c_iflag |= INPCK;
			break;
		case 's':
		case 'S':
			options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;
			break;	
		default:
		    fprintf(stderr,"unsupported parity\n");
			return -1;	
	}
	
	switch(stopbits) {
		case 1:
			options.c_cflag &= ~CSTOPB;
			break;
		case 2:
			options.c_cflag |= CSTOPB;
			break;
		default:
			fprintf(stderr,"unsupported stop bits\n");
			return -1;
	}
	
	if(parity != 'n')
	    options.c_iflag |= INPCK;
	tcflush(fd,TCIFLUSH);
	options.c_cc[VTIME] = 150;
	options.c_cc[VMIN] = 0;
	if(tcsetattr(fd,TCSANOW,&options) != 0) {
		perror("setup serial 3");
		return -1;
	}
	
	return 0;
}

void str_print(char *buf, int len) {
	int i;
	
	for(i = 0; i < len; i++) {
		if(i%10 == 0) {
		    printf("\n");
		    ALOGI("\n");
		}
		    
		printf("0x%02x",buf[i]);
		ALOGI("0x%02x",buf[i]);
	}
	
	printf("\n");
	ALOGI("\n");
}


int main(int argc, char **argv) {
	int i = 0;
	int fd = 0;
	int cnt = 0;
	char buf[256];
	int ret;
	fd_set rd_fdset;
	struct timeval dly_tm;
	
	if(argc != PT_NUM) {
		usage();
		return -1;
	}
	
	sscanf(argv[PT_CYCLE],"%d",&cnt);
	if(cnt == 0)
	    cnt = 0xFFFF;
	    
	fd = OpenDev(argv[PT_DEV_NAME]);
	if(fd < 0) {
		printf("open /dev/tty error\r\n");
		return -1;
	}
	
	set_speed(fd,115200);
	if(set_Parity(fd,8,1,'N') == -1) {
		printf("set parity error\n");
		exit(0);
	}
	
	printf("select(%s),cnt %d. \n",argv[PT_DEV_NAME],cnt);
	while(i < cnt) {
		FD_ZERO(&rd_fdset);
		FD_SET(fd,&rd_fdset);
		
		dly_tm.tv_sec = 5;
		dly_tm.tv_usec = 0;
		memset(buf,0,256);
		
		ret = select(fd+1,&rd_fdset,NULL,NULL,&dly_tm);//&dly_tm
		DBG("select() return %d,fd = %d",ret,fd);
		if(ret == 0)
		    //continue;
		    printf("ret = 0\r\n");
		    
		if(ret < 0) {
			printf("select(%s) return %d. [%d]: %s \n", argv[PT_DEV_NAME],ret,errno,strerror(errno));
			continue;
		}
		
		i++;
		printf("uart test continue\r\n");
		
		ret = read(fd,buf,256);
		printf("cnt %d : read(%s) return %d.\n",i,argv[PT_DEV_NAME],ret);
		ALOGI("cnt %d : read(%s) return %d.\n",i,argv[PT_DEV_NAME],ret);
		str_print(buf,ret);
	}
	 
	close(fd);
	printf("uart test exit\r\n");
	return 0;	
}
