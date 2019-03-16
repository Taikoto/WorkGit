#include <stdio.h>  
#include <utils/Log.h>
#include <sys/types.h>  
#include <sys/socket.h>  
//#include <sys/un.h>  
//#include <stddef.h>
#include <netinet/in.h> 
#include <fcntl.h>
//#include <sys/epoll.h>
//#include <poll.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/select.h>
#include <math.h>
#include <pthread.h>
#include <private/android_filesystem_config.h> // for AID_SYSTEM
#include <hardware/eqc_algorithm_client.h>
#include <linux/time.h>
#include <linux/timex.h>
#include <linux/rtc.h>

#define SOCKET_ALGORITHM_DOMAIN "@eqc_algoritm_server"//"/data/misc/algorithm"//"127.0.0.1:5099"//"/data/misc/algorithm"
#define EQC_ALGORITHM_NAME "/dev/eqc_algorithm"

#define LOG_TAG "eqc_algorithm_client"
#define DBGI(f,...) ALOGI("[%s]%s:line=%d"f,LOG_TAG,__func__,__LINE__,##__VA_ARGS__)
#define DBGE(f,...) ALOGE("[%s]%s:line=%d"f,LOG_TAG,__func__,__LINE__,##__VA_ARGS__)

#define ALGORITHM_CMD_STR				"\"cmd\""
#define ALGORITHM_QUO_STR 				"\""
#define ALGORITHM_COL_STR 				":"
#define ALGORITHM_COMMA_STR				","
#define ALGORITHM_PARAM_JSON_START    	"{"
#define ALGORITHM_PARAM_JSON_END	   	"}"

#define SOCKET_CONNECT_QUERY_STR "eqc algorithm server recevied you"
#define SOCKET_RESPONSE_QUERY_STR "eqc algorithm client link ok"
#define SOCKET_RECEVIE_REPLY_STR "eqc algorithm client receive ok"
#define SOCKET_SERVER_REPLY_STR "eqc algorithm server receive ok"

static algorithm_data_callbacks client_callback_backup;
///////////////////////////////////////
////////////send_data/////////////////
typedef struct
{
    GpsUtcTime      timestamp;
    double          latitude;
    double          longitude;
}gps_location_point;

gps_location_point g_last_location_point_param = {0};

extern U8 g_record_valible_index_1;
extern LOCATION_TYPE g_current_location_type;
extern int g_current_mode_status;
typedef struct
{
	char status;
	GpsLocation gps_location;
}GpsLocationData;

GpsLocationData g_location_data;
LocationData g_callback_send_location_data;
////GPS control////////

/*******************************************************************************************
宏定义
********************************************************************************************/
//#define MICRO_SEC_ONE_MILLISECONDS    1*1000
#define EQC_POSITION_DEFAULT_GAP_TIMER	1*60	// 1 分钟的节拍
#define MAX_SAMPLE_TIME             20//16
#define GPS_SEARCH_TIMEOUT          50     //GPS search time 
#define GPS_SEARCH_TIME_ADD	30
#define MAX_SN_NUMBER               20//25          //SEARCH_SATELLITE_TIME 
#define BUFF_LENGTH         MAX_SAMPLE_TIME*16
#define TIME_OF_4G_AFTER_TURN_ON  5          //10 MINUTES

/*******************************************************************************************
变量定义
********************************************************************************************/
typedef unsigned char U8;  
	
typedef enum
{
	MMI_WORK_MODE_NONE = 0,
	MMI_WORK_MODE_NO_POSITON,					// 1 无需定位
/**固定的采样频率*/	
	MMI_WORK_MODE_MULTI_POSITION_STATIC,		// 2 多种定位方式结合(GPS+LBS+WIFI)
	MMI_WORK_MODE_GPS_POSITION_STATIC,		// 3 GPS定位
	MMI_WORK_MODE_LBS_POSITION_STATIC,		// 4 LBS定位
	MMI_WORK_MODE_WIFI_POSITION_STATIC,		// 5 WIFI定位
#if 1//defined(__MMI_GPS_SAMPLE_LOW_MODE__)
/**动态的采样频率，最低一分钟一次**/
	MMI_WORK_MODE_MULTI_POSITION_DYNAMIC,	//6 多种定位方式结合(GPS+LBS+WIFI)
	MMI_WORK_MODE_GPS_POSITION_DYNAMIC,		//7 GPS定位
	MMI_WORK_MODE_LBS_POSITION_DYNAMIC,		//8 LBS定位
	MMI_WORK_MODE_WIFI_POSITION_DYNAMIC,		//9 WIFI定位
#endif
#if 1//defined(__MMI_GPS_POWER_SAVE_MODE__)
	MMI_WORK_MODE_MULTI_POSITION_POWER_SAVE,	//10 多种定位方式结合(GPS+LBS+WIFI)
	MMI_WORK_MODE_GPS_POSITION_POWER_SAVE,		//11 GPS定位
	MMI_WORK_MODE_LBS_POSITION_POWER_SAVE,		//12 LBS定位
	MMI_WORK_MODE_WIFI_POSITION_POWER_SAVE,		//13 WIFI定位
#endif
#if 1//yls 20160113
	MMI_WORK_MODE_MULTI_POSITION_HIGH_FREQUENCY,
#endif
	MMI_WORK_MODE_ALL
}EQC_POSITION_WORK_MODE;


typedef enum
{
    EQC_POSITION_NONE_STATUS = 0,            
    EQC_POSITION_OPEN_GPS,           //开GPS
    EQC_POSITION_CLOSE_GPS,           //关GPS
    EQC_POSITION_CLOSE_UART,           //关UART
}EQC_POSITION_GPS_STATUS;

typedef enum
{
    STATUS_NONE = 0,
    STATUS_TEST,
    STATUS_POWER_ON,//开机上报位置2
    STATUS_OTHER,//主动定位、SOS定位等3
    STATUS_POWER_OFF,
    STATUS_ONLY_CELL_ID,//只需要LBS定位5
    STATUS_NORMAL,//周期定位比如1分钟一次位置7
    STATUS_AGPS,
    STATUS_UNKNOW,
    STATUS_ALL
}GPS_START_STATUS;

typedef enum
{
    STATUS_NONE_MODE = 0,
    ABS_S_M0,		
    IN_S_TE_M1,
    IN_S_nTE_M2,//
    IN_nS_TE_M3,//
    IN_nS_nTE_M4,//
    OUT_nS_W_TE_M5,//
    OUT_nS_W_nTE_M6,//
    OUT_nS_nW_M7,//
    OUT_S_M8,
    PAN_E_M9,
    SCHOOL_E_M10,
    STATUS_ALL_MODE
}EQC_STATUS_MODE;

typedef enum
{
	GPS_SIGNAL_STATUS_NONE = 0,
	GPS_NO_SIGANL_FOR_POSITION,
	GPS_HAS_SIGNAL_NO_POSITION,
	GPS_POSITION_SUCCESS,
	GPS_SIGNAL_STATUS_ALL
}GPS_SIGNAL_STATUS_ENUM;

typedef struct
{
    EQC_POSITION_WORK_MODE mode;
    int gap_time;
}eqc_position_work_mode_struct;

typedef struct
{
    MMI_BOOL power_on_positon;      //开机开gps
    MMI_BOOL agps_positon;              //下载星历开gps
    MMI_BOOL other_position;            //非周期性定位
    MMI_BOOL normal_positon;        //周期定位开gps
    MMI_BOOL normal_delay_positon;        //周期定位关GPS时候，延时3分钟模式
}GPS_START_STRUCT;

typedef enum
{
	LOCATION_PROCESS_NONE = 0,
	GPS_OPEN_INIT,
	GPS_SEARCH_SATLLITE_SIGNAL,
	GPS_SEARCH_SATLLITE_FOR_LOCATION,
	GPS_CLOSE_HAS_LOCATION,
	GPS_CLOSE_FOR_TIMEOUT,
	GPS_CLOSE_FOR_NO_SIGNAL,
	LBS_LOCATION_START,
	LBS_LOCATION_DONE,
	WIFI_LOCATION_START,
	WIFI_LOCATION_DONE,
	GPS_OPEN_FAILED,
	LOCATION_PROCESS_ALL
}EQC_ALGORITHM_LOCATION_PROCESS;

typedef struct
{
	//pthread_t algorithm_main_thread;
	int unit_node_add;//such as one minute to judge if it's needed to location,sometimes it's needed to delay 30s
	
	//pthread_t algorithm_process_thread;
	GPS_START_STATUS status;
	EQC_ALGORITHM_LOCATION_PROCESS process;
	MMI_BOOL is_need_sn_check;
	MMI_BOOL is_normal_positon;
	MMI_BOOL is_other_positon;
	MMI_BOOL is_cell_id_change;
}eqc_algorithm_process;

eqc_algorithm_process g_algorithm_process = {0};
int g_gps_work_thread_working = 0;

eqc_position_work_mode_struct g_eqc_position_work_mode = {0};
GPS_START_STRUCT g_gps_start_struct = {0};
kal_int32 gps_speed = 0;
static U8 g_is_gps_in_tracker_mode = 0;
U8 g_is_need_start_gps_func = 0;
U8 g_is_start_work_in_normal = 0;

U8 p_index = 0;
U8 satellite_no[BUFF_LENGTH] = {'^'};
U8 p_index_1 = 0;
U8 valid_satellite_no[24] = {'^'};
#if 0//yls 20160220 for record gps signal
U8 p_index_record_satellite = 0;
U8 record_satellite_no[MAX_SAMPLE_TIME][24] = {'^'};
U8 p_index_current = 0;
#endif
#if 1//yls 20140610 check position    satellite_vailid
U8 g_record_valible_index_1 = 0;
#endif

#if 1
U8 g_is_need_clear_gsv_record = 1;//for GBGSV+GLGSV+GPGSV+GNGSV
#endif

LOCATION_TYPE g_current_location_type = LOCATION_BY_NONE;
int g_current_mode_status = 0;
position_type_enum g_other_position_type = position_type_none;
////

static U8 g_is_unit_time_status = 0;

static void mmi_gps_start_close(GPS_START_STATUS status);
static void gps_data_make_clear(void);
static void server_prepare_location_data_reply(MMI_BOOL is_cycle_positon,U8 is_ok);

extern U8 nmea_rmc_status_is_A(void);
extern char get_nmea_rmc_status();
extern void set_gps_location_data(U8 is_ok);
extern void gps_nmea_param_make_clear();
extern void algorithm_set_gps_speed(void);
extern void eqc_position_work_in_normal(void);
extern void eqc_algorithm_param_init(void);
extern void eqc_algorithm_work_with_unit_time(void);
///////////////////////////////
/*****LBS*****/
#define MAX_GET_ATTACH_LBS_DATA_GAP 20
#define MAX_CHANGE_CELL_ID 4
long g_cell_id_buff[MAX_CHANGE_CELL_ID] = {0,0,0,0};
long g_cell_id_buff_china[3][MAX_CHANGE_CELL_ID];
static U8 g_is_cell_id_changed = 0;
U8 g_is_lbs_search_thread_exist = 0;
U8 g_is_cell_id_changed_tmp = 1;

