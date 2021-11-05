/*
 * Flashlight Core
 *
 * Copyright (C) 2015 MediaTek Inc.
 *
 * Author: Simon Wang <Simon-TCH.Wang@mediatek.com>
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

#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/miscdevice.h>
#include <linux/module.h>

#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

#include "bridge_i2c.h"

kal_uint16 i2cId = 0x6c;

int bridge_debug_mask = 0;
module_param_named(bridge_debug_mask, bridge_debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);


static int bridge_i2c_open(struct inode *inode, struct file *file)
{
    return nonseekable_open(inode, file);
}

#define BRIDGE_DEBUG_USER  1
#if(defined (ONE_REG_TO_ONE_DATA))
static ssize_t bridge_i2c_read(struct file *file, char __user *buf, size_t count, loff_t *data)
{
    unsigned char mcu_comm_data[SENSOR_COMM_DATA_SIZE] = {0};
#ifdef BRIDGE_DEBUG_USER
	int i = 0;
#endif
	int ret = -1;

	BRIDGE_DEBUG("bridge_i2c_read count = %zu\n",count);
	if(count < 1 || count > 4) {
		BRIDGE_ERROR("read invalid data from user\n");
		return -EFAULT;
	}
	
    memset(mcu_comm_data,0,4);
    if (copy_from_user(mcu_comm_data, buf, count))
        return -EFAULT;

#ifdef BRIDGE_DEBUG_USER
    BRIDGE_DEBUG("user read ************ data is:\n");
    for(i = 0; i < count; i++)
        BRIDGE_DEBUG("CSD mcu_comm_data[%d]=%x,,",i,mcu_comm_data[i]);
    BRIDGE_DEBUG("\n");
#endif

	ret = bridge_i2c_read_sensor(mcu_comm_data,count);
	BRIDGE_DEBUG("bridge_i2c_read ret = %d\n",ret);
    if (copy_to_user(buf, &ret, count))
        return -EFAULT;

#ifdef BRIDGE_DEBUG_USER
    BRIDGE_DEBUG("CSD read ************ data is:\n");
    for(i = 0; i < count; i++)
        BRIDGE_DEBUG("CSD mcu_comm_data[%d]=%x,,",i,mcu_comm_data[i]);
    BRIDGE_DEBUG("\n");
#endif

    return count;
}
#else
static ssize_t bridge_i2c_read(struct file *file, char __user *buf, size_t count, loff_t *data)
{
    unsigned char mcu_comm_data[SENSOR_COMM_DATA_SIZE] = {0};
#ifdef BRIDGE_DEBUG_USER
	int i = 0;
#endif
	int ret = -1;
	kal_uint8 *get_byte = NULL;
	get_byte = kmalloc(count, GFP_KERNEL);
	if(!get_byte) {
		BRIDGE_ERROR("alloc failed !\n");
		kfree(get_byte);
		return -ENOMEM;
	}

	BRIDGE_DEBUG("bridge_i2c_read count = %zu\n",count);
	if(count < 1 || count > SENSOR_COMM_DATA_SIZE) {
		BRIDGE_ERROR("read invalid data from user\n");
		return -EFAULT;
	}

    memset(mcu_comm_data,0,SENSOR_COMM_DATA_SIZE);
    if (copy_from_user(mcu_comm_data, buf, count))
        return -EFAULT;

#ifdef BRIDGE_DEBUG_USER
    BRIDGE_DEBUG("user read ************ data is:\n");
    for(i = 0; i < 2; i++)
        BRIDGE_DEBUG("CSD mcu_comm_data[%d]=%x,,",i,mcu_comm_data[i]);
    BRIDGE_DEBUG("\n");
#endif

	ret = bridge_i2c_read_sensor(mcu_comm_data,count-2,get_byte,i2cId);//mcu_comm_data[2],get_byte[count-2]
	BRIDGE_DEBUG("bridge_i2c_read get_byte = 0x%x,%d,0x%x\n",get_byte[0],ret,i2cId);
    if (copy_to_user(buf, get_byte, count-2))
        return -EFAULT;

#ifdef BRIDGE_DEBUG_USER
    BRIDGE_DEBUG("CSD read ************ data is:\n");
    for(i = 0; i < count-2; i++)
        BRIDGE_DEBUG("CSD get_byte[%d]=%x,,",i,get_byte[i]);
    BRIDGE_DEBUG("\n");
#endif
	kfree(get_byte);

    return count;
}
#endif
/*
extern kal_uint16 bridge_i2c_read_sensor(kal_uint8 *pusendcmd, kal_uint16 size);
extern void bridge_i2c_write_sensor(kal_uint8 *pusendcmd, kal_uint16 size);
*/

