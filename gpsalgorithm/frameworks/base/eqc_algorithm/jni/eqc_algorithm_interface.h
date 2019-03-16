#ifndef __EQC_ALGORITHM_INTERFACE_H__
#define __EQC_ALGORITHM_INTERFACE_H__
#include <stdio.h>
#include <stdlib.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

typedef unsigned char kal_uint8; 
typedef uint16_t GpsStatusValue;
typedef int64_t GpsUtcTime;

typedef struct
{
    double          latitude;
    double          longitude;
    double          altitude;
    float           speed;
    float           accuracy;
    GpsUtcTime      timestamp;
    GpsStatusValue  status;
}gps_location_data_struct;

typedef struct
{
    gps_location_data_struct gps_data;
    kal_uint8 mode; 
}LocationData;

static void eqc_algorithm_postion_data_reply(LocationData* data);
#endif