U8 get_status_of_attach_cell_id(void)
{
    U8 status = 0;

    status = g_is_cell_id_changed;
    g_is_cell_id_changed = 0;

    return status;
}

void algorithm_get_attach_cellinfo(cell_info_struct * cell_info)
{
	int i = 0, is_same_cell_id = 0;

        DBGI(" cell_info status =%d,cell_info->cell_id =%ld \n",cell_info->arfcn,cell_info->cell_id);
        if(cell_info->cell_id == -1)
        {
                if(cell_info->arfcn == 1)
                {
                        g_is_cell_id_changed = 1;
                } 
                g_is_unit_time_status = g_is_unit_time_status | 1;
                eqc_algorithm_work_with_unit_time();
                return;
        }
        else if((cell_info->arfcn == 0) && (cell_info->cell_id == 0))
        {
                g_is_unit_time_status = g_is_unit_time_status | 1;
                eqc_algorithm_work_with_unit_time();
                return;
        }
        else if(cell_info->arfcn == 3)
    	 {
    	 	for(i=0;i<MAX_CHANGE_CELL_ID;i++)
 		{
 			if(g_cell_id_buff[i] == cell_info->cell_id)
			{
				is_same_cell_id = 1;
				break;
			}
 		}
		
                if(!is_same_cell_id)
                {
                        for(i=0;i<MAX_CHANGE_CELL_ID-1;i++)
                        {
                                g_cell_id_buff[i] = g_cell_id_buff[i+1];
                        }
                        g_cell_id_buff[MAX_CHANGE_CELL_ID-1] = cell_info->cell_id;
                   
                        g_is_cell_id_changed = 1;
                }	
	        g_is_unit_time_status = g_is_unit_time_status | 1;
		eqc_algorithm_work_with_unit_time();		
		return;
    	 }
       // return;
#if 1
        if(cell_info->mcc == 460)
        {
                if(cell_info->lac >= 40960)//3G,g_cell_id_buff_china[1]
                {
                        for(i=0;i<MAX_CHANGE_CELL_ID;i++)
                        {
		                if(g_cell_id_buff_china[1][i] == cell_info->cell_id)
                                {
                                        is_same_cell_id = 1;
                                }
                        }

                        if(!is_same_cell_id)
                        {
                                for(i=0;i<MAX_CHANGE_CELL_ID-1;i++)
                                {
                                        g_cell_id_buff_china[1][i] = g_cell_id_buff_china[1][i+1];
                                }
                                g_cell_id_buff_china[1][MAX_CHANGE_CELL_ID-1] = cell_info->cell_id;
                   
                                g_is_cell_id_changed = 1;
                        }
                }
                else
                {
                        if(cell_info->cell_id > 65535)//4G,g_cell_id_buff_china[2]
                        {
                                for(i=0;i<MAX_CHANGE_CELL_ID;i++)
                                {
		                        if(g_cell_id_buff_china[2][i] == cell_info->cell_id)
                                        {
                                                is_same_cell_id = 1;
                                        }
                                }

                                if(!is_same_cell_id)
                                {
                                        for(i=0;i<MAX_CHANGE_CELL_ID-1;i++)
                                        {
                                                g_cell_id_buff_china[2][i] = g_cell_id_buff_china[2][i+1];
                                        }
                                        g_cell_id_buff_china[2][MAX_CHANGE_CELL_ID-1] = cell_info->cell_id;
                   
                                        g_is_cell_id_changed = 1;
                                } 
                        }
                        else//2G,g_cell_id_buff_china[0]
                        {
                                for(i=0;i<MAX_CHANGE_CELL_ID;i++)
                                {
		                        if(g_cell_id_buff_china[0][i] == cell_info->cell_id)
                                        {
                                                is_same_cell_id = 1;
                                        }
                                }

                                if(!is_same_cell_id)
                                {
                                        for(i=0;i<MAX_CHANGE_CELL_ID-1;i++)
                                        {
                                                g_cell_id_buff_china[0][i] = g_cell_id_buff_china[0][i+1];
                                        }
                                        g_cell_id_buff_china[0][MAX_CHANGE_CELL_ID-1] = cell_info->cell_id;
                   
                                        g_is_cell_id_changed = 1;
                                }  
                        }
                }
        }
        else
        {
                for(i=0;i<MAX_CHANGE_CELL_ID;i++)
                {
		        if(g_cell_id_buff[i] == cell_info->cell_id)
                        {
                                is_same_cell_id = 1;
                        }
                }

                if(!is_same_cell_id)
                {
                        for(i=0;i<MAX_CHANGE_CELL_ID-1;i++)
                        {
                                g_cell_id_buff[i] = g_cell_id_buff[i+1];
                        }
                        g_cell_id_buff[MAX_CHANGE_CELL_ID-1] = cell_info->cell_id;
                   
                        g_is_cell_id_changed = 1;
                }

        }


#else
        for(i=0;i<MAX_CHANGE_CELL_ID;i++)
        {
		if(g_cell_id_buff[i] == cell_info->cell_id)
                {
                        is_same_cell_id = 1;
                }
        }

        if(!is_same_cell_id)
        {
                for(i=0;i<MAX_CHANGE_CELL_ID-1;i++)
                {
                        g_cell_id_buff[i] = g_cell_id_buff[i+1];
                }
                g_cell_id_buff[MAX_CHANGE_CELL_ID-1] = cell_info->cell_id;
                   
                g_is_cell_id_changed = 1;
        }
#endif
        g_is_unit_time_status = g_is_unit_time_status | 1;
        eqc_algorithm_work_with_unit_time();
}

#if 0
void lbs_get_attach_request(void)
{
        client_callback_backup.attach_request(NULL);
}

void eqc_algorithm_get_lbs_run(void)
{
        g_is_lbs_search_thread_exist = 1;

        while(g_is_lbs_search_thread_exist)
        {
                lbs_get_attach_request();
                sleep(MAX_GET_ATTACH_LBS_DATA_GAP);
        }
}

void eqc_algorithm_lbs_init(void)
{
        int ret = 0;
        pthread_t thread;

        if(g_is_lbs_search_thread_exist == 0) 
        {
                ret = pthread_create(&thread,NULL,eqc_algorithm_get_lbs_run,NULL);
        }
}
#endif
/*****LBS*****/
//////////////////////////

#if 1//absolute motionless
U8 is_in_sleep_time(void)
{
        //struct timex txc;
        //struct rtc_time tm;
        U32 cur_time =0;       
 
        //do_gettimeofday(&(txc.time));

        //txc.time.tv_sec -= sys_tz.tz_minuteswest*60;
        //rtc_time_to_tm(txc.time.tv_sec,&tm);
        //cur_time =tm.tm_hour*3600+tm.tm_min*60+tm.tm_sec;

        struct timeval tv;
        struct timezone tz;
        struct tm *tm_pt;

        gettimeofday(&tv,&tz);

        DBGI("tv.sec=%d,tv.usec=%d,tz.minu=%d,tz.dsttime=%d \n",tv.tv_sec,tv.tv_usec,tz.tz_minuteswest,tz.tz_dsttime);
        tm_pt = localtime(&tv.tv_sec);
        DBGI("tm_pt.hour=%d,tm_pt.min=%d,tm_pt.sec=%d \n",tm_pt->tm_hour,tm_pt->tm_min,tm_pt->tm_sec);
        cur_time =tm_pt->tm_hour*3600+tm_pt->tm_min*60+tm_pt->tm_sec;
        if((cur_time >= 22*3600 && cur_time < 24*3600) || (cur_time >= 0 && cur_time < 6*3600))
        {
                return 1;
        }

        return 0;
}
#endif

#if 1//set network
int g_current_network_setting = 2;

void algorithm_network_mode_hdlr(int network)
{
	static int times_of_4g=0;
	
        DBGI("set network =%d,g_current_network_setting=%d \n",network,g_current_network_setting);
      //  if(g_current_network_setting == 0)
      //  	return;
       // return;//yls for test 
        if(times_of_4g < TIME_OF_4G_AFTER_TURN_ON)
        {
	        times_of_4g++;
	        return;
	}
  	       
        
        if(network != g_current_network_setting)
        {
              client_callback_backup.set_network(&network);  
	}
	
}

void algorithm_set_network(int *network)
{
        g_current_network_setting = *network;
}
#endif


///////////////////////////////
/*****SENSOR*****/
#define Th_Gsensor_Motion_Interrupt	100
#define INDOOR_CONTINUE_MOTION_COUNT   5
#define MMI_LAST_MOTION_STATUS_TIME 3
typedef struct
{
    U16 motion_status_buffer_count;
    U16 cellid_change_buffer[INDOOR_CONTINUE_MOTION_COUNT];
    U16 motion_status_buffer[INDOOR_CONTINUE_MOTION_COUNT];
    U16 motion_temp[INDOOR_CONTINUE_MOTION_COUNT];
    U16 indoor_walk_to_outside_count;
    U16 motion_eint[INDOOR_CONTINUE_MOTION_COUNT];
}MMI_INDOOR_CONTINUE_MOTION_STATUS;
MMI_INDOOR_CONTINUE_MOTION_STATUS g_motion_status_struct = {0};

int g_last_get_eint_count = 0;
int g_is_time_of_eint_by_motion_sensor = 0;
long g_total_pedeometer_count = 0;

#if 1//absolute motionless

#define Th_Gsensor_Motion_absolute_silent			2
#define MIN_LAST_TIME_MOTION_STATUS		5

typedef struct
{
	kal_uint8 g_absolute_motion_array[MIN_LAST_TIME_MOTION_STATUS];
	int count;
}absolute_motion_struct;

absolute_motion_struct g_absolute_motion_param = {0};


U8 is_in_motionless_from_gsensor_enit(void)
{
	int i = 0;

	if(g_absolute_motion_param.count < MIN_LAST_TIME_MOTION_STATUS)
		return 0;
	
	for(i = 0;i< MIN_LAST_TIME_MOTION_STATUS;i++)
	{
		if(g_absolute_motion_param.g_absolute_motion_array[i] > 0)
		{
			return 0;
		}
	}	
	
	return 1;
}