static ssize_t bridge_i2c_write(struct file *file, const char __user *buf, size_t count, loff_t *data)
{
    unsigned char soc_send_msg[SENSOR_COMM_DATA_SIZE] = {0};

#ifdef BRIDGE_DEBUG_USER
    int i = 0;
#endif
	
	BRIDGE_DEBUG("bridge_i2c_write count = %zu\n",count);
	if(count < 1 || count > SENSOR_COMM_DATA_SIZE) {
		BRIDGE_ERROR("write invalid data from user\n");
		return -EFAULT;
	}
	
	//memset(soc_send_msg,0,4);
    if (copy_from_user(soc_send_msg, buf, count))
        return -EFAULT;
#ifdef BRIDGE_DEBUG_USER
    BRIDGE_DEBUG("user write ************ data is:\n");
    for(i = 0; i < count; i++)
	    BRIDGE_DEBUG("CSD soc_send_msg[%d]=%x,,",i,soc_send_msg[i]);
    BRIDGE_DEBUG("\n");
#endif
    bridge_i2c_write_sensor(soc_send_msg,count,i2cId);
	BRIDGE_DEBUG("bridge_i2c_write_sensor i2cId = 0x%x\n",i2cId);
#ifdef BRIDGE_DEBUG_USER
    BRIDGE_DEBUG("CSD write ************ data is:\n");
    for(i = 0; i < count; i++)
	    BRIDGE_DEBUG("CSD soc_send_msg[%d]=%x,,",i,soc_send_msg[i]);
    BRIDGE_DEBUG("\n");
#endif

    return count;
}

static int bridge_i2c_release(struct inode *inode, struct file *file)
{
    return 0;
}

static long bridge_i2c_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    long err = 0;
	int ret = 0;
    //void __user *argp = (void __user *)arg;
	ret = bridge_i2c_read_max9286(REVERSE_CONTROL_CHANNAL_EN);
	BRIDGE_DEBUG("default sensor value : 0x%x, cmd = %d\n",ret,cmd);
    switch (cmd) {
        case BRIDGE_I2C_CAMERA_ID_FRONT:
			i2cId = 0x62;
			//bridge_i2c_write_max9286(REVERSE_CONTROL_CHANNAL_EN , 0X01);
            BRIDGE_DEBUG("select front camera sensor\n");
        break;
        case BRIDGE_I2C_CAMERA_ID_REAR:
			i2cId = 0x64;
            BRIDGE_DEBUG("select rear camera sensor\n");
        break;
        case BRIDGE_I2C_CAMERA_ID_LEFT:
			i2cId = 0x66;
			//bridge_i2c_write_max9286(REVERSE_CONTROL_CHANNAL_EN , 0X04);
            BRIDGE_DEBUG("select left camera sensor\n");
        break;
        case BRIDGE_I2C_CAMERA_ID_RIGHT:
			i2cId = 0x68;
			//bridge_i2c_write_max9286(REVERSE_CONTROL_CHANNAL_EN , 0X08);
            BRIDGE_DEBUG("select right camera sensor\n");
        break;
        case BRIDGE_I2C_CAMERA_ID_RELEASE:
			i2cId = 0x6c;
			//bridge_i2c_write_max9286(REVERSE_CONTROL_CHANNAL_EN , 0X00);
            BRIDGE_DEBUG("release camera sensor\n");
        break;
        default:
            BRIDGE_ERROR("unknown IOCTL: 0x%08x\n", cmd);
            err = -ENOIOCTLCMD;
        break;
    }

	ret = bridge_i2c_read_max9286(REVERSE_CONTROL_CHANNAL_EN);
	BRIDGE_DEBUG("sensor value : 0x%x\n",ret);

    return err;
}
static const struct file_operations bridge_i2c_fops = {
    //.owner = THIS_MODULE,
    .open = bridge_i2c_open,
    .read = bridge_i2c_read,
    .write = bridge_i2c_write,
    .release = bridge_i2c_release,
    .unlocked_ioctl = bridge_i2c_unlocked_ioctl,
};

static struct miscdevice bridge_i2c_device = {
    .name = "bridge_i2c",
    .fops = &bridge_i2c_fops,
};
static int bridge_i2c_probe(void/*struct i2c_client *client, const struct i2c_device_id *id*/)
{
	int err = 0;
	BRIDGE_DEBUG("start\n");
    err = misc_register(&bridge_i2c_device);
    if (err) {
        BRIDGE_ERROR("misc device register failed, err = %d\n", err);
        goto exit_misc_register_failed;
    }

	BRIDGE_DEBUG("Exit done\n");
	return 0;
	
exit_misc_register_failed:
    misc_deregister(&bridge_i2c_device);
	return -1;	
}

static int __init bridge_i2c_init(void)
{
	bridge_i2c_probe();

	BRIDGE_DEBUG("Init done\n");

	return 0;
}

static void __exit bridge_i2c_exit(void)
{
	misc_deregister(&bridge_i2c_device);
	BRIDGE_DEBUG("Exit done\n");
}

module_init(bridge_i2c_init);
module_exit(bridge_i2c_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Taikoto");
MODULE_DESCRIPTION("bridge_i2c Driver");
