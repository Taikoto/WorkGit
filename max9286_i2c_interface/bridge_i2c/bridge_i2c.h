/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _BRIDGE_I2C_H
#define _BRIDGE_I2C_H

#include <linux/types.h>
#include <linux/ioctl.h>

typedef signed char kal_int8;
typedef signed short kal_int16;
typedef signed int kal_int32;
typedef long long kal_int64;
typedef unsigned char kal_uint8;
typedef unsigned short kal_uint16;
typedef unsigned int kal_uint32;
typedef unsigned long long kal_uint64;
typedef char kal_char;

#define SENSOR_COMM_DATA_SIZE 64
/* device node and sysfs */
#define BRIDGE_I2C_DEVNAME          "bridge_i2c"

#define BRIDGE     'T'
#define BRIDGE_I2C_CAMERA_ID_FRONT     _IO(BRIDGE,0)
#define BRIDGE_I2C_CAMERA_ID_REAR      _IO(BRIDGE,1)
#define BRIDGE_I2C_CAMERA_ID_LEFT      _IO(BRIDGE,2)
#define BRIDGE_I2C_CAMERA_ID_RIGHT     _IO(BRIDGE,3)
#define BRIDGE_I2C_CAMERA_ID_RELEASE   _IO(BRIDGE,4)

typedef enum {
	CAMERA_ID_FRONT = 0,
	CAMERA_ID_REAR,
	CAMERA_ID_LEFT,
	CAMERA_ID_RIGHT,
	RELEASE
} CameraId;

#define REVERSE_CONTROL_CHANNAL_EN  0X0A
extern int bridge_debug_mask;
#define BRIDGE_INFO(fmt,arg...)     do{\
                                       if(bridge_debug_mask)\
                                       printk("<<-BRIDGE-INFO->> "fmt"\n",##arg);\
                                    }while(0)

#define BRIDGE_ERROR(fmt,arg...)       printk("<<-BRIDGE-ERROR->> "fmt"\n",##arg)
#define BRIDGE_DEBUG(fmt,arg...)    do{\
                                       if(bridge_debug_mask)\
                                       printk("<<-BRIDGE-DEBUG->> [%d]"fmt"\n",__LINE__, ##arg);\
                                    }while(0)

extern unsigned int camera_sensor_vsync_det;
extern unsigned int Camera_Sensor_Signal_Detect(void);
#if(defined (ONE_REG_TO_ONE_DATA))
extern kal_uint16 bridge_i2c_read_sensor(kal_uint8 *pusendcmd, kal_uint16 size);
#else
extern kal_uint16 bridge_i2c_read_sensor(kal_uint8 *pusendcmd, kal_uint16 size, kal_uint8 *purecvval, kal_uint16 i2cId);
#endif
extern void bridge_i2c_write_sensor(kal_uint8 *pusendcmd, kal_uint16 size, kal_uint16 i2cId);
extern kal_uint16 bridge_i2c_read_max9286(kal_uint32 addr);
extern void bridge_i2c_write_max9286(kal_uint32 addr, kal_uint32 para);
//extern kal_uint16 bridge_i2c_read_sensor(kal_uint32 addr);
//extern void bridge_i2c_write_sensor(kal_uint32 addr, kal_uint32 para);

#endif /* _BRIDGE_I2C_H */

