 
#ifndef ANDROID_INCLUDE_HARDWARE_GYRO_TEMP_H
#define ANDROID_INCLUDE_HARDWARE_GYRO_TEMP_H
 
#include <stdbool.h>
#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>
 
#include <hardware/hardware.h>
//#include <hardware/hw_auth_token.h>
 
#define GYRO_TEMP_HARDWARE_MODULE_ID "gyro_temp"
 
typedef struct gyro_temp_module {
    struct hw_module_t common;
 
}gyro_temp_module_t;
 
typedef struct gyro_temp_device {
    struct hw_device_t common;  
    int fd;
    int (*device_ctrl)(struct gyro_temp_device* dev, int value);
    int (*device_read)(struct gyro_temp_device* dev, int value);
} gyro_temp_device_t;
 
 
#endif  // ANDROID_INCLUDE_HARDWARE_GYRO_TEMP_H

