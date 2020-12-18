#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>


#define DEV_NAME "gyroscope"
#define DEV_PATH "/dev/gyroscope"
#define GYROSCOPE     0X86
#define GYROSCOPE_IOCTL_READ_TEMPERATURE        _IOR(GYROSCOPE, 0x0A, int)



/*!< Signed integer types  */
typedef   signed char     int8_t;
typedef   signed short    int16_t;
//typedef   signed long     int32_t;
 
/*!< Unsigned integer types  */
typedef unsigned char     uint8_t;  
typedef unsigned short    uint16_t;
//typedef unsigned long     uint32_t;
 
 
/*!< STM8Lx Standard Peripheral Library old types (maintained for legacy purpose) */
 
typedef int32_t  s32;
typedef int16_t s16;
typedef int8_t  s8;
 
typedef uint32_t  u32; 
typedef uint16_t u16;
typedef uint8_t  u8;


int main(int argc, char **argv) 
{
    int ret = 0;
    int fp = 0;
    int value = 0;
    s32 temperature[1] = {0};
	
    if ((ret = (fp = open(DEV_PATH, O_RDWR))) < 0) {
        printf("ioctl test error retcode = %d\n", ret);
        exit(0);
    }

    ret = ioctl(fp,GYROSCOPE_IOCTL_READ_TEMPERATURE,&value);
    printf("value = 0x%x, ret = %d\r\n",value, ret);
    ret = read(fp,temperature,1);
    printf("temperature = 0x%x, ret = %d\r\n",temperature[0], ret);
    ret = read(fp,&value,1);
    printf("temperature = 0x%x, ret = %d\r\n",value, ret);
    printf("sizeof(value) = %d, sizeof(temperature) = %d\r\n",sizeof(value),sizeof(temperature));

    return ret;
}