U8 is_out_motionless_from_gsensor_enit(void)
{
	int i = 0,count_motion =0;

	if(g_absolute_motion_param.count < MIN_LAST_TIME_MOTION_STATUS)
		return 1;
	
	for(i = 0;i< MIN_LAST_TIME_MOTION_STATUS;i++)
	{
		if(g_absolute_motion_param.g_absolute_motion_array[i] == 2)
			return 1;

	}
	
	return 0;
}
#endif

#if 1//airplane mode
//////////////////////////
/*****airplane mode *****/
U8 g_is_in_home_fence = 0;
U8 g_is_in_airplane_mode = 0;

U8 is_in_home_fence(void)
{
#if 1//yls for not support fence
        return 1;
#else
        return g_is_in_home_fence;
#endif
}

void set_status_of_home_fence(int*status)
{
        g_is_in_home_fence = (U8)*status;
}


U8 is_in_airplane_mode(void)
{
        DBGI("g_is_in_airplane_mode =%d \n",g_is_in_airplane_mode);
        return g_is_in_airplane_mode;
}

void set_status_of_airplane_mode(int*enable)
{
        g_is_in_airplane_mode = (U8)*enable;
        DBGI("set g_is_in_airplane_mode =%d \n",g_is_in_airplane_mode);
}

void notify_and_change_airplane_mode(int enable)
{
        DBGI("change airplane mode =%d \n",enable);
        client_callback_backup.set_airplane_mode(&enable);
}

void algorithm_set_fence_status(int*type,int*status)
{
        if(*type == 1)
        {
                set_status_of_home_fence(status);
        }
}

void algorithm_set_airplane_mode(int*enable)
{
        if((g_is_in_airplane_mode == 1) && (*enable == 0))
        {
                memset(&g_absolute_motion_param,0,sizeof(g_absolute_motion_param));
        }
        g_is_in_airplane_mode = (U8)*enable;
        DBGI("set g_is_in_airplane_mode =%d \n",g_is_in_airplane_mode);
}
/*****airplane mode *****/
/////////////////////////
#endif

U8 get_sensor_enit_count_last_unit(void)
{
    return g_is_time_of_eint_by_motion_sensor;
}

U8 mmi_if_countinue_move_three_minute(void)
{
	if (g_eqc_position_work_mode.gap_time > 1*60)
	{
		return 0;
	}
	
	if(g_motion_status_struct.motion_status_buffer_count < 3)
		return 0;


	if((g_motion_status_struct.motion_status_buffer[g_motion_status_struct.motion_status_buffer_count-1] == 1)&&
	   (g_motion_status_struct.motion_status_buffer[g_motion_status_struct.motion_status_buffer_count-2] == 1)&&
	   (g_motion_status_struct.motion_status_buffer[g_motion_status_struct.motion_status_buffer_count-3] == 1)	
	)
	return 1;

	
	return 0;



}

void eqc_algorithm_set_work_param(void)
{
       int i = 0;

       g_is_cell_id_changed_tmp = get_status_of_attach_cell_id();
#if 1//yls 20151109
	if(g_motion_status_struct.motion_status_buffer_count >= INDOOR_CONTINUE_MOTION_COUNT)
	{
		g_motion_status_struct.motion_status_buffer_count = INDOOR_CONTINUE_MOTION_COUNT -1;
		for(i = 1; i< INDOOR_CONTINUE_MOTION_COUNT;i++)
		{
			g_motion_status_struct.motion_status_buffer[i - 1] = g_motion_status_struct.motion_status_buffer[i];
			g_motion_status_struct.cellid_change_buffer[i - 1] = g_motion_status_struct.cellid_change_buffer[i];
			g_motion_status_struct.motion_temp[i-1] = g_motion_status_struct.motion_temp[i];
			g_motion_status_struct.motion_eint[i-1] = g_motion_status_struct.motion_eint[i];
		}
		
	}
	if(g_is_time_of_eint_by_motion_sensor > 0)
	{
		g_motion_status_struct.motion_temp[g_motion_status_struct.motion_status_buffer_count] = 1;
	}
	else
	{
		g_motion_status_struct.motion_temp[g_motion_status_struct.motion_status_buffer_count] = 0;
	}
	g_motion_status_struct.motion_eint[g_motion_status_struct.motion_status_buffer_count] = g_is_time_of_eint_by_motion_sensor;
	if(g_is_cell_id_changed_tmp && (g_is_time_of_eint_by_motion_sensor < Th_Gsensor_Motion_Interrupt ))
	{
		g_motion_status_struct.cellid_change_buffer[g_motion_status_struct.motion_status_buffer_count] = 1;
		g_motion_status_struct.motion_status_buffer[g_motion_status_struct.motion_status_buffer_count] = 0;
		g_motion_status_struct.motion_status_buffer_count ++ ;

	}
	else if(g_is_cell_id_changed_tmp && (g_is_time_of_eint_by_motion_sensor >= Th_Gsensor_Motion_Interrupt ))
	{
		g_motion_status_struct.cellid_change_buffer[g_motion_status_struct.motion_status_buffer_count] = 1;
		g_motion_status_struct.motion_status_buffer[g_motion_status_struct.motion_status_buffer_count] = 1;
		g_motion_status_struct.motion_status_buffer_count ++ ;
	}
	else if(g_is_time_of_eint_by_motion_sensor >= Th_Gsensor_Motion_Interrupt )
	{
		g_motion_status_struct.cellid_change_buffer[g_motion_status_struct.motion_status_buffer_count] = 0;
		g_motion_status_struct.motion_status_buffer[g_motion_status_struct.motion_status_buffer_count] = 1;
		g_motion_status_struct.motion_status_buffer_count ++ ;
	}
	else
	{
		g_motion_status_struct.motion_status_buffer[g_motion_status_struct.motion_status_buffer_count] = 0;
		g_motion_status_struct.cellid_change_buffer[g_motion_status_struct.motion_status_buffer_count] = 0;
		g_motion_status_struct.motion_status_buffer_count ++ ;

	}

#endif

#if 1//yls 20151021
	if(g_absolute_motion_param.count >= MIN_LAST_TIME_MOTION_STATUS)
	{
		g_absolute_motion_param.count =  MIN_LAST_TIME_MOTION_STATUS -1;
		for(i = 1; i< MIN_LAST_TIME_MOTION_STATUS;i++)
		{
			g_absolute_motion_param.g_absolute_motion_array[i - 1] = g_absolute_motion_param.g_absolute_motion_array[i];
		}
	}

	if(g_is_time_of_eint_by_motion_sensor > 0)
	{
		if(g_is_time_of_eint_by_motion_sensor >= Th_Gsensor_Motion_absolute_silent)
			g_absolute_motion_param.g_absolute_motion_array[g_absolute_motion_param.count] = 2;
		else	
			g_absolute_motion_param.g_absolute_motion_array[g_absolute_motion_param.count] = 1;
		g_absolute_motion_param.count ++;
	}
	else
	{
		g_absolute_motion_param.g_absolute_motion_array[g_absolute_motion_param.count] = 0;
		g_absolute_motion_param.count ++;
	}	

#endif

        DBGI(" -1-cell_id_change=%d,sensor_eint=%d,buffer_count=%d,g_is_start_work_in_normal=%d \n",g_is_cell_id_changed_tmp,g_is_time_of_eint_by_motion_sensor,g_motion_status_struct.motion_status_buffer_count,g_is_start_work_in_normal);
#if 0//airplane mode

        if(is_in_airplane_mode())
        {
                if((is_in_sleep_time() == 0)
                  ||(is_out_motionless_from_gsensor_enit() == 1))
                {
                        notify_and_change_airplane_mode(0);
                }
                return;
        }
        else
        {
                if((is_in_sleep_time() == 1) && (is_in_home_fence() == 1) && (is_in_motionless_from_gsensor_enit() == 1))
                {
                        notify_and_change_airplane_mode(1);
                }
                
        }
#endif


        g_is_need_start_gps_func = 1;
        if(g_is_start_work_in_normal)
                eqc_position_work_in_normal();
}


void algorithm_set_gsensor_eint(float* x,float* y,float* z,int*eint_count)
{

	if((*eint_count) < 0)
        {
                 g_is_time_of_eint_by_motion_sensor = Th_Gsensor_Motion_Interrupt;
                 g_last_get_eint_count = 0;
                 g_total_pedeometer_count = 0;
        }
        else if(*eint_count > g_last_get_eint_count)
        {
                g_is_time_of_eint_by_motion_sensor = (*eint_count - g_last_get_eint_count)*2;
                g_last_get_eint_count = *eint_count;
                g_total_pedeometer_count = *eint_count;//(*eint_count)/2;
        }
        else if(*eint_count == g_last_get_eint_count)
        {
                g_is_time_of_eint_by_motion_sensor = 0;
                g_last_get_eint_count = *eint_count;
                g_total_pedeometer_count = *eint_count;//(*eint_count)/2;
        }
        else
        {
                g_is_time_of_eint_by_motion_sensor = (*eint_count)*2;//*eint_count;
                g_last_get_eint_count = *eint_count;
                g_total_pedeometer_count = *eint_count;//(*eint_count)/2;
        }
        g_is_unit_time_status = g_is_unit_time_status | 2;
        
        eqc_algorithm_work_with_unit_time();
}
/*****SENSOR*****/
//////////////////////////

void eqc_algorithm_send_data_hldr(void)
{
	if(g_callback_send_location_data.cycle_location_flag != 0)
		client_callback_backup.location(&g_callback_send_location_data);
	memset(&g_callback_send_location_data,0,sizeof(g_callback_send_location_data));
}

void eqc_algorithm_work_with_unit_time(void)
{
	pthread_t thread,thread_data_send;
	int ret = 0;

        DBGI("g_is_unit_time_status=%d \n",g_is_unit_time_status);
        if(g_is_unit_time_status == 3)
        {
                g_is_unit_time_status = 0;
                ret =  pthread_create(&thread_data_send,NULL,eqc_algorithm_send_data_hldr,NULL);
                ret = pthread_create(&thread,NULL,eqc_algorithm_set_work_param,NULL);
        }
}

