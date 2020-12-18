#define LOG_TAG "GyroTemperature_JNI"
#include "jni.h"
//#include "core_jni_helpers.h"
#include "JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <utils/Log.h>
#include <hardware/hardware.h>
#include <hardware/gyro_temp.h>

 
namespace {
 
struct gyro_temp_device* gyro_temp_device = NULL;
 
 static inline int gyro_temp_device_open(const hw_module_t* module, struct gyro_temp_device** device) {
     return module->methods->open(module, GYRO_TEMP_HARDWARE_MODULE_ID, (struct hw_device_t**)device);
 }
 
static jint GyroTemperatureInit(JNIEnv* env, jobject /* clazz */) {
	ALOGE("com_android_server_GyroTemperature GyroTemperatureInit");
	const hw_module_t *hw_module = NULL;
    ALOGE("Gyro_tmep JNI: initializing......");
    if(hw_get_module(GYRO_TEMP_HARDWARE_MODULE_ID, &hw_module) == 0) {
        ALOGE("Gyro_tmep JNI: gyro_temp Stub found.");
        if(gyro_temp_device_open(hw_module, &gyro_temp_device) == 0) {
            ALOGE("Gyro_tmep JNI: gyro_temp device is open.");
            return 0;
        }
		
        ALOGE("Gyro_tmep JNI: failed to open gyro_temp device.");
        return -1;
    }
	
    ALOGE("Gyro_tmep JNI: failed to get gyro_temp stub hw_module.");
    return -1;
}

static jint GyroTemperatureNativeCtrl(JNIEnv* env, jobject clazz) {
    int value;
	
    ALOGE("com_android_server_Gyro_tempService GyroTemperatureNativeCtrl");
    if(!gyro_temp_device) {
        ALOGE("Gyro_tmep JNI: gyro_tmep_device is not open.");  
        return NULL;  
    }  

    gyro_temp_device->device_ctrl(gyro_temp_device, value);	 
    ALOGE("Gyro_tmep JNI: ioctl %d from Gyro_tmep gyro_tmep_device.", value); 
	//jstring result = (env)->NewStringUTF(value);
	 
    return value;
}

static jint GyroTemperatureNativeRead(JNIEnv* env, jobject clazz) {
    int value;
	
    ALOGE("com_android_server_Gyro_tempService GyroTemperatureNativeRead");
    if(!gyro_temp_device) {
        ALOGE("Gyro_tmep JNI: gyro_tmep_device is not open.");  
        return NULL;  
    }  

    gyro_temp_device->device_read(gyro_temp_device, value);	 
    ALOGE("Gyro_tmep JNI: read %d from Gyro_tmep gyro_tmep_device.", value); 
	 
    return value;
}
 
static const JNINativeMethod methods[] = {
    {"GyroTemperatureInit", "()I", (void*) GyroTemperatureInit},
    {"GyroTemperatureNativeCtrl", "()I", (void*) GyroTemperatureNativeCtrl},
	{"GyroTemperatureNativeRead", "()I", (void*) GyroTemperatureNativeRead},
};
}
 
namespace android {
 
int register_android_server_GyroTemperature(JNIEnv *env) {
    return jniRegisterNativeMethods(env, "com/android/server/GyroTemperature", methods, NELEM(methods));
}
}
