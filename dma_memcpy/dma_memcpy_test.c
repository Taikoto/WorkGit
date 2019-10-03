#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>


#define MEMCPY_NO_DMA 0
#define MEMCPY_DMA    1


void print_usage(void)
{
	printf("usge:\n");
	printf("select_test <nodma|dma> \n");
}

int main(int argc ,char **argv)
{
	if(argc < 2)
	{
		print_usage();
		return -1;
	}

	int fd = open("/dev/my-dma",O_RDWR);

	if(fd < 0)
	{
		printf("open /dev/my-dma failed!\n");
		return -1;
	}

	if(strcmp("dma", argv[1]) == 0)
	{
		ioctl(fd,MEMCPY_DMA);
	}
	else
	{
		ioctl(fd,MEMCPY_NO_DMA);
	}

	close(fd);
}