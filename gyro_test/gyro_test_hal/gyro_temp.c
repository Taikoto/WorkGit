#define LOG_TAG "GYRO_TEMPHAL"
 
#include <malloc.h>
#include <stdint.h>
#include <log/log.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cutils/atomic.h>
#include <stdlib.h>
#include <unistd.h>
#include <hardware/hardware.h>
#include <hardware/gyro_temp.h>
#include <sys/ioctl.h>
 
#define DEVICE_NAME "/dev/gyroscope" 
#define MODULE_NAME "Default gyroscope temperature HAL" 
#define MODULE_AUTHOR "The Android Open Source Project"

#define DEV_NAME "gyroscope"
#define DEV_PATH "/dev/gyroscope"
#define GYROSCOPE     0X86
#define GYROSCOPE_IOCTL_READ_TEMPERATURE        _IOR(GYROSCOPE, 0x0A, int)
 
static int gyro_temp_device_open(const struct hw_module_t* module, const char* name, struct hw_device_t** device);
static int gyro_temp_device_close(struct hw_device_t* device);
static int gyro_temp_device_ctrl(struct gyro_temp_device* dev, int value);
static int gyro_temp_device_read(struct gyro_temp_device* dev, int value);
 
static struct hw_module_methods_t gyro_temp_module_methods = {
    .open = gyro_temp_device_open,
};
 
gyro_temp_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = 1,
        .hal_api_version = 1,
        .id = GYRO_TEMP_HARDWARE_MODULE_ID,
        .name = MODULE_NAME,
        .author = MODULE_AUTHOR,
        .methods = &gyro_temp_module_methods,
    },
};
 
static int gyro_temp_device_open(const struct hw_module_t* module, const char* name, struct hw_device_t** device)
{
	gyro_temp_device_t *dev = malloc(sizeof(gyro_temp_device_t));
    memset(dev, 0, sizeof(gyro_temp_device_t));
	
	ALOGE("gyro_temp: gyro_temp_device_open name = %s",name);
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (hw_module_t*)module;
    dev->common.close = gyro_temp_device_close;
	dev->device_ctrl = gyro_temp_device_ctrl;
	dev->device_read = gyro_temp_device_read;
    if((dev->fd = open(DEVICE_NAME, O_RDWR)) == -1) {
		ALOGE("gyro_temp: open /dev/gyroscope fail-- %s.", strerror(errno));free(dev);
        return -EFAULT;
    }
	
    *device = (hw_device_t*)&(dev->common); //
	ALOGE("gyro_temp: open /dev/gyroscope successfully.");
    return 0;
}

static int gyro_temp_device_close(struct hw_device_t* device)
{
    struct gyro_temp_device* gyro_temp_device = (struct gyro_temp_device*)device;
 
    if(gyro_temp_device) {
        close(gyro_temp_device->fd);
        free(gyro_temp_device);
    }
    return 0;
}

static int gyro_temp_device_ctrl(struct gyro_temp_device* dev, int value)
{
	int ret = 0;

    ret = ioctl(dev->fd,GYROSCOPE_IOCTL_READ_TEMPERATURE,&value);
    ALOGE("gyro_temp hal value = 0x%x, ret = %d\r\n",value, ret);
	
    return value;
}

static int gyro_temp_device_read(struct gyro_temp_device* dev, int value)
{
    int ret = 0;
	
    ALOGE("gyro_temp: read gyro_temp_device_read");
    ret = read(dev->fd,&value,1);
    ALOGE("gyro_temp hal value = 0x%x, ret = %d\r\n",value, ret);

    return value;
}

