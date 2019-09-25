#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define DEVICE_NAME   "/dev/ttyS7"//device point

int main(void)
{
    int fd;
    int ret;

    printf("uart test\r\n");

    fd = open(DEVICE_NAME,O_RDWR);//Open device ,get the handle
    if(fd < 0) {
        printf("open device %s error \n",DEVICE_NAME);
    }
   
    ret = close(fd); //close device
    printf("ret = %d \n",ret);

    return 0;
}