U8 gps_location_status_onoff(void)
{
    //DBGI(" g_location_data.status =%d \n",g_location_data.status);
	if(get_nmea_rmc_status() == 'A')//(g_location_data.status == 'A')
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void eqc_algorithm_current_mode_do()
{
      g_current_mode_status = 1;
}


void gps_location_record_satellite_cno(U16 cno,U16 sv)
{

    if(satellite_no[0] == '^')
        memset(satellite_no, '^', sizeof(satellite_no));
    DBGI(" cno=%d, sv=%d \n",cno,sv);	
#if 0//yls 20160220 for record gps signal
    if(record_satellite_no[0][0] == '^')
        memset(record_satellite_no, '^', sizeof(record_satellite_no));
#endif
    if(g_algorithm_process.is_need_sn_check)
    {
        if(cno >= MAX_SN_NUMBER)
        {
            satellite_no[p_index] = sv;
            p_index++;
        }

#if 0//yls 20160220 for record gps signal	
	if(cno>10)//15  //20
	{
	        record_satellite_no[p_index_record_satellite-1][p_index_current] = sv;
	       // p_index_record_satellite++;
		p_index_current++;
	}
#endif
    }

    if(cno >= 30)
    {
        valid_satellite_no[p_index_1] = sv;
        p_index_1 ++;
    }

#if 1//yls 20140610 check position    satellite_vailid
    if(cno > 0)
    {
        g_record_valible_index_1 ++;
    }
#endif	
}

void gps_search_satellite_signal_param_clear(void)
{
#if 1//yls 20140610 check position    satellite_vailid
	g_record_valible_index_1 =0;
#endif

	p_index = 0;
	memset(satellite_no, '^', sizeof(satellite_no));    
#if 0//yls 20160220 for record gps signal
	p_index_record_satellite = 0;
	p_index_current = 0;
	memset(record_satellite_no, '^', sizeof(record_satellite_no));    
#endif
	p_index_1 = 0;
	memset(valid_satellite_no, '^', sizeof(valid_satellite_no));   
        g_is_need_clear_gsv_record = 0;
}

int gps_search_satellite_signal_check_timeout(void)
{
    U8 number = 0;
    int k = 0;
    MMI_BOOL need_stop_gps_in_15sec = MMI_TRUE;
#if 0//yls 20160220 for record gps signal
    U8 number_satellite = 0;
#endif
    if(g_algorithm_process.is_need_sn_check == 0)
    {
          return 1;
    }
    g_algorithm_process.is_need_sn_check = 0;

    for(k = 0; k < BUFF_LENGTH; k++)
    {
        printf("satellite_no[%d] =%d,%d",k,satellite_no[k],'^');
        if(satellite_no[k] != '^')
            number++;
        
        if(number >= 3)
        {
            need_stop_gps_in_15sec = MMI_FALSE;
            break;
        }
#if 0//yls 20160220 for record gps signal
	if(record_satellite_no[k] != '^')
		number_satellite++;
#endif		
    }

    if(need_stop_gps_in_15sec && number > 0 && number < 3)
    {
        if((satellite_no[0] ==satellite_no[1]) && (satellite_no[0] != '^'))
            need_stop_gps_in_15sec = MMI_FALSE;
        else if(nmea_rmc_status_is_A())  
        {
        	need_stop_gps_in_15sec = MMI_FALSE;
        }  
    }

    p_index = 0;
    memset(satellite_no, '^', sizeof(satellite_no));    
#if 0//yls 20160220 for record gps signal
    p_index_record_satellite = 0;
    p_index_current = 0;
    memset(record_satellite_no, '^', sizeof(record_satellite_no));    
#endif

    if(need_stop_gps_in_15sec)
    {        
            return 0;     
    }

	return 1;	
}

int gps_add_time_for_location(void)
{
	int i = 0, number = 0;

	if(gps_location_status_onoff() == 0)
	{
		for(i = 0; i < 24; i++)
		{
			if(valid_satellite_no[i] == '^')
				break;
			else
				number++;
		}

		if(number >= 3)
		{
			return 1;
		}
	}

	return 0;
}

void eqc_gps_search_hdlr(void)
{
    printf(" %s \n",__FUNCTION__);
	int ret = 0,sleep_time = 1000,is_add = 0,is_need_loop;
	int search_satellite_time = 0,signal_satellite = 0,search_time_exist = 0;
	
	search_time_exist = GPS_SEARCH_TIMEOUT - MAX_SAMPLE_TIME;
	is_need_loop = 0;
	
	do
	{
                printf(" %s process_id=%d\n",__FUNCTION__,g_algorithm_process.process);
                DBGI("1 g_algorithm_process.process=%d \n",g_algorithm_process.process);
                if((g_algorithm_process.process > GPS_OPEN_INIT)
                &&(g_algorithm_process.process < LBS_LOCATION_START)
                &&(gps_location_status_onoff())
                )
                {
                	g_algorithm_process.process = GPS_CLOSE_HAS_LOCATION;
                }
		switch(g_algorithm_process.process)
		{
			case GPS_OPEN_INIT:
			{
				g_algorithm_process.process = GPS_SEARCH_SATLLITE_SIGNAL;
				sleep_time = 1000;
				break;
			}

			case GPS_SEARCH_SATLLITE_SIGNAL:
			{
				search_satellite_time ++;
                                DBGI("2 search_satellite_time=%d \n",search_satellite_time);
				if(search_satellite_time >= MAX_SAMPLE_TIME )
				{
					signal_satellite = gps_search_satellite_signal_check_timeout();
					g_algorithm_process.is_need_sn_check = MMI_FALSE;
                                 DBGI("3 signal_satellite=%d \n",signal_satellite);
                                 //signal_satellite = 1;	//yls test 20170109			
					if(signal_satellite)
					{
						sleep_time = 1000;

						g_algorithm_process.process = GPS_SEARCH_SATLLITE_FOR_LOCATION;//
						search_satellite_time = 0;
					}
					else
					{
						g_algorithm_process.process = GPS_CLOSE_FOR_NO_SIGNAL;
						sleep_time = 100;
					}
				}
				else
				{
					sleep_time = 1000;
				}
				break;
			}
			case GPS_SEARCH_SATLLITE_FOR_LOCATION:
			{
				search_satellite_time ++;
                                DBGI("4 search_satellite_time=%d search_time_exist=%d\n",search_satellite_time,search_time_exist);
				if(search_satellite_time >= search_time_exist)
				{
					if(is_add)
					{
						g_algorithm_process.process = GPS_CLOSE_FOR_TIMEOUT;
						g_algorithm_process.unit_node_add = 0;
						sleep_time = 100;
					}
					else
					{
						signal_satellite = gps_add_time_for_location();
                                                DBGI("5 signal_satellite=%d \n",signal_satellite);
						if(signal_satellite)
						{
							g_algorithm_process.unit_node_add = GPS_SEARCH_TIME_ADD;
							is_add = 1;
							search_satellite_time = 0;
							search_time_exist = GPS_SEARCH_TIME_ADD;
							sleep_time = 1000;
						}
						else
						{
							g_algorithm_process.process = GPS_CLOSE_FOR_TIMEOUT;
							sleep_time = 100;
						}
					}
				}
				else
				{
					sleep_time = 1000;
				}
				break;
			}

			case GPS_CLOSE_HAS_LOCATION:
			{
				g_algorithm_process.unit_node_add = 0;
				sleep_time = 1000;
                                //if(is_allowed_close_gps_when_location())
                                {
                                        g_current_location_type = LOCATION_BY_GPS;
					g_algorithm_process.process = LOCATION_PROCESS_NONE;
					mmi_gps_start_close(STATUS_ALL);
                                        //set_is_allowed_close_gps_when_location(0);
                                }

				break;
			}

			case GPS_CLOSE_FOR_TIMEOUT:
			{
				//GPS_data_process_stop();
				
				if(gps_location_status_onoff() == 0)
				{
					g_algorithm_process.unit_node_add = 0;

					g_algorithm_process.process = LOCATION_PROCESS_NONE;
                                        
                                        g_current_location_type = LOCATION_BY_NONE;

					if( g_algorithm_process.is_other_positon)
						mmi_gps_start_close(STATUS_OTHER);
					if(g_algorithm_process.is_normal_positon)
						mmi_gps_start_close(STATUS_NORMAL);
					
					sleep_time = 100;
				}
				else
				{
					sleep_time = 1000;
				}
				break;
			}

			case GPS_CLOSE_FOR_NO_SIGNAL:
			{

                                g_current_location_type = LOCATION_BY_NONE;
				g_algorithm_process.unit_node_add = 0;
				g_algorithm_process.process = LOCATION_PROCESS_NONE;
				//GPS_data_process_stop();
				mmi_gps_start_close(STATUS_ALL);
				
				sleep_time = 100;
				
				break;
			}
			case GPS_OPEN_FAILED:
			{
				g_algorithm_process.unit_node_add = 0;
				g_algorithm_process.process = LOCATION_PROCESS_NONE;
				sleep_time = 10;
				break;
			}
			default:
			{
				g_algorithm_process.unit_node_add = 0;
				g_algorithm_process.process = LOCATION_PROCESS_NONE;
				sleep_time = 10;
				break;
			}
		}
        usleep(sleep_time*1000);

	}while(g_algorithm_process.process);

}

void eqc_position_process_hdlr(void)
{
    int is_cycle = 0;
	//EQC_POSITION_GPS_STATUS *status  = (EQC_POSITION_GPS_STATUS *)arg;
    printf(" %s process_id=%d\n",__FUNCTION__, g_algorithm_process.process);
    DBGI(" 2 g_algorithm_process.process=%d \n",g_algorithm_process.process);
	//if(g_algorithm_process.process == LOCATION_PROCESS_NONE)//(*status == EQC_POSITION_OPEN_GPS)
	{
        gps_data_make_clear();
		g_algorithm_process.process = GPS_OPEN_INIT;
		client_callback_backup.open_gps(&is_cycle);
		eqc_gps_search_hdlr();
	}
	
}

U8 is_need_start_according_to_last_location_status(void)
{



	return 0;
}

void algorithm_set_location_status(int status)
{
	//status : 0 not location  ; 1:location
}
#if 1//yls 20170526
#define min_no_gps_location_time  30
int g_open_gps_time = -1;
int g_gps_not_location_time = min_no_gps_location_time;


U8 is_need_to_close_gps(void)
{
    DBGI("g_open_gps_time=%d,g_gps_not_location_time=%d",g_open_gps_time,g_gps_not_location_time);

    if(gps_location_status_onoff() && (g_open_gps_time >0) && (!g_algorithm_process.is_other_positon || g_algorithm_process.is_normal_positon))
    {
    	g_open_gps_time --;
    	if(g_open_gps_time < 0)
    	   g_open_gps_time = 0;
        if(g_gps_not_location_time >= min_no_gps_location_time)
        {
    	    g_gps_not_location_time = 0; 
            return 2;  
        }
        g_gps_not_location_time = 0;
        return 0;
    }
    else if(gps_location_status_onoff() && (g_open_gps_time == -1) && g_algorithm_process.is_normal_positon)
    {
    	g_open_gps_time = 3;	
    	
        if(g_gps_not_location_time >= min_no_gps_location_time)
        {
    	    g_gps_not_location_time = 0; 
            return 2;  
        }
        g_gps_not_location_time = 0; 
    	return 0;
    }
    else if(gps_location_status_onoff() == 0)
    {
#if 0
        g_open_gps_time = -1;
        g_gps_not_location_time ++;
        if(g_gps_not_location_time >= min_no_gps_location_time)
        {
    		g_gps_not_location_time = min_no_gps_location_time;
        }
#else
    	if(g_open_gps_time != -1)
    		g_open_gps_time = 0;
    	g_gps_not_location_time ++;
    	if(g_gps_not_location_time >= min_no_gps_location_time)//120
    	{
    		g_open_gps_time = -1;
    		g_gps_not_location_time = min_no_gps_location_time;
    	}
#endif
    	return 1;
    }   
    else if(g_algorithm_process.is_other_positon && gps_location_status_onoff() && (g_open_gps_time >0))
    {
        if(g_gps_not_location_time >= min_no_gps_location_time)
        {
    	    g_gps_not_location_time = 0; 
            return 2;  
        }
    	return 0;
    }
    return 1;	
}
#endif

void mmi_gps_status_hdlr(EQC_POSITION_GPS_STATUS status)//开启关闭关UART
{
    gps_close_param param ;
    int ret = 0;
    pthread_t thread;
    U8 close_gps = 0;

    printf(" %s status = %d\n",__FUNCTION__, status);
    DBGI("status=%d \n",status);
	if((status == EQC_POSITION_CLOSE_GPS)
	||(status == EQC_POSITION_CLOSE_UART)	
	)
	{
        eqc_algorithm_current_mode_do();
#if 0//yls 20170506
        param.is_cycle_upload = MMI_FALSE;
#else
		//GPS_data_process_stop(g_algorithm_process.is_normal_positon,g_algorithm_process.is_cell_id_change,g_algorithm_process.is_other_positon);
        if(g_algorithm_process.is_cell_id_change || (gps_location_status_onoff() && g_algorithm_process.is_normal_positon))
                param.is_cycle_upload = MMI_TRUE;
        else
                param.is_cycle_upload = MMI_FALSE;
#endif
        param.is_other_upload = g_algorithm_process.is_other_positon;
        param.is_location = g_location_data.status;
        param.is_unknow_open = 0;
        #if 1//yls 20170526
        close_gps = 1; //is_need_to_close_gps();
        DBGI("close_gps = %d",close_gps);
        if(close_gps == 1)
        #endif
        client_callback_backup.close_gps(&param);
        if(gps_location_status_onoff() ==1)
             algorithm_set_gps_speed();
		g_algorithm_process.is_normal_positon = MMI_FALSE;
		g_algorithm_process.is_cell_id_change = MMI_FALSE;
		g_algorithm_process.is_other_positon = MMI_FALSE;

        if((param.is_cycle_upload || (close_gps == 2)) || param.is_other_upload)
                server_prepare_location_data_reply(param.is_cycle_upload,close_gps);
	}
	else
	{
        DBGI("g_algorithm_process.process = %d ,status=%d \n",g_algorithm_process.process,status);
		if(g_algorithm_process.process == LOCATION_PROCESS_NONE)
		{	
             //eqc_position_process_hdlr(status);
                         gps_nmea_param_make_clear();
			 ret =  pthread_create(&thread,NULL,eqc_position_process_hdlr,NULL);		
		}
	}
}


void mmi_gps_start_open(GPS_START_STATUS status)
{
    printf(" %s status = %d\n",__FUNCTION__, status);
    switch( status)
    {
        case STATUS_POWER_ON:
            g_gps_start_struct.power_on_positon = MMI_TRUE;
        break;

        case STATUS_AGPS:
            g_gps_start_struct.agps_positon = MMI_TRUE;
        break;

        case STATUS_OTHER:
            g_gps_start_struct.other_position = MMI_TRUE;
        break;

        case STATUS_NORMAL:
            g_gps_start_struct.normal_positon = MMI_TRUE;
        break;
        
        default:

        break;
    }

    mmi_gps_status_hdlr(EQC_POSITION_OPEN_GPS);
}

static void mmi_gps_start_close(GPS_START_STATUS status)
{
    DBGI("GPS close status=%d \n",status);
    switch( status)
    {
        case STATUS_POWER_ON:
            g_gps_start_struct.power_on_positon = MMI_FALSE;
            
            if( !g_gps_start_struct.power_on_positon
                && !g_gps_start_struct.agps_positon
                && !g_gps_start_struct.other_position
                && !g_gps_start_struct.normal_positon
                )
            {
	            mmi_gps_status_hdlr(EQC_POSITION_CLOSE_GPS);
            }       
                    
            break;

        case STATUS_AGPS:
            g_gps_start_struct.agps_positon = MMI_FALSE;

            if( !g_gps_start_struct.power_on_positon
                && !g_gps_start_struct.agps_positon
                && !g_gps_start_struct.other_position
                && !g_gps_start_struct.normal_positon
                )
            {
	            mmi_gps_status_hdlr(EQC_POSITION_CLOSE_GPS);
            }       
       

            break;

        case STATUS_OTHER:
            g_gps_start_struct.other_position = MMI_FALSE;
            g_gps_start_struct.power_on_positon = MMI_FALSE;
            if( !g_gps_start_struct.power_on_positon
                && !g_gps_start_struct.agps_positon
                && !g_gps_start_struct.other_position
                && !g_gps_start_struct.normal_positon
                )
            {
	            mmi_gps_status_hdlr(EQC_POSITION_CLOSE_GPS);
            }       

            break;

        case STATUS_NORMAL:
            g_gps_start_struct.normal_positon = MMI_FALSE;
            if( !g_gps_start_struct.power_on_positon
                && !g_gps_start_struct.agps_positon
                && !g_gps_start_struct.other_position
                && !g_gps_start_struct.normal_positon
                && !g_gps_start_struct.normal_delay_positon
                )
            {
	            mmi_gps_status_hdlr(EQC_POSITION_CLOSE_GPS);
            }       
   
            break;
            
        case STATUS_ALL:
            g_gps_start_struct.power_on_positon = MMI_FALSE;
            g_gps_start_struct.agps_positon = MMI_FALSE;
            g_gps_start_struct.other_position = MMI_FALSE;
            g_gps_start_struct.normal_delay_positon = MMI_FALSE;   
	     g_gps_start_struct.normal_positon = MMI_FALSE;		
            {
	            mmi_gps_status_hdlr(EQC_POSITION_CLOSE_GPS);
            }       

            break;

        default:

            break;
    }

}

U8  mmi_if_eint_enough(void)
{
	if (g_eqc_position_work_mode.gap_time <= 1*60)
	{
		if (mmi_if_countinue_move_three_minute())
		{
			return 1;
		}
		//else if (g_is_time_of_eint_by_motion_sensor > Th_Gsensor_Motion_Interrupt)
		//{
		//	return 1;
		//}
		else
		{
			return 0;
		}
	}
	else if (g_eqc_position_work_mode.gap_time <= 5*60)
	{
		if (g_is_time_of_eint_by_motion_sensor > 200)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		if (g_is_time_of_eint_by_motion_sensor > 300)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
}

void mmi_start_position_normal(void)
{
    printf(" %s \n",__FUNCTION__);
    g_gps_start_struct.normal_delay_positon = MMI_FALSE;
    g_algorithm_process.is_need_sn_check= MMI_TRUE;		
    mmi_gps_start_open(STATUS_NORMAL);
    //mmi_prepare_lbs_positon_normal();
}


void mmi_normal_position_motion_hdlr(void)
{
    printf(" %s \n",__FUNCTION__);
	U8 cell_id_change = 0;
	int network =0;
#if 0//yls 20170506
		g_algorithm_process.is_cell_id_change = MMI_FALSE;
		g_algorithm_process.is_normal_positon = MMI_TRUE;
		mmi_start_position_normal();

#else
	cell_id_change = g_is_cell_id_changed_tmp;//get_status_of_attach_cell_id();
        DBGI("cell_id_change=%d sensor_einti=%d,gps_speed=%d \n",cell_id_change,g_is_time_of_eint_by_motion_sensor,gps_speed);
	if(cell_id_change)
	{
		g_algorithm_process.is_cell_id_change = MMI_TRUE;
		g_algorithm_process.is_normal_positon = MMI_TRUE;
		mmi_start_position_normal();
		//GPS_data_process_init();
                network = 2; 
	}
	else if((gps_speed >10)
	  ||(mmi_if_eint_enough())
          ||(is_need_start_according_to_last_location_status())
	)
	{
		g_algorithm_process.is_cell_id_change = MMI_FALSE;
		g_algorithm_process.is_normal_positon = MMI_TRUE;
		mmi_start_position_normal();
                network = 2; 
	}
	else
	{
		g_algorithm_process.is_cell_id_change = MMI_FALSE;
        	g_algorithm_process.is_normal_positon = MMI_FALSE;        
		mmi_gps_start_close(STATUS_NORMAL);
                network = 0;
	}
#endif
#if 1//set network
         algorithm_network_mode_hdlr(0);//algorithm_network_mode_hdlr(network);
#endif
}

void mmi_start_position_other(void)
{
	g_algorithm_process.is_other_positon = MMI_TRUE;
	g_algorithm_process.is_need_sn_check= MMI_TRUE;	//gps sn check 
	mmi_gps_start_open(STATUS_OTHER);
}

void mmi_eqc_gps_work_in_strict_mode(void)
{
        g_is_need_start_gps_func = 0;
	mmi_normal_position_motion_hdlr();
}

void eqc_position_work_in_normal(void)
{
    printf(" %s \n",__FUNCTION__);
	int sleep_time = 0;
	
	if(g_eqc_position_work_mode.gap_time == 0)
        {
		g_eqc_position_work_mode.gap_time = EQC_POSITION_DEFAULT_GAP_TIMER;
        }
        if(g_eqc_position_work_mode.mode == 0)
        {
                g_eqc_position_work_mode.mode = MMI_WORK_MODE_MULTI_POSITION_STATIC;
                g_eqc_position_work_mode.gap_time = EQC_POSITION_DEFAULT_GAP_TIMER;
        }        

	sleep_time = g_eqc_position_work_mode.gap_time;
        DBGI("eqc_position_work_in_normal TIME=%d,mode=%d",g_eqc_position_work_mode.gap_time,g_eqc_position_work_mode.mode);
	
	do//while(1)
	{
        DBGI("g_algorithm_process.unit_node_add=%d \n",g_algorithm_process.unit_node_add);
		if(g_algorithm_process.unit_node_add ==0)
		{
			sleep_time = g_eqc_position_work_mode.gap_time;
			
			if(g_eqc_position_work_mode.mode == MMI_WORK_MODE_NO_POSITON)
			{
                                g_is_need_start_gps_func = 0;
				//mmi_eqc_gps_not_working_mode();
			}
			else if(((g_eqc_position_work_mode.mode == MMI_WORK_MODE_MULTI_POSITION_STATIC)
			||(g_eqc_position_work_mode.mode == MMI_WORK_MODE_GPS_POSITION_STATIC)
			||(g_eqc_position_work_mode.mode == MMI_WORK_MODE_MULTI_POSITION_DYNAMIC)
			||(g_eqc_position_work_mode.mode == MMI_WORK_MODE_GPS_POSITION_DYNAMIC)
			) 
			&& (g_eqc_position_work_mode.gap_time == EQC_POSITION_DEFAULT_GAP_TIMER)
			)
			{
				mmi_eqc_gps_work_in_strict_mode();//严格的算法，一般用于手表等电池容量较小的终端
			}
			else 
			{
				mmi_eqc_gps_work_in_strict_mode();//严格的算法，一般用于手表等电池容量较小的终端
			}

			gps_speed = 0;
		}
		else
		{
			sleep_time = g_algorithm_process.unit_node_add + 10;
		}
		
                DBGI(" is need sleep g_is_need_start_gps_func =%d",g_is_need_start_gps_func);
                if(g_is_need_start_gps_func)
		sleep(sleep_time);

	}while(g_is_need_start_gps_func);
}

void eqc_algorithm_set_work_mode(int mode,int time)
{
	g_eqc_position_work_mode.gap_time = time;

	switch(mode)
	{
		case 0:
		case 4:

			g_eqc_position_work_mode.mode = MMI_WORK_MODE_NO_POSITON;
			break;

		case 1:
		case 2:

			g_eqc_position_work_mode.mode = MMI_WORK_MODE_MULTI_POSITION_STATIC;
			break;

		default:

			break;
	}
	
	//eqc_algorithm_lbs_init();
}


void eqc_algorithm_param_init(void)
{
	int ret = 0;
        pthread_t thread;
        
        if(g_eqc_position_work_mode.mode == 0)
             eqc_algorithm_set_work_mode(1,60);
        g_is_start_work_in_normal = 1;
	g_algorithm_process.process = LOCATION_PROCESS_NONE;
	g_algorithm_process.unit_node_add = 0;
        //sleep(1);
	//ret= pthread_create(&thread,NULL,eqc_position_work_in_normal,NULL);
}


//////////end////////////////////////

///////////////////////////////////////
////////////send_data/////////////////


static void gps_data_make_clear(void)
{
	memset(&g_location_data,0,sizeof(GpsLocationData));
}

void gps_data_set_for_send(char status,GpsLocation location_data)
{
	g_location_data.status = status;
	memcpy(&g_location_data.gps_location,&location_data,sizeof(GpsLocation));
}

static void server_prepare_location_data_reply(MMI_BOOL is_cycle_positon,U8 is_ok)
{
    set_gps_location_data(is_ok);
    if(is_ok == 2)
        is_cycle_positon = MMI_TRUE;
    DBGI("g_location_data.status = %d \n",g_location_data.status);
    if((g_location_data.status == 'A')&&(g_last_location_point_param.timestamp == 0))
    {
        g_last_location_point_param.timestamp = g_location_data.gps_location.timestamp;
        g_last_location_point_param.latitude = g_location_data.gps_location.latitude;
        g_last_location_point_param.longitude = g_location_data.gps_location.longitude;
        
    }

    g_callback_send_location_data.cycle_location_flag = is_cycle_positon;
    g_callback_send_location_data.delay_flag = 0;
    g_callback_send_location_data.location_type = g_current_location_type;
    g_callback_send_location_data.other_location_type = g_other_position_type;

    g_callback_send_location_data.mode = g_current_mode_status;
    g_callback_send_location_data.pedometer = g_total_pedeometer_count;//100;/// test  
    g_callback_send_location_data.gps_data.latitude = g_location_data.gps_location.latitude;
    g_callback_send_location_data.gps_data.longitude = g_location_data.gps_location.longitude;
    g_callback_send_location_data.gps_data.altitude = g_location_data.gps_location.altitude;
    g_callback_send_location_data.gps_data.speed = g_location_data.gps_location.speed;
    g_callback_send_location_data.gps_data.bearing = g_location_data.gps_location.bearing;
    g_callback_send_location_data.gps_data.accuracy = g_location_data.gps_location.accuracy;

    g_callback_send_location_data.gps_data.sv_no = g_record_valible_index_1;
    g_callback_send_location_data.gps_data.timestamp_cur = g_location_data.gps_location.timestamp;
    g_callback_send_location_data.gps_data.status = g_location_data.status;

    g_callback_send_location_data.gps_data.timestamp_last = g_last_location_point_param.timestamp;
    g_callback_send_location_data.gps_data.latitude_last = g_last_location_point_param.latitude;
    g_callback_send_location_data.gps_data.longitude_last = g_last_location_point_param.longitude;
    DBGI(" -1- g_other_position_type =%d \n",g_other_position_type);
    if(g_other_position_type & position_type_power_on)
           eqc_algorithm_param_init();
    g_other_position_type = 0;
    if(g_location_data.status == 'A')
    {
        g_last_location_point_param.timestamp = g_location_data.gps_location.timestamp;
        g_last_location_point_param.latitude = g_location_data.gps_location.latitude;
        g_last_location_point_param.longitude = g_location_data.gps_location.longitude;    
    }
    DBGI("utc_time=%lld,lat=%f,lon=%f,status=%d \n",g_callback_send_location_data.gps_data.timestamp_cur,g_callback_send_location_data.gps_data.latitude,g_callback_send_location_data.gps_data.longitude,g_callback_send_location_data.gps_data.status);
    if(g_callback_send_location_data.other_location_type != 0)
    {
	client_callback_backup.location(&g_callback_send_location_data);
        memset(&g_callback_send_location_data,0,sizeof(g_callback_send_location_data));
    }
}


void client_set_work_mode(location_work_mode_struct * work_mode)
{
	eqc_algorithm_set_work_mode(work_mode->mode,(work_mode->gap_time*60));
}

void algorithm_make_location_once(position_type_enum*type)
{
        g_other_position_type = g_other_position_type | *type;
        DBGI("g_other_position_type =%d \n",(int)g_other_position_type);
        mmi_start_position_other();
}

void algorithm_make_clear_pedometer(void)
{

}

void algorithm_get_pedometer(void)
{

}

/**********nmea parse************/
typedef struct
{
	U8		NoMsg;					//Number of Messages  Total number of GPGSV messages being output
	U8		MsgNo;					//Message Number  Number of this message
	U16		NoSv;					//Satellites in View
}GSVStruct;
GSVStruct GSVData;

typedef struct
{
	char	UTC_Time[20];			//Current time	hhmmss.sss
	char	Valid_status;			//V = Data Invalid / Receiver Warning, A=Data Valid 
	char	Latitude[20];			//User datum latitude degrees, minutes, decimal minutes format
	char	NS_indicator;			//N/S Indicator  Hemisphere N=north or S=south
	char	Longitude[20];			//User datum latitude degrees, minutes, decimal minutes format
	char	EW_indicator;			//E/W indicator  'E'= East, or 'W' = West
	char	Spd[20];					//Speed  Speed Over Ground
	char	cog[20];					//COG  Course Over Ground
	char	Date[7];				//Current Date in Day, Month Year format ddmmyy
	char	mv[20];						//Magnetic variation value  Not being output by receiver degrees
	char	mvE;					//Magnetic variation E/W indicator  Not being output by receiver
	char	mode;
}RMCStruct;
RMCStruct RMCData;

typedef struct
{
	U16		sv;						//Satellite ID  SV ID (GPS: 1-32, SBAS 33-64 (33=PRN120))
	U16		elv;					//Elevation  Maximum 90
	U16		az;						//Azimuth  Range 0 to 359
	U16		cno;					//C/No  Range 0 to 99, null when not tracking
}SATELLITESTRUCT;
SATELLITESTRUCT Satellite_status[16];

typedef struct
{
	char	UTC_Time[20];			//Current time	hhmmss.sss
	char	Latitude[20];			//Degrees + minutes dddmm.mmmm
	char	NS_indicator;			//N/S Indicator  N=north or S=south
	char	Longitude[20];			//Degrees + minutes dddmm.mmmm
	char	EW_indicator;			//E/W indicator  E=east or W=west
	U8		FS; 					//Position Fix Indicator 0=No fix/Invalid 1=Standard GPS(2D/3D)  2=Differential GPS  6=Estimated (DR) Fix 
	U8		NoSV;					//Satellites_Used Range 0 to 12
	char	HDOP[20];					//Horizontal Dilution of Precision
	char	msl[20];					//MSL Altitude 
	char	Unit1;					//Units  Meters (fixed field)
	char	Altref[20];					//Geoid Separation
	char	Unit2;					//Units  Meters (fixed field)
	char	DiffAge[20];				//unit=s Age of Differential Corrections  Blank (Null) fields when DGPS is not used  
	char	DiffStation[20];			//Diff. Reference Station ID 
}GGAStruct;
GGAStruct	GGAData;


U8 tims_of_discard_mnea = 0 ;//cyp 20170701

typedef struct
{
	char Smode;	 //M=Manual - forced to operate in 2D or 3D mode   A=allowed to automatically switch 2D/3D
	U8 FS;		 //1=Fix not available  2=2D Fix  3=3D Fix
	U16 Sv[12];	 //Satellites Used  SV on Channel n If less than 12 SVs are used for navigation, the remaining fields are left empty. If more than 12 SVs are used for navigation, only the IDs of the first 12 are being output.
			 //The SV Numbers (Fields 'Sv') are in the range of 1 to 32 for GPS satellites, and 33 to 64 for SBAS satellites (33 = SBAS PRN 120, 34 = SBAS PRN 121 and so on)
	char PDOP[20];	 //Position Dilution of Precision (00.0 to 99.99)
	char HDOP[20];	 //Horizontal Dilution of Precision (00.0 to 99.99)
	char VDOP[20];	 //Vertical Dilution of Precision (00.0 to 99.99)
 }GSAStruct;
GSAStruct GSAData;


char g_nmea_buf[1024];


static double
str2float(char*  p)
{
    int   result = 0;
    int   len    = strlen(p);
    char  temp[16];

    if (len >= (int)sizeof(temp))
        return 0;
    if(len <= 0)
        return 0;

    memcpy( temp, p, len );
    temp[len] = 0;
    return strtod( temp, NULL );
}

static int
str2int( const char*  p, const char*  end )
{
    int   result = 0;
    int   len    = end - p;
    int   sign = 1;

    if (*p == '-')
    {
        sign = -1;
        p++;
        len = end - p;
    }

    for ( ; len > 0; len--, p++ )
    {
        int  c;

        if (p >= end)
            goto Fail;

        c = *p - '0';
        if ((unsigned)c >= 10)
            goto Fail;

        result = result*10 + c;
    }
    DBGI("result =%d \n",result);
    return  sign*result;

Fail:
    return -1;
}

void algorithm_set_gps_speed(void)
{
    gps_speed=(kal_int32)(str2float(RMCData.Spd)*1.852);
    DBGI("gps_speed =%d \n",gps_speed);
}
static int nmea_reader_update_time()
{
    int        hour, minute;
    double     seconds;
    int    day, mon, year;
    struct tm  tm;
    struct tm  tm_local;
    time_t     fix_time;

    int len;



    len = strlen(RMCData.Date);
    if(len != 6)
    {
          DBGE("UTC_Date error \n");
          return -1;
    }

    day  = str2int(RMCData.Date, RMCData.Date+2);
    mon  = str2int(RMCData.Date+2, RMCData.Date+4);
    year = str2int(RMCData.Date+4, RMCData.Date+6) + 2000;

    if ((day|mon|year) < 0) {
        DBGE("date not properly formatted: '%s'", RMCData.Date);
        return -1;
    }

    memset((void*)&tm, 0x00, sizeof(tm));
    if (year < 0) {
        // no date yet, get current one
        time_t  now = time(NULL);
        gmtime_r( &now, &tm );
        year = tm.tm_year + 1900;
        mon  = tm.tm_mon + 1;
        day  = tm.tm_mday;
    }

    len = strlen(RMCData.UTC_Time);
    if(len < 6)
    {
          DBGE("UTC_Timer error =%s \n",RMCData.UTC_Time);
          return -1;
    }



    hour    = str2int(RMCData.UTC_Time,RMCData.UTC_Time+2);
    minute  = str2int(RMCData.UTC_Time+2, RMCData.UTC_Time+4);
    seconds = str2float(RMCData.UTC_Time+4);

    tm.tm_hour = hour;
    tm.tm_min  = minute;
    tm.tm_sec  = (int) seconds;
    tm.tm_year = year - 1900;
    tm.tm_mon  = mon - 1;
    tm.tm_mday = day;
    tm.tm_isdst = -1;

    if (mktime(&tm) == (time_t)-1)
        //DBGE("mktime error: %d %s\n", errno, strerror(errno));
        DBGE("mktime error !\n");
    fix_time = mktime(&tm);
    localtime_r( &fix_time, &tm_local );

    fix_time += tm_local.tm_gmtoff;
    DBGI("fix_time: %d\n",fix_time);
    g_location_data.gps_location.timestamp = (int64_t)fix_time * 1000;
    DBGI("timestamp: %lld\n",g_location_data.gps_location.timestamp);
    return 0;
}


static double
convert_from_hhmm(char* degree,int mark)
{
    double  val     = str2float(degree);
    int     degrees = (int)(floor(val) / 100);
    double  minutes = val - degrees*100.;
    double  dcoord  = degrees + minutes / 60.0;
    if(mark == 0)
    {
        dcoord = -dcoord;
        
    }
    return -dcoord;
}

void set_gps_location_data(U8 is_ok)
{
    float pdop = 0;
    memset(&g_location_data,0,sizeof(GpsLocationData));
    g_location_data.status = 'V';
    pdop = str2float(GSAData.PDOP);
    if(RMCData.Valid_status == 'A' && GSAData.FS == 3 && (pdop<=1.8) && (is_ok != 2))
    {
    g_location_data.status = RMCData.Valid_status;	
    if(RMCData.NS_indicator == 'S')
        g_location_data.gps_location.latitude = convert_from_hhmm(RMCData.Latitude,1);
    else 
        g_location_data.gps_location.latitude = convert_from_hhmm(RMCData.Latitude,0);
    if(RMCData.EW_indicator == 'W')
        g_location_data.gps_location.longitude = convert_from_hhmm(RMCData.Longitude,1);
    else
        g_location_data.gps_location.longitude = convert_from_hhmm(RMCData.Longitude,0);

    g_location_data.gps_location.altitude = str2float(GGAData.msl);//0;//GGA

    g_location_data.gps_location.speed = str2float(RMCData.Spd);

    g_location_data.gps_location.bearing = str2float(RMCData.cog);
    g_location_data.gps_location.accuracy = 0;

    nmea_reader_update_time();
    }
}

U8 nmea_rmc_status_is_A(void)
{
	if(RMCData.Valid_status == 'A')
	{
		return 1;
	}
	
	return 0;
}

char get_nmea_rmc_status()
{
    float pdop = 0,altitude = 0;
    DBGI("RMCData.Valid_status=%d ,GSAData.FS=%d,GSAData.PDOP=%s \n",RMCData.Valid_status,GSAData.FS,GSAData.PDOP);
    pdop = str2float(GSAData.PDOP);
    altitude = str2float(GGAData.msl);
#if 1  //cyp 20170701
    if((RMCData.Valid_status == 'A') && (GSAData.FS == 3) && (pdop<=1.8) && (altitude > -50) && tims_of_discard_mnea >3 && g_open_gps_time != 3)
#else	
    if((RMCData.Valid_status == 'A') && (GSAData.FS == 3) && (pdop<=1.8) && (altitude > -50) )
#endif		
    //if((RMCData.Valid_status == 'A') && (GSAData.FS == 3) && (pdop<=1.8))
	{
	  //tims_of_discard_mnea = 0;//cyp  20170701
         return 'A';//RMCData.Valid_status;
	}
    else 
         return 'V';
}

U32 Get_NMEA_Form(S8* source,S8* result,U32 len)
{
    U32    i;
    if(*source=='*'||*source=='$')        //with no message head,*=checksum,$=start all is end
        return 0;
    for(i=1;i<=len;i++)
    {
        if(*source==',')
        {
            *result=0;
            return    i;
        }
        if(*source=='*')
        {
            *result=0;
            return    i;
        }
        *result=*source;
        result++;
        source++;
    }
    return    0;
}

U32 GGA_decode(S8* buff,U32 len)
{
    U32 temp_len=0;
    S8 temp[20];
    //head
    len=len-7;
    buff=buff+7;
    
    //UTC Time
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            strncpy(GGAData.UTC_Time,temp,temp_len);
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }
    }
    
    //Latitude
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            if(temp_len>2)
            {
                strncpy(GGAData.Latitude,temp,temp_len);
            }
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }
    }
    //NS_indicator
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            GGAData.NS_indicator=temp[0];
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }
    }
    //Longitude
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            if(temp_len>2)
            {
                strncpy(GGAData.Longitude,temp,temp_len);
            }
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }
    }
    //EW_indicator
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            GGAData.EW_indicator=temp[0];
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }
    }
    //FS
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            GGAData.FS=atoi(temp);
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }
    }
    //NoSV
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            GGAData.NoSV=atoi(temp);
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }
    }
    //HDOP                    //Horizontal Dilution of Precision
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            strncpy(GGAData.HDOP,temp,temp_len);
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }
    }
    //msl;                    //MSL Altitude 
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {        
            strncpy(GGAData.msl,temp,temp_len);
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }
    }
    //char    Unit1;                    //Units  Meters (fixed field)
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            GGAData.Unit1=temp[0];
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }
    }
    //Altref;                    //Geoid Separation
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            strncpy(GGAData.Altref,temp,temp_len);
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }
    }
    //Uint2;                    //Units  Meters (fixed field)
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            GGAData.Unit2=temp[0];
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }
    }
    if(GGAData.FS==2)
    {
        //DiffAge;                //unit=s Age of Differential Corrections  Blank (Null) fields when DGPS is not used
        if(len>0)
        {
            temp_len=Get_NMEA_Form(buff,temp,len);
            if(temp_len!=0)
            {
                strncpy(GGAData.DiffAge,temp,temp_len);
                len=len-temp_len;
                buff=buff+temp_len;
            }
            else
            {
                return 0;
            }
        }
    }
    else
    {
    }
    //DiffStation;            //Diff. Reference Station ID
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            strncpy(GGAData.DiffStation,temp,temp_len);
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }
    }
    
    return    1;
}

