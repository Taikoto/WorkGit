#ifndef __EQC_ALGORITHM_CLIENT_H__
#define __EQC_ALGORITHM_CLIENT_H__
#include <stdio.h>
#include <stdlib.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <hardware/hardware.h>
#include <hardware/gps.h>

#define ALGORIHTM_CLIENT_HARDWARE_MODULE_ID "eqc_algorithm"

#define SAL_CELL_NBR_MAX 6
#define MAX_WIFI_LIST_COUNT	6
#define WIFI_MAC_ADDRESS_LEN	6

typedef unsigned char kal_uint8; 
typedef uint16_t GpsStatusValue;
typedef int64_t GpsUtcTime;
typedef unsigned char U8;
typedef char          S8;
typedef short S16;
typedef unsigned short U16 ;
typedef unsigned int U32;
typedef unsigned short kal_uint16;
typedef unsigned int kal_uint32;
typedef int kal_int32;

typedef enum
{
	MMI_FALSE = 0,
	MMI_TRUE,	
}MMI_BOOL;

typedef enum
{
	position_type_none = 0,
	position_type_power_on = 1,
	//position_type_power_off = 2,
	position_type_cycle = 2,
	position_type_active = 4,
	position_type_SOS = 8,
	position_type_location_info = 16,
	position_type_DS = 32,
	position_type_ss = 64,
	position_type_shake = 128,
	position_type_antioff = 256,
	position_type_photo = 512,
	position_type_get_lat_lon = 1024,
	position_type_weather = 2048,
	position_type_all
}position_type_enum;

typedef enum
{
	LOCATION_WORK_MODE_NONE = 0,
	LOCATION_WORK_MODE_SMART,
	LOCATION_WORK_MODE_HAND,
	LOCATION_WORK_ALL
}LOCATION_WORK_MODE;

typedef struct
{
	LOCATION_WORK_MODE mode;
	int gap_time;
}location_work_mode_struct;


typedef enum
{
	CMD_NONE_SET = 0,
        CMD_WORK_MODE_SET,
        CMD_CLEAR_PEDOMETER,
        CMD_GET_ATTACH_CELLINFO,
	CMD_ALL
}SetCmd;

typedef struct
{
    double          latitude;
    double          longitude;
    double          altitude;
    float           speed;
    float           bearing;		
    float           accuracy;
    int             sv_no;	
    GpsUtcTime      timestamp_cur;
    GpsStatusValue  status;
    GpsUtcTime      timestamp_last;
    double          latitude_last;
    double          longitude_last;
}gps_location_data_struct;

typedef enum
{
	LOCATION_BY_NONE = 0,
	LOCATION_BY_GPS,
	LOCATION_BY_WIFI,
	LOCATION_BY_LBS,
	LOCATION_BY_ALL
}LOCATION_TYPE;


/*cell info structure*/
typedef struct{
    kal_uint16 arfcn;           /*ARFCN*/
    kal_uint8 bsic;              /*BSIC*/
    kal_uint8 rxlev;            /*Received signal level*/
    kal_uint16 mcc;            /*MCC*/
    kal_uint16 mnc;            /*MNC*/
    kal_uint16 lac;              /*LAC*/
    long cell_id;                /*CI*/
}cell_info_struct;

typedef struct{
    cell_info_struct cur_cell_info;
    cell_info_struct nbr_cell_info[SAL_CELL_NBR_MAX];
    int nbr_cell_num ;
}lbs_location_data_struct;

typedef struct
{
    kal_uint8 bssid[ WIFI_MAC_ADDRESS_LEN ]; /* MAC address */
    kal_int32 rssi;                           /* receive signal, in dBm */
}wifi_info_struct;

typedef struct
{
    kal_uint8 num;
    wifi_info_struct ap_list[MAX_WIFI_LIST_COUNT];
}wifi_location_data_struct;


typedef struct
{
	LOCATION_TYPE location_type;
	kal_uint8 cycle_location_flag;
        position_type_enum other_location_type;
	kal_uint8 delay_flag;
	kal_uint8 mode;
	long pedometer;
	gps_location_data_struct gps_data;
 	//lbs_location_data_struct lbs_data;
	//wifi_location_data_struct wifi_data;

}LocationData;

typedef struct
{
	kal_uint8 is_cycle_upload;
	kal_uint8 is_other_upload;
	kal_uint8 is_location;
	kal_uint8 is_unknow_open;
}gps_close_param;


typedef void (* algorithm_location_recv)(LocationData *location_data);

typedef void (*algorithm_pedomter_reply )(kal_int32* steps);

typedef void (*algorithm_get_attach_request )(void * arg);

typedef void (*algorithm_open_gps)(int*is_cycle_open);

typedef void (*algorithm_close_gps)(gps_close_param*param);

typedef void (*algorithm_reset_next_cycle)(int*sec);

typedef void (*algorithm_change_network_set)(int*network);

typedef void (*algorithm_change_airplane_mode)(int*enable);

typedef struct
{
	size_t          size;
	algorithm_location_recv location;
	algorithm_pedomter_reply pedeomter;
	algorithm_get_attach_request attach_request;
	algorithm_open_gps open_gps;
	algorithm_close_gps close_gps;
        algorithm_reset_next_cycle reset_next_cycle;
        algorithm_change_network_set set_network;
        algorithm_change_airplane_mode set_airplane_mode;    
}algorithm_data_callbacks;

typedef struct {
	/** set to sizeof(GpsInterface) */
	size_t          size;
	/**
	 * Opens the interface and provides the callback routines
	 * to the implementation of this interface.
	 */
	int   (*init)( algorithm_data_callbacks * callbacks);

	void (*set_work_mode)(location_work_mode_struct * work_mode);

	void (*location_once)(position_type_enum* type);	

	void (*make_clear_pedometer)(void);

	void (*get_pedometer)(void);

	void (*reply_attach_cellinfo)(cell_info_struct*cell_info);

        void (*set_nmea_data)(const char*nmea);

        void (*set_gsensor_eint)(float* x,float* y,float* z,int*eint_count);

        void (*set_fence_status)(int*type,int* status);

        void (*set_airplane_mode)(int* enable);

        void (*set_network)(int* network);
} AlgorithmInterface;


struct algorithm_client_device_t {
    struct hw_device_t common;

    const AlgorithmInterface* (*get_algorithm_interface)(struct algorithm_client_device_t* dev);
};


#endif
