/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#define LOG_TAG "eqc_algorithm_jni"

#include <jni.h>
#include <cutils/jstring.h>
#include <utils/Log.h>
#include <cutils/xlog.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>  
#include <sys/socket.h>  
#include <sys/un.h> 
#include <poll.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <unistd.h>
#include <android_runtime/AndroidRuntime.h>
#include <android_runtime/Log.h>
//#include <private/android_filesystem_config.h> // for AID_SYSTEM
#include <hardware/eqc_algorithm_client.h>

namespace android {
typedef void (*FuncPtrChar) (char*);

#define EQC_ALGORITHM_DEBUG  0

#ifdef __cplusplus
extern "C" {
#endif
void eint_start_work(void);
void eint_stop_work(void);
void gsensor_clear_step_count(void);
int gsensor_get_step_count(void);
int is_pedometer_available(void);
int is_heartrate_available(void);
int is_light_perception_available(void);
#ifdef __cplusplus
}
#endif

static const char *classPathNameRx = "com/enqualcomm/support/EqcAlgorithmNative";

static JavaVM *g_algorithm_callback_jvm = NULL;


static const AlgorithmInterface* g_algorithm_interface = NULL;
LocationData g_callback_location_data;
static long g_total_steps_count = 0;	
int g_callback_cmd_for_replying = 0;

int g_is_cycle_open_gps = 0;
gps_close_param g_gps_close_param = {0,0,0,0};

int g_next_cycle_start_time = 0;
int g_setting_network = 0;
int g_setting_airplane_mode_enable = 0;
typedef enum
{
	CMD_OPEN_GPS = 0,
	CMD_CLOSE_GPS,
	CMD_GET_ATTACH_LBS,
	CMD_REPLY_LOCATION_DATA,
	CMD_GET_ATTACH_ALL,
        CMD_SET_NETWORK,
        CMD_SET_AIRPLANE_MODE
}callback_cmd;

typedef struct
{
	int is_open_gps;
	int is_close_gps;
	int is_get_attach_lbs;
        int is_location_data;
        int is_reset_next_cycle;
}cmd_callbak_struct;
cmd_callbak_struct g_cmd_callback = {0,0,0,0,0};
extern void algorithm_pthread_run_hdlr(void);
extern void callback_to_java_function(callback_cmd cmd);
static void eqc_checkAndClearExceptionFromCallback(JNIEnv* env, const char* methodName) {
    if (env->ExceptionCheck()) {
        ALOGE("An exception was thrown by callback '%s'.", methodName);
        //LOGE_EX(env);
        env->ExceptionClear();
    }
}


jint EqcAlgorithm_set_nmea_data(JNIEnv *env, jobject thiz,jstring nmea)
{
        const char*nmea_buf = env->GetStringUTFChars(nmea,0);
        ALOGI(" NMEA =%s \n",nmea_buf);
        g_algorithm_interface->set_nmea_data(nmea_buf);
        return 0;
}


jint EqcAlgorithm_set_gsensor_eint(JNIEnv *env, jobject thiz,jfloat x ,jfloat y,jfloat z,jint eint)
{
        ALOGI(" eint =%d \n",eint);
        g_algorithm_interface->set_gsensor_eint(&x,&y,&z,&eint);
        return 0;
}

jint EqcAlgorithm_set_fence_param(JNIEnv *env, jobject thiz,jint type,jint status)
{
        g_algorithm_interface->set_fence_status(&type,&status);
        return 0;
}

jint EqcAlgorithm_notify_current_airplane_mode(JNIEnv *env, jobject thiz,jint enable)
{
        g_algorithm_interface->set_airplane_mode(&enable);
        return 0;
}

jint EqcAlgorithm_notify_current_network(JNIEnv *env, jobject thiz,jint network)
{
        g_algorithm_interface->set_network(&network);
        return 0;
}

jint EqcAlgorithm_Gsensor_eint_work_start(JNIEnv *env, jobject thiz)
{
	eint_start_work();
	return 0;
}

jint EqcAlgorithm_Gsensor_eint_work_stop(JNIEnv *env, jobject thiz)
{
	eint_stop_work();
	return 0;
}

jint EqcAlgorithm_Gsensor_clear_step(JNIEnv *env, jobject thiz)
{
	gsensor_clear_step_count();
	return 0;
}

jint EqcAlgorithm_Gsensor_get_step_count(JNIEnv *env, jobject thiz)
{
        int step_count = 0;
	step_count = gsensor_get_step_count();
        ALOGI("get step count =%d",step_count);
	return 0;
}

jint EqcAlogorithm_set_WorkMode(JNIEnv *env, jobject thiz,jint mode,jint time)
{
	location_work_mode_struct work_mode;

	memset(&work_mode,0,sizeof(work_mode));
	work_mode.gap_time = time;
	work_mode.mode = (LOCATION_WORK_MODE)mode;
	ALOGI("MODE=%d,time=%d",work_mode.mode,work_mode.gap_time);
	g_algorithm_interface->set_work_mode(&work_mode);
	
	ALOGI("set mode over");
	return 0;
}

////////EqcAlogorithm_start_normal_postion_func////////////
jint EqcAlogorithm_start_normal_postion_func(JNIEnv *env, jobject thiz)
{
	return 0;
}

jint EqcAlogorithm_start_postion_func_once(JNIEnv *env, jobject thiz,jint type)
{
        position_type_enum tmp_type ;
#if 0
        if(type == 0)
        {
                     tmp_type = position_type_power_on;     
        }
        else if(type == 1)
        {
                     tmp_type = position_type_power_off;            
        }
        else if(type == 2)
        {
                     tmp_type = position_type_SOS;            
        }
        else if(type == 3)
        {
                     tmp_type = position_type_ss;            
        }
#endif
        tmp_type = (position_type_enum)type;
	g_algorithm_interface->location_once(&tmp_type);
	return 0;
}

///////////////////////////////////

////////mmi_factory_start_gps_action////////////
	
jint mmi_factory_start_gps_action(JNIEnv *env, jobject thiz)
{
	return 0;
}
	
///////////////////////////////////

////////mmi_factory_get_gps_data////////////
jint mmi_factory_get_gps_data(JNIEnv *env, jobject thiz, jintArray cno, jintArray sv)
{
	return 0;
}


///////////////////////////////////

////////EqcAlogorithm_get_total_pedometers()////////////
jlong EqcAlogorithm_get_total_pedometers(JNIEnv *env, jobject thiz)
{
	g_algorithm_interface->get_pedometer();
	return 0;
}


///////////////////////////////////

////////EqcAlogorithm_clear_params_of_pedometer()////////////
jint EqcAlogorithm_clear_params_of_pedometer(JNIEnv *env, jobject thiz)
{
	g_algorithm_interface->make_clear_pedometer();
	return 0;
}


jint EqcAlogorithm_reply_attach_cell_info(JNIEnv *env, jobject thiz,jint arfcn,jint bsic,jint rxlev,jint mcc,jint mnc,jint lac,jlong cell_id)
{
	cell_info_struct cell_info;

	memset(&cell_info,0,sizeof(cell_info));
	cell_info.arfcn = arfcn;
	cell_info.bsic = bsic;
	cell_info.rxlev = rxlev;
	cell_info.mcc = mcc;
	cell_info.mnc = mnc;
	cell_info.lac = lac;
	cell_info.cell_id = cell_id;
	ALOGI(" 1 lac=%d,cell_id=%ld",cell_info.lac,cell_info.cell_id);
	g_algorithm_interface->reply_attach_cellinfo(&cell_info);
	ALOGI("reply attach cellid over");
	return 0;
}

jint EqcAlgorithm_is_pedometer_available(JNIEnv *env, jobject thiz)
{
	return is_pedometer_available();
}

jint EqcAlgorithm_is_heartrate_available(JNIEnv *env, jobject thiz)
{
	return is_heartrate_available();
}

jint EqcAlgorithm_is_light_perception_available(JNIEnv *env, jobject thiz)
{
	return is_light_perception_available();
}

////////////////////////////////
static void algorithm_recv_location_callback(LocationData *location_data)
{
	ALOGI("callback  start \n");
	memset(&g_callback_location_data,0,sizeof(g_callback_location_data));
	memcpy(&g_callback_location_data,location_data,sizeof(LocationData));
	g_cmd_callback.is_location_data = 1;
        algorithm_pthread_run_hdlr();
}

static void algorithm_pedometer_get_callback(kal_int32 * steps)
{
	ALOGI("2callback  start \n");
	g_total_steps_count = (long)(*steps);
	algorithm_pthread_run_hdlr();
}
static void algorithm_get_cellid_callback(void * arg)
{
	ALOGI("3callback  start \n");
	g_cmd_callback.is_get_attach_lbs = 1;
        algorithm_pthread_run_hdlr();
}

static void algorithm_open_gps_callback(int * is_cycle_open)
{
	ALOGI("open gps callback  start \n");
	g_cmd_callback.is_open_gps = 1;
	g_is_cycle_open_gps = *is_cycle_open;
        algorithm_pthread_run_hdlr();
}

static void algorithm_close_gps_callback(gps_close_param*param)
{
	ALOGI("close gps callback  start \n");
	g_cmd_callback.is_close_gps = 1;
	memcpy(&g_gps_close_param,param,sizeof(gps_close_param));
        algorithm_pthread_run_hdlr();
}

static void algorithm_reset_next_cycle_callback(int *sec)
{
	ALOGI("is_reset_next_cycle  start \n");
	g_cmd_callback.is_reset_next_cycle = 1;
        g_next_cycle_start_time = *sec;
        algorithm_pthread_run_hdlr();
}

static void algorithm_set_network_callback(int *network)
{
	ALOGI("set_network  start \n");
        g_setting_network = *network;
	callback_to_java_function(CMD_SET_NETWORK);
}

static void algorithm_set_airplane_mode_callback(int *enable)
{
	ALOGI("set_airplane_mode  start \n");
        g_setting_airplane_mode_enable = *enable;
	callback_to_java_function(CMD_SET_AIRPLANE_MODE);
}

#if 0
void callback_to_java_function(callback_cmd cmd)
{
	JNIEnv *env = NULL;
	jclass cls;
	jmethodID mid;
	jobject obj = NULL;
	if(g_algorithm_callback_jvm->AttachCurrentThread(&env, NULL) != JNI_OK)
	{
		ALOGE("%s: AttachCurrentThread() failed", __FUNCTION__);
		return ;
}
	cls = env->FindClass(classPathNameRx);
	if(cls == NULL)
	{
		ALOGE("FindClass() Error.....");
		goto error; 
	}
	obj = env->AllocObject(cls);
	if(obj == NULL)
	{
		ALOGE("NEW obj() Error.....");
		goto error; 
	}

	switch(cmd)
	{
		case CMD_OPEN_GPS:
		{
			mid = env->GetStaticMethodID(cls, "reportOpenGps", "(I)V");
			if (mid == NULL) 
			{
				ALOGE("GetMethodID() Error.....");
				goto error;  
			}
			//callbakc to java 
			env->CallStaticVoidMethod(cls, mid,g_is_cycle_open_gps);
			break;
		}
		case CMD_CLOSE_GPS:
		{
			mid = env->GetStaticMethodID(cls, "reportCloseGps", "(IIII)V");
			if (mid == NULL) 
			{
				ALOGE("GetMethodID() Error.....");
				goto error;  
			}
			//callbakc to java 
			env->CallStaticVoidMethod(cls, mid,(jint)g_gps_close_param.is_cycle_upload,(jint)g_gps_close_param.is_other_upload,(jint)g_gps_close_param.is_location,(jint)g_gps_close_param.is_unknow_open);
			break;
		}
		case CMD_GET_ATTACH_LBS:
		{
			mid = env->GetStaticMethodID(cls, "reportGetAttachCell", "()V");
			if (mid == NULL) 
			{
				ALOGE("GetMethodID() Error.....");
				goto error;  
			}
			//callbakc to java 
			env->CallStaticVoidMethod(cls, mid);
			break;
		}
		case CMD_REPLY_LOCATION_DATA:
		{
			mid = env->GetStaticMethodID(cls, "reportLocation", "(IIIDDDFFIFJJDDIJI)V");
			if (mid == NULL) 
			{
				ALOGE("GetMethodID() Error.....");
				goto error;  
			}
			//callbakc to java 
			env->CallStaticVoidMethod(cls, mid ,(jint)g_callback_location_data.cycle_location_flag,(jint)g_callback_location_data.location_type,(jint)g_callback_location_data.delay_flag,
				g_callback_location_data.gps_data.latitude,g_callback_location_data.gps_data.longitude,
				g_callback_location_data.gps_data.altitude,g_callback_location_data.gps_data.speed,
                                g_callback_location_data.gps_data.bearing,g_callback_location_data.gps_data.sv_no,
				g_callback_location_data.gps_data.accuracy,(jlong)g_callback_location_data.gps_data.timestamp_cur,
                                (jlong)g_callback_location_data.gps_data.timestamp_last,g_callback_location_data.gps_data.latitude_last,g_callback_location_data.gps_data.longitude_last,
				(jint)g_callback_location_data.mode,g_callback_location_data.pedometer,(jint)g_callback_location_data.gps_data.status
				);
			break;
		}

		default:
		{

			break;
		}
	}

        error:    
	//Detach main thread
	if(g_algorithm_callback_jvm->DetachCurrentThread() != JNI_OK)
	{
		ALOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
	}
}
#else

void callback_to_java_function(callback_cmd cmd)
{
	JNIEnv *env = NULL;
	jclass cls;
	jmethodID mid;
	jobject obj = NULL;

	//Attach main thread
	if(g_algorithm_callback_jvm->AttachCurrentThread(&env, NULL) != JNI_OK)
	{
		ALOGE("%s: AttachCurrentThread() failed", __FUNCTION__);
		return ;
	}
	//find class
	cls = env->FindClass(classPathNameRx);
	if(cls == NULL)
	{
		ALOGE("FindClass() Error.....");
		goto error; 
	}
	obj = env->AllocObject(cls);
	if(obj == NULL)
	{
		ALOGE("NEW obj() Error.....");
		goto error; 
	}

	switch(cmd)
	{
		case CMD_OPEN_GPS:
		{
			mid = env->GetStaticMethodID(cls, "EqcAlgorithmNativeMethod2", "()V");
			if (mid == NULL) 
			{
				ALOGE("GetMethodID() Error.....");
				goto error;  
			}
			//callbakc to java 
			env->CallStaticVoidMethod(cls, mid);
			break;
		}
		case CMD_CLOSE_GPS:
		{
			mid = env->GetStaticMethodID(cls, "EqcAlgorithmNativeMethod3", "()V");
			if (mid == NULL) 
			{
				ALOGE("GetMethodID() Error.....");
				goto error;  
			}
			//callbakc to java 
			env->CallStaticVoidMethod(cls, mid);
			break;
		}
		case CMD_GET_ATTACH_LBS:
		{
			mid = env->GetStaticMethodID(cls, "EqcAlgorithmNativeMethod4", "()V");
			if (mid == NULL) 
			{
				ALOGE("GetMethodID() Error.....");
				goto error;  
			}
			//callbakc to java 
			env->CallStaticVoidMethod(cls, mid);
			break;
		}
		case CMD_REPLY_LOCATION_DATA:
		{
			mid = env->GetStaticMethodID(cls, "EqcAlgorithmNativeMethod1", "(IDDDFFIFJIJI)V");
			if (mid == NULL) 
			{
				ALOGE("GetMethodID() Error.....");
				goto error;  
			}
			//callbakc to java 
                        int status_gps = (jint)g_callback_location_data.gps_data.status;
                        ALOGI("g_callback_location_data.cycle_location_flag=%d,other=%d,staus=%d-%x,status_gps=%d-%x  \n",g_callback_location_data.cycle_location_flag,g_callback_location_data.other_location_type,(jint)g_callback_location_data.gps_data.status,(jint)g_callback_location_data.gps_data.status,status_gps,status_gps);

			if(g_callback_location_data.cycle_location_flag)
                        {
			env->CallStaticVoidMethod(cls, mid ,(jint)(g_callback_location_data.other_location_type+2),
				g_callback_location_data.gps_data.latitude,g_callback_location_data.gps_data.longitude,
				g_callback_location_data.gps_data.altitude,g_callback_location_data.gps_data.speed,
                                g_callback_location_data.gps_data.bearing,g_callback_location_data.gps_data.sv_no,
				g_callback_location_data.gps_data.accuracy,(jlong)g_callback_location_data.gps_data.timestamp_cur,
				(jint)g_callback_location_data.mode,(jlong)g_callback_location_data.pedometer,status_gps);
                        }
                        else
                        {
			env->CallStaticVoidMethod(cls, mid ,(jint)(g_callback_location_data.other_location_type),
				g_callback_location_data.gps_data.latitude,g_callback_location_data.gps_data.longitude,
				g_callback_location_data.gps_data.altitude,g_callback_location_data.gps_data.speed,
                                g_callback_location_data.gps_data.bearing,g_callback_location_data.gps_data.sv_no,
				g_callback_location_data.gps_data.accuracy,(jlong)g_callback_location_data.gps_data.timestamp_cur,
				(jint)g_callback_location_data.mode,(jlong)g_callback_location_data.pedometer,status_gps);
                        }
			break;
		}
		case CMD_SET_NETWORK:
		{
			mid = env->GetStaticMethodID(cls, "EqcAlgorithmNativeSetNetwork", "(I)V");
			if (mid == NULL) 
			{
				ALOGE("GetMethodID() Error.....");
				goto error;  
			}
			//callbakc to java 
			env->CallStaticVoidMethod(cls, mid,g_setting_network);
			break;
		}
		case CMD_SET_AIRPLANE_MODE:
		{
			mid = env->GetStaticMethodID(cls, "EqcAlgorithmNativeSetAirpaneMode", "(I)V");
			if (mid == NULL) 
			{
				ALOGE("GetMethodID() Error.....");
				goto error;  
			}
			//callbakc to java 
			env->CallStaticVoidMethod(cls, mid,g_setting_airplane_mode_enable);
			break;
		}
		default:
		{

			break;
		}
	}

        error:    
	//Detach main thread
	if(g_algorithm_callback_jvm->DetachCurrentThread() != JNI_OK)
	{
		ALOGE("%s: DetachCurrentThread() failed", __FUNCTION__);
	}
}
#endif

//void *algorithm_pthread_run_hdlr(void*arg)
void algorithm_pthread_run_hdlr(void)
{
	
	//while(1)
	{

		if(g_cmd_callback.is_close_gps)
		{
			g_cmd_callback.is_close_gps = 0;
			callback_to_java_function(CMD_CLOSE_GPS);
		}
		else if(g_cmd_callback.is_open_gps)
		{
			g_cmd_callback.is_open_gps = 0;
			callback_to_java_function(CMD_OPEN_GPS);
		}
		else if(g_cmd_callback.is_get_attach_lbs)
		{
			g_cmd_callback.is_get_attach_lbs = 0;
			callback_to_java_function(CMD_GET_ATTACH_LBS);
		}
		else if(g_cmd_callback.is_location_data)
		{
			g_cmd_callback.is_location_data = 0;
			callback_to_java_function(CMD_REPLY_LOCATION_DATA);
		}		
		//sleep(1);
	}

        return ;
}

algorithm_data_callbacks g_algorithm_callbacks = {
    sizeof(algorithm_data_callbacks),
    algorithm_recv_location_callback,
    algorithm_pedometer_get_callback,
    algorithm_get_cellid_callback,
    algorithm_open_gps_callback,
    algorithm_close_gps_callback,
    algorithm_reset_next_cycle_callback,
    algorithm_set_network_callback,
    algorithm_set_airplane_mode_callback,
};


static void eqc_algorithm_init_native(JNIEnv *env, jobject thiz)
{
	int err;
	hw_module_t* module;	
	pthread_t thread,thread_main;	

	env->GetJavaVM(&g_algorithm_callback_jvm);

	//pthread_create(&thread,NULL,algorithm_pthread_run_hdlr,NULL);

	err = hw_get_module(ALGORIHTM_CLIENT_HARDWARE_MODULE_ID, (hw_module_t const**)&module);

        ALOGI("err =%d",err);

	if (err == 0) 
	{
		hw_device_t* device;
		err = module->methods->open(module, ALGORIHTM_CLIENT_HARDWARE_MODULE_ID, &device);
                ALOGI("CLIENT 1 err =%d \n",err); 
		if (err == 0) 
		{
			struct algorithm_client_device_t* algorithm_device = (struct algorithm_client_device_t *)device;
			g_algorithm_interface = algorithm_device->get_algorithm_interface(algorithm_device);

			if(g_algorithm_interface)
			{
				g_algorithm_interface->init(&g_algorithm_callbacks);
                                ALOGI("CLIENT jni init \n"); 
			}
		}
	}
}
///////////////////////
#if 0
static JNINativeMethod methodsRx[] = {
  {"native_eqc_algorithm_init", "()V", (void*)eqc_algorithm_init_native }, 
  {"native_EqcAlogorithm_set_WorkMode", "(II)I", (void*)EqcAlogorithm_set_WorkMode }, 
  //{"native_EqcAlogorithm_start_normal_postion_func", "()I", (void*)EqcAlogorithm_start_normal_postion_func }, 
  {"native_EqcAlogorithm_start_postion_func_once", "(I)I", (void*)EqcAlogorithm_start_postion_func_once },
 // {"native_mmi_factory_start_gps_action", "()I", (void*)mmi_factory_start_gps_action },
 // {"native_mmi_factory_get_gps_data", "([I[I)I", (void*)mmi_factory_get_gps_data },
 // {"native_EqcAlogorithm_get_total_pedometers", "()J", (void*)EqcAlogorithm_get_total_pedometers },
 // {"native_EqcAlogorithm_clear_params_of_pedometer", "()I", (void*)EqcAlogorithm_clear_params_of_pedometer },
  {"native_EqcAlogorithm_reply_attach_cell_info","(IIIIIII)I",(void*)EqcAlogorithm_reply_attach_cell_info},
  {"native_EqcAlgorithm_set_nmea_data","(Ljava/lang/String;)I",(void*)EqcAlgorithm_set_nmea_data},
  {"native_EqcAlgorithm_set_gsensor_data","(FFFI)I",(void*)EqcAlgorithm_set_gsensor_eint},
};
#else
static JNINativeMethod methodsRx[] = {
  {"native_EqcAlgorithmMethod2", "()V", (void*)eqc_algorithm_init_native }, 
  {"native_EqcAlgorithmMethod1", "(II)I", (void*)EqcAlogorithm_set_WorkMode }, 
  {"native_EqcAlgorithmMethod3", "(I)I", (void*)EqcAlogorithm_start_postion_func_once },
  {"native_EqcAlgorithmMethod4","(IIIIIIJ)I",(void*)EqcAlogorithm_reply_attach_cell_info},
  {"native_EqcAlgorithmMethod5","(Ljava/lang/String;)I",(void*)EqcAlgorithm_set_nmea_data},
  {"native_EqcAlgorithmMethod6","(FFFI)I",(void*)EqcAlgorithm_set_gsensor_eint},
  {"native_EqcAlgorithmMethod7","(II)I",(void*)EqcAlgorithm_set_fence_param},
  {"native_EqcAlgorithmMethod8","(I)I",(void*)EqcAlgorithm_notify_current_airplane_mode},
  {"native_EqcAlgorithmMethod9","(I)I",(void*)EqcAlgorithm_notify_current_network},
  {"native_EqcAlgorithm_Gsensor_eint_work_start","()I",(void*)EqcAlgorithm_Gsensor_eint_work_start},
  {"native_EqcAlgorithm_Gsensor_eint_work_stop","()I",(void*)EqcAlgorithm_Gsensor_eint_work_stop},
  {"native_EqcAlgorithm_Gsensor_clear_step","()I",(void*)EqcAlgorithm_Gsensor_clear_step},
  {"native_EqcAlgorithm_Gsensor_get_step","()I",(void*)EqcAlgorithm_Gsensor_get_step_count},
  {"native_EqcAlgorithm_isPedometerAvailable","()I",(void*)EqcAlgorithm_is_pedometer_available},
  {"native_EqcAlgorithm_isHeartrateAvailable","()I",(void*)EqcAlgorithm_is_heartrate_available},
  {"native_EqcAlgorithm_isLightPerceptionAvailable","()I",(void*)EqcAlgorithm_is_light_perception_available},
};
#endif

/*
 * Register several native methods for one class.
 */



static jint registerNativeMethods(JNIEnv* env, const char* className,
    JNINativeMethod* gMethods, int numMethods)
{
	jclass clazz;

	clazz = env->FindClass(className);
	if(env->ExceptionCheck()){
		env->ExceptionDescribe();
		env->ExceptionClear();
	}
	if (clazz == NULL) {
	    //DBG("Native registration unable to find class '%s'", className);
	    return JNI_FALSE;
	}
	//method_reportLocationData = env->GetMethodID(clazz, "reportLocationData", "(IDDDFFFJ)V");
	if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
	    //ALOGD("RegisterNatives failed for '%s'", className);
	    return JNI_FALSE;
	}