U32 GSV_decode(S8* buff,U32 len)
{
    U32 temp_len=0;
    S8    temp[20];
    U8    i=0;
    U8    start=0;
    U8    loop;


    len=len-7;
    buff=buff+7;

    //NoMsg;                    //Number of Messages  Total number of GPGSV messages being output
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            GSVData.NoMsg=atoi(temp);
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {        
            return 0;
        }
    }
    //MsgNo;                    //Message Number  Number of this message
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            GSVData.MsgNo=atoi(temp);

            if(GSVData.MsgNo == 1) 
            {
                 if(g_is_need_clear_gsv_record)
                 gps_search_satellite_signal_param_clear();
            }
                  
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            gps_search_satellite_signal_param_clear();
            return 0;
        }
    }
    //NoSv;                    //Satellites in View
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            GSVData.NoSv=atoi(temp);
          
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {        
            return 0;
        }
    }
    start=(GSVData.MsgNo-1)*4;
    
    if(GSVData.NoSv/4>(GSVData.NoMsg-1))
        loop=4;	
    else
    {
        loop=GSVData.NoSv-start;
    }
    for(i=0;i<loop;i++)
    {
        //sv;                        //Satellite ID  SV ID (GPS: 1-32, SBAS 33-64 (33=PRN120))
        if(len>0)
        {
            temp_len=Get_NMEA_Form(buff,temp,len);
            if(temp_len!=0)
            {
                (Satellite_status[start]).sv=atoi(temp);
                len=len-temp_len;
                buff=buff+temp_len;
            }
            else
            {
           
                return 0;
            }
        }
        //elv;                    //Elevation  Maximum 90
        if(len>0)
        {
            temp_len=Get_NMEA_Form(buff,temp,len);
            if(temp_len!=0)
            {
                (Satellite_status[start]).elv=atoi(temp);
                len=len-temp_len;
                buff=buff+temp_len;
            }
            else
            {
                return 0;
            }        
        }
        //az;                        //Azimuth  Range 0 to 359
        if(len>0)
        {
            temp_len=Get_NMEA_Form(buff,temp,len);
            if(temp_len!=0)
            {
                (Satellite_status[start]).az=atoi(temp);
                len=len-temp_len;
                buff=buff+temp_len;
            }
            else
            {            
                return 0;
            }        
        }
        //cno;                    //C/No  Range 0 to 99, null when not tracking
        if(len>0)
        {
            temp_len=Get_NMEA_Form(buff,temp,len);
            if(temp_len!=0)
            {
                (Satellite_status[start]).cno=atoi(temp);
                len=len-temp_len;
                buff=buff+temp_len;
                gps_location_record_satellite_cno((Satellite_status[start]).cno,(Satellite_status[start]).sv);
            }
            else
            {            
                return 0;
            }        
        }
        start++;
    } 

    return    1;        
}

