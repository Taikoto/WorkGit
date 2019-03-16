#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <utils/Log.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/ioctl.h>
#include <fcntl.h>

#define GSENSOR_DEVICE_NAME         "/dev/gsensor"
#define GSENSOR							0x85

#define HEARTRATE_DEVICE_NAME         "/dev/heartrate"
#define ALS_PS_DEVICE_NAME         "/dev/als_ps"

#define GSENSOR_IOCTL_EINT_ON         _IO(GSENSOR, 0x14)
#define GSENSOR_IOCTL_EINT_OFF          _IO(GSENSOR, 0x15)
#define	GSENSOR_IOCTL_CLEAN_STEP       _IO(GSENSOR, 0x16) 
#define	GSENSOR_IOCTL_GET_STEP         _IOR(GSENSOR, 0x17, int)

#define DEV_LOG_TAG "gsensor"

#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,DEV_LOG_TAG,__VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,DEV_LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,DEV_LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,DEV_LOG_TAG,__VA_ARGS__)
#define  LOGF(...)  __android_log_print(ANDROID_LOG_FTAL,DEV_LOG_TAG,__VA_ARGS__)

void eint_start_work(void)
{
    int fd = -1;
    fd = open(GSENSOR_DEVICE_NAME, O_RDWR);
    if(fd == -1) 
    {
        LOGI("Failed to open device:gsensor_app_control.");
        return;
    }

    LOGI("Successed to open device:gsensor_app_control.");
    if (ioctl(fd, GSENSOR_IOCTL_EINT_ON) < 0)
    {
        LOGI("gsensor_app_control start failed.");
    } else {
        LOGI("gsensor_app_control start successed.");	
    }
    close(fd);
}

void eint_stop_work(void)
{
    int fd = -1;
    fd = open(GSENSOR_DEVICE_NAME, O_RDWR);
    if(fd == -1) 
    {
        LOGI("Failed to open device:gsensor_app_control.");
        return;
    }

    LOGI("Successed to open device:gsensor_app_control.");
    if (ioctl(fd, GSENSOR_IOCTL_EINT_OFF) < 0)
    {
        LOGI("gsensor_app_control stop failed.");
    } else {
        LOGI("gsensor_app_control stop successed.");	
    }
    close(fd);
}

void gsensor_clear_step_count(void)
{
    int fd = -1;
    fd = open(GSENSOR_DEVICE_NAME, O_RDWR);
    if(fd == -1) 
    {
        LOGI("Failed to open device:gsensor_step_control.");
        return;
    }

    LOGI("Successed to open device:gsensor_step_control.");
    if (ioctl(fd, GSENSOR_IOCTL_CLEAN_STEP) < 0)
    {
        LOGI("gsensor_step_control clear failed.");
    } else {
        LOGI("gsensor_step_control clear successed.");	
    }
    close(fd);
}

int gsensor_get_step_count(void)
{
    int fd = -1;
    int step_count = 0;

    fd = open(GSENSOR_DEVICE_NAME, O_RDWR);
    if(fd == -1) 
    {
        LOGI("Failed to open device:gsensor_step_control.");
        return step_count;
    }

    LOGI("Successed to open device:gsensor_step_control.");
    if (ioctl(fd, GSENSOR_IOCTL_GET_STEP,&step_count) < 0)
    {
        LOGI("gsensor_step_control get failed.");
    } else {
        LOGI("gsensor_step_control get successed.");	
    }
    close(fd);

    return step_count;
}

int is_pedometer_available(void)
{
    int fd = -1;
    int available = 1;

    LOGI("[is_pedometer_available]");
    fd = open(GSENSOR_DEVICE_NAME, O_RDWR);
    if(fd == -1) 
    {
        LOGI("Failed to open device: gsensor.");
        available = 0;
    }
    close(fd);

    return available;
}

int is_heartrate_available(void)
{
    int fd = -1;
    int available = 1;

    LOGI("[is_heartrate_available]");
    fd = open(HEARTRATE_DEVICE_NAME, O_RDWR);
    if(fd == -1) 
    {
        LOGI("Failed to open device: heart rate.");
        available = 0;
    }
    close(fd);
    return available;
}

int is_light_perception_available(void)
{
    int fd = -1;
    int available = 1;

    LOGI("[is_light_perception_available]");
    fd = open(ALS_PS_DEVICE_NAME, O_RDWR);
    if(fd == -1) 
    {
        LOGI("Failed to open device: als_ps.");
        available = 0;
    }
    close(fd);
    
    return available;
}

#ifdef __cplusplus
}
#endif