	//ALOGD("%s, success\n", __func__);
	return JNI_TRUE;
}

/*
 * Register native methods for all classes we know about.
 *
 * returns JNI_TRUE on success.
 */
static jint registerNatives(JNIEnv* env)
{
	jint ret = JNI_FALSE;
	
	if (registerNativeMethods(env, classPathNameRx,methodsRx, 
								sizeof(methodsRx) / sizeof(methodsRx[0]))){
		ret = JNI_TRUE;
	}


	//ALOGD("%s, done\n", __func__);
	return ret;
}

}
// ----------------------------------------------------------------------------

/*
 * This is called by the VM when the shared library is first loaded.
 */
 
typedef union {
    JNIEnv* env;
    void* venv;
} UnionJNIEnvToVoid;

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    UnionJNIEnvToVoid uenv;
    uenv.venv = NULL;
    jint result = -1;
    JNIEnv* env = NULL;

    //ALOGD("JNI_OnLoad");

    if (vm->GetEnv(&uenv.venv, JNI_VERSION_1_4) != JNI_OK) {
        //ALOGD("ERROR: GetEnv failed");
        goto bail;
    }
    env = uenv.env;

    
    if (android::registerNatives(env) != JNI_TRUE) {
        //ALOGD("ERROR: registerNatives failed");
        goto bail;
    }

    result = JNI_VERSION_1_4;

bail:
    return result;
}