/*
U32 GSV_decode(S8* buff,U32 len)
{
    U32 temp_len=0;
    S8    temp[20];
    U8    i=0;
    U8    start=0;
    U8    loop;
    len=len-7;
    buff=buff+7;

    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            GSVData.NoMsg=atoi(temp);
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
        
            return 0;
        }
    }
    //MsgNo;                    //Message Number  Number of this message
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            GSVData.MsgNo=atoi(temp);

            if(GSVData.MsgNo == 1)  //CLEAR
            {
                gps_search_satellite_signal_param_clear();
            }
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
             //INVALID
            gps_search_satellite_signal_param_clear();
        
            return 0;
        }
    }    
    return    1;        
}*/

U32 RMC_decode(S8* buff,U32 len)
{
    U32 temp_len=0;
    S8    temp[20];
    //head
    len=len-7;
    buff=buff+7;

    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            strncpy(RMCData.UTC_Time,temp,temp_len);
            DBGI("RMCData.UTC_Time=%s \n",RMCData.UTC_Time);
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }        
    }
    //Valid_status;            //V = Data Invalid / Receiver Warning, A=Data Valid 
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            RMCData.Valid_status=temp[0];
            DBGI("RMCData.Valid_status=%d \n",RMCData.Valid_status);
            #if 1//yls 20170707
            if(RMCData.Valid_status == 'A')
            {
            	tims_of_discard_mnea++;  
            }
            else
            {
            	tims_of_discard_mnea = 0;  
            }
            #endif
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }        
    }
    //Latitude[20];            //User datum latitude degrees, minutes, decimal minutes format
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            if(temp_len>2)
            {
                strncpy(RMCData.Latitude,temp,temp_len);
                DBGI("RMCData.Latitude=%s \n",RMCData.Latitude);
            }
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }        
    }
    //NS_indicator;            //N/S Indicator  Hemisphere N=north or S=south
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            RMCData.NS_indicator=temp[0];
            DBGI("RMCData.NS_indicator=%d \n",RMCData.NS_indicator);
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }        
    }
    //Longitude[20];            //User datum latitude degrees, minutes, decimal minutes format
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            if(temp_len>2)
            {
                strncpy(RMCData.Longitude,temp,temp_len);
                DBGI("RMCData.Longitude=%s \n",RMCData.Longitude);
            }
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }        
    }
    //EW_indicator;            //E/W indicator  'E'= East, or 'W' = West
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            RMCData.EW_indicator=temp[0];
            DBGI("RMCData.EW_indicator=%d \n",RMCData.EW_indicator);
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }        
    }
    //Spd;                    //Speed  Speed Over Ground
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            strncpy(RMCData.Spd,temp,temp_len);
            DBGI("RMCData.Spd=%s \n",RMCData.Spd);
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }        
    }
    //cog;                    //COG  Course Over Ground
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            strncpy(RMCData.cog,temp,temp_len);
            DBGI("RMCData.cog=%s \n",RMCData.cog);
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }        
    }
    //char    Date[6];                //Current Date in Day, Month Year format ddmmyy
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            strncpy(RMCData.Date,temp,temp_len);
            DBGI("RMCData.Date=%s \n",RMCData.Date);
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }        
    }
    //mv;                        //Magnetic variation value  Not being output by receiver degrees
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            strncpy(RMCData.mv,temp,temp_len);
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }        
    }
    //mvE;                    //Magnetic variation E/W indicator  Not being output by receiver
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            RMCData.mvE=temp[0];
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }        
    }
    //mode;
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            RMCData.mode=temp[0];
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }        
    }
    return 1;
}

