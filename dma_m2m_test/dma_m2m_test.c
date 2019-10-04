#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>


#define DEVICE_NAME "dma_test"
#define DEV_PATH "/dev/dma_test"

char buf[]="111232342342342";  
char temp[64]={0}; 

void print_usage(void)
{
	printf("usge:\n");
	printf("select_test <nodma|dma> \n");
}

int main(void)
{
	int len;
	print_usage();
	
	int fd = open(DEV_PATH,O_RDWR);

	if(fd < 0)
	{
		printf("open /dev/dma_test failed!\n");
		return -1;
	}

    write(fd,buf,strlen(buf));  
  
    len = read(fd,temp,sizeof(temp));  
  
    printf("len=%d,%s \n",len,temp);  

	close(fd);
}
