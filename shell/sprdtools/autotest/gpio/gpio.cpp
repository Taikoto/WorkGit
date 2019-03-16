#include <cutils/log.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <cutils/misc.h>
#include <string.h>
#include <cstdlib>

#include "diag.h"
#include "autotest_modules.h"

//------------------------------------------------------------------------------
#define DRIVER_PATH  "/system/lib/modules/autotst.ko"
#define DRIVER_NAME  "autotst"
#define DEV_PATH     "/dev/autotst"

#ifdef SHARKL2_BBAT_GPIO_ADDRESS
	#define DEV_PIN_CONFIG_PATH    "/sys/kernel/debug/pinctrl/402a0000.pinctrl/pins_debug"
#elif defined(IWHALE2_BBAT_GPIO_ADDRESS)
	#define DEV_PIN_CONFIG_PATH     "/sys/kernel/debug/pinctrl/e42a0000.pinctrl/pins_debug"
#elif defined(ISHARKL2_BBAT_GPIO_ADDRESS)
	#define DEV_PIN_CONFIG_PATH     "/sys/kernel/debug/pinctrl/e42a0000.pinctrl/pins_debug"
#else
	#define DEV_PIN_CONFIG_PATH     "/sys/kernel/debug/pinctrl/e42a0000.pinctrl/pins_debug"  //"/sys/kernel/pin_debug/pin_config"
#endif

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//====================================================================
//add the setgpio function set enable or disable gpio
//step one : pin offset exchange the gpio function
//step two : check the gpio struct path
//step three : exchange the gpio function to export mode
//step four: operation the gpio function disable or enable
//step five : get the reslut the operation of the gpio
//====================================================================
int Setgpio_value(char *pin_addr,int gpio_num,int value)
{
    char cmd[255] = {0};
    int ret = 2;
    char gpio_path[255] = {0};
    char export_cmd[255] = {0};
    char out_cmd[255] = {0};
    char setval_cmd[255] = {0};
    int fd= -1;
    int num= -1;
    char *resp = 0;

    ALOGD("Setgpio_value---- the pin_addr = %s, gpio_num = %d, value = %d\n ", pin_addr, gpio_num, value );
#if 1
    if(0 != access(DEV_PIN_CONFIG_PATH, F_OK)){
        ALOGE("the path can`t find %s\n", DEV_PIN_CONFIG_PATH);
        return -1;
    }
    sprintf(cmd, "echo %s > %s", pin_addr,DEV_PIN_CONFIG_PATH);
    system(cmd);
#endif
    ALOGD("the Setgpio_value = %s\n", cmd);
    usleep(500);
    //add the gpio_num list path
    if(0 != access("/sys/class/gpio/export", F_OK)){
        ALOGE("the path can`t find %s\n", gpio_path);
        return  -1;
    }

    sprintf(export_cmd, "echo %d > /sys/class/gpio/export", gpio_num);
    //modify the gpio function set export function
    system(export_cmd);
    usleep(200);

    //moidify the gpionum list set out direction function
    char gpio_out[255] = {0};
    sprintf(gpio_out, "/sys/class/gpio/gpio%d/direction", gpio_num);
    if(0 != access(gpio_out, F_OK)){
        ALOGE("the path can`t find %s\n", gpio_out);
        return  -1;
    }
    sprintf(out_cmd, "echo out > %s", gpio_out);
    system(out_cmd);
    usleep(100);

    ALOGD(" Setgpio_value = %s\n", out_cmd);
    //moidify the gpionum list set out direction function
    char gpio_setval_path[255] = {0};
    sprintf(gpio_setval_path, "/sys/class/gpio/gpio%d/value", gpio_num);
    if(0 != access(gpio_setval_path, F_OK)){
        ALOGE("the path can`t find %s\n", gpio_setval_path);
        return  -1;
    }
    sprintf(setval_cmd, "echo %d > %s", value, gpio_setval_path);
    system(setval_cmd);
    usleep(100);
    char gpio_enable_status[3] = {0};
    if (0 == access(gpio_setval_path , F_OK)){
        fd = open(gpio_setval_path, O_RDONLY);
        if(fd < 0){
            ALOGE("the %s--- open fail file %s\n", __func__, gpio_setval_path);
            return ret;
        }else{
            memset(gpio_enable_status, 0, sizeof(gpio_enable_status));
            num = read(fd , gpio_enable_status, 3);
            close(fd);
            if(num < 0){
                ALOGE("the %s--- read fail file %s\n", __func__, gpio_setval_path);
                return ret;
            }
            gpio_enable_status[2] = '\0';
            //DBGMSG("the  gpio_enable_status value---%s\n", gpio_enable_status);
            ret = atoi(gpio_enable_status);
            ALOGD("the gpio_enable_status=value %d \n",ret);
        }
    }else{
        ALOGE("the %s---filepath can`t find  %s\n", __func__, gpio_setval_path);
    }
    usleep(200);
    return 0;//ret;
}


int Getgpio_value(char *pin_addr,int gpio_num)
{
    int ret = 0;

    return ret;
}
//----------------------------------------------------------------------------
void  cam_sd_tcard_addvoltage(void)
{
    system("echo 1 > /sys/kernel/debug/sprd-regulator/vddcamio/enable");
    usleep(50);
    system("echo 1 > /sys/kernel/debug/sprd-regulator/vddsdcore/enable");
    usleep(50);
    system("echo 1 > /sys/kernel/debug/sprd-regulator/vddsdio/enable");
    usleep(50);
    system("echo 1 > /sys/kernel/debug/sprd-regulator/vddsd/enable");
    usleep(50);
    system("echo 1 > /sys/kernel/debug/sprd-regulator/vddsim0/enable");
    usleep(50);
    system("echo 1 > /sys/kernel/debug/sprd-regulator/vddsim1/enable");
    usleep(50);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static int testGpio(const uchar * data, int data_len, uchar * rsp, int rsp_size)
{
    int ret = 0;
    ALOGD("data[0] = %d,data[1] = %d,data[2] = %x , data[3] = %x ,data[4] = %x \n", *data,data[1],data[2],data[3],data[4]);
    //as not the gpio drv
    cam_sd_tcard_addvoltage();
    char pin_addr[15] = {0};
    int  gpio_num = data[1];
    int  value = data[2];
    const char *pinconfig = "pin_config";
    sprintf(pin_addr, "0x%x", (data[3]<<8) | data[4]);
    ALOGD("Setgpio  value  pin_addr = %s!!!gpio_num= %d, value = %d\n  ", pin_addr,gpio_num, value);
    if(1 == data[0])
        ret = Setgpio_value(pin_addr, gpio_num, value);
    else if(0 == data[0])
        ret = Getgpio_value(pin_addr,gpio_num);
    ALOGD("Setgpio_value end  ret = %d!!!\n  ", ret);
    return ret;
}

extern "C"
void register_this_module(struct autotest_callback * reg)
{
   ALOGD("file:%s, func:%s\n", __FILE__, __func__);

   reg->diag_ap_cmd = DIAG_CMD_GPIO;
   reg->autotest_func = testGpio;
}