U32 GSA_decode(S8* buff,U32 len)
{
    U32 temp_len=0;
    S8    temp[20];
    U8    i=0;

    //head
    len=len-7;
    buff=buff+7;

    //Smode;                    //M=Manual - forced to operate in 2D or 3D mode   A=allowed to automatically switch 2D/3D
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            GSAData.Smode=temp[0];
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }
    }

    //FS;                        //1=Fix not available  2=2D Fix  3=3D Fix
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            GSAData.FS=atoi(temp);
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }
    }
    //Sv[12];                    //Satellites Used  SV on Channel n If less than 12 SVs are used for navigation, the remaining fields are left empty. If more than 12 SVs are used for navigation, only the IDs of the first 12 are being output.
                                //The SV Numbers (Fields 'Sv') are in the range of 1 to 32 for GPS satellites, and 33 to 64 for SBAS satellites (33 = SBAS PRN 120, 34 = SBAS PRN 121 and so on)
    for(i=0;i<12;i++)
    {
        if(len>0)
        {
            temp_len=Get_NMEA_Form(buff,temp,len);
            if(temp_len!=0)
            {
                GSAData.Sv[i]=atoi(temp);
                len=len-temp_len;
                buff=buff+temp_len;
            }
            else
            {
                return 0;
            }
        }
    }
    //PDOP;                    //Position Dilution of Precision (00.0 to 99.99)
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            strncpy(GSAData.PDOP,temp,temp_len);
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }
    }
    //HDOP;                    //Horizontal Dilution of Precision (00.0 to 99.99)
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            strncpy(GSAData.HDOP,temp,temp_len);
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }
    }
    //VDOP;                    //Vertical Dilution of Precision (00.0 to 99.99)
    if(len>0)
    {
        temp_len=Get_NMEA_Form(buff,temp,len);
        if(temp_len!=0)
        {
            strncpy(GSAData.VDOP,temp,temp_len);
            len=len-temp_len;
            buff=buff+temp_len;
        }
        else
        {
            return 0;
        }
    }
    return 1;
}


void algorithm_nmea_parse_hdlr(void)
{
    int len = 0,length = 0;
    char *nmea = NULL,*end = NULL;

    len = strlen(g_nmea_buf);

    if(len < 0)
    {
         return; 
    }
    nmea = g_nmea_buf;
    end = g_nmea_buf+len;
    DBGI(" nmea[0]=%c,end[-1]=%d \n",nmea[0],end[-1]);
    // the initial '$' is optional
    //if (nmea < end && nmea[0] == '$')
       // nmea += 1;

    DBGI(" end[-1]=%d-%d-%d ,nmea=%s \n",end[-1],'\n','\r',nmea);

    // remove trailing newline
    if (end > nmea && end[-1] == '\n') {
        end -= 1;
        if (end > nmea && end[-1] == '\r')
            end -= 1;
    }      
    
    if((strncmp(nmea,"$GPGSV,",7)==0)
       ||(strncmp(nmea,"$GNGSV,",7)==0)
       ||(strncmp(nmea,"$BDGSV,",7)==0)
       ||(strncmp(nmea,"$GLGSV,",7)==0)
       ||(strncmp(nmea,"$GBGSV,",7)==0)
      )
    {
         GSV_decode(nmea,(end-nmea));
    }
    else if((strncmp(nmea,"$GPRMC,",7)==0)
	 ||(strncmp(nmea,"$GNRMC,",7)==0)
	 ||(strncmp(nmea,"$BDRMC,",7)==0)
	 ||(strncmp(nmea,"$GLRMC,",7)==0)
	 ||(strncmp(nmea,"$GBRMC,",7)==0)
      )
    {
        g_is_need_clear_gsv_record = 1;
	RMC_decode(nmea,(end-nmea));
    }
    else if((strncmp(nmea,"$GPGGA,",7)==0)
	 ||(strncmp(nmea,"$GNGGA,",7)==0)
	 ||(strncmp(nmea,"$BDGGA,",7)==0)
	 ||(strncmp(nmea,"$GLGGA,",7)==0)
	 ||(strncmp(nmea,"$GBGGA,",7)==0)
      )
    {
	GGA_decode(nmea,(end-nmea));
    }
    else if((strncmp(nmea,"$GPGSA,",7)==0)
	 ||(strncmp(nmea,"$GNGSA,",7)==0)
	 ||(strncmp(nmea,"$BDGSA,",7)==0)
	 ||(strncmp(nmea,"$GLGSA,",7)==0)
	 ||(strncmp(nmea,"$GBGSA,",7)==0)
      )
    {
	GSA_decode(nmea,(end-nmea));
    }

}

void algorithm_get_nmea_data(const char*nmea)
{
    //pthread_t thread;
    
    DBGI(" nmea=%s \n",nmea);
    memset(g_nmea_buf,0,sizeof(g_nmea_buf)); 
    strcpy(g_nmea_buf,nmea);
    algorithm_nmea_parse_hdlr();
    //ret= pthread_create(&thread,NULL,algorithm_nmea_parse_hdlr,NULL);

}

/*******nmea parse end************/

void gps_nmea_param_make_clear()
{
    memset(&RMCData,0,sizeof(RMCStruct));
    memset(&GSVData,0,sizeof(GSVStruct));
    memset(&Satellite_status[0],0,sizeof(SATELLITESTRUCT)*16);
    #if 1//yls 20170707
    tims_of_discard_mnea = 0;  
    #endif
}

static int algorithm_client_init(algorithm_data_callbacks * callbacks)
{
	int ret = 0,fd;

    DBGI("CLIENT INIT \n");
	client_callback_backup = *callbacks;
    //eqc_algorithm_param_init();
	return 0;
}

static const AlgorithmInterface  AlgorithmClientInterface = {
    sizeof(AlgorithmInterface),
    algorithm_client_init,
    client_set_work_mode,
    algorithm_make_location_once,
    algorithm_make_clear_pedometer,
    algorithm_get_pedometer,
    algorithm_get_attach_cellinfo,
    algorithm_get_nmea_data,
    algorithm_set_gsensor_eint,
    algorithm_set_fence_status,
    algorithm_set_airplane_mode,
    algorithm_set_network,
};

/*
const AlgorithmInterface* get_algorithm_interface(void)
{
    return &AlgorithmClientInterface;
}
*/

const AlgorithmInterface* gps__get_algorithm_interface(struct algorithm_client_device_t* dev)
{
    return &AlgorithmClientInterface;
}


static int open_algorithm_client(const struct hw_module_t* module, char const* name,
        struct hw_device_t** device)
{
	 
    struct algorithm_client_device_t *dev = malloc(sizeof(struct algorithm_client_device_t));
    memset(dev, 0, sizeof(*dev));

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t*)module;

    dev->get_algorithm_interface = gps__get_algorithm_interface;

    *device = (struct hw_device_t*)dev;

    return 0;
}


static struct hw_module_methods_t algorithm_client_module_methods = {
    .open = open_algorithm_client
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = ALGORIHTM_CLIENT_HARDWARE_MODULE_ID,
    .name = "Hardware Algorithm client Module",
    .author = "The Algorithm client Source Project",
    .methods = &algorithm_client_module_methods,
};

