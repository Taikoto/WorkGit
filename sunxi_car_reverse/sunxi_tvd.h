/*
 * Fast car reverse head file
 *
 * Copyright (C) 2015-2018 AllwinnerTech, Inc.
 *
 * Contacts:
 * Zeng.Yajian <zengyajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */


#ifndef __sunxi_tvd_h__
#define __sunxi_tvd_h__

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>

#include <media/videobuf2-core.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-ctrls.h>

#include "../sunxi-tvd/tvd.h"

/* tvd interface */
#define CVBS_INTERFACE   0
#define YPBPRI_INTERFACE 1
#define YPBPRP_INTERFACE 2

/* output resolution */
#define NTSC             0
#define PAL              1
#ifdef CONFIG_VIDEO_SUNXI_TVD
extern int tvd_open_special(int tvd_fd);
extern int tvd_close_special(int tvd_fd);

extern int vidioc_s_fmt_vid_cap_special(int tvd_fd, struct v4l2_format *f);
extern int vidioc_g_fmt_vid_cap_special(int tvd_fd, struct v4l2_format *f);

extern int dqbuffer_special(int tvd_fd, struct tvd_buffer **buf);
extern int qbuffer_special(int tvd_fd, struct tvd_buffer *buf);

extern int vidioc_streamon_special(int tvd_fd, enum v4l2_buf_type i);
extern int vidioc_streamoff_special(int tvd_fd, enum v4l2_buf_type i);

extern void tvd_register_buffer_done_callback(void *func);
#else
int tvd_open_special(int tvd_fd)
{
	return 0;
}
int tvd_close_special(int tvd_fd)
{
	return 0;
}

int vidioc_s_fmt_vid_cap_special(int tvd_fd, struct v4l2_format *f)
{
	return 0;
}
int vidioc_g_fmt_vid_cap_special(int tvd_fd, struct v4l2_format *f)
{
	return 0;
}

int dqbuffer_special(int tvd_fd, struct tvd_buffer **buf)
{
	return 0;
}
int qbuffer_special(int tvd_fd, struct tvd_buffer *buf)
{
	return 0;
}

int vidioc_streamon_special(int tvd_fd, enum v4l2_buf_type i)
{
	return 0;
}
int vidioc_streamoff_special(int tvd_fd, enum v4l2_buf_type i)
{
	return 0;
}

void tvd_register_buffer_done_callback(void *func)
{
	return ;
}

#endif
#ifdef CONFIG_VIDEO_SUNXI_VIN_SPECIAL
extern int vin_open_special(int tvd_fd);
extern int vin_close_special(int tvd_fd);

extern int vin_s_input_special(int id, int i);
extern int vin_s_fmt_special(int tvd_fd, struct v4l2_format *f);
extern int vin_g_fmt_special(int tvd_fd, struct v4l2_format *f);

extern int vin_dqbuffer_special(int tvd_fd, struct vin_buffer **buf);
extern int vin_qbuffer_special(int tvd_fd, struct vin_buffer *buf);

extern int vin_streamon_special(int tvd_fd, enum v4l2_buf_type i);
extern int vin_streamoff_special(int tvd_fd, enum v4l2_buf_type i);

extern void vin_register_buffer_done_callback(int tvd_fd, void *func);
#endif


#if 0
/* tvd interface */
#define CVBS_INTERFACE   0
#define YPBPRI_INTERFACE 1
#define YPBPRP_INTERFACE 2

/* output resolution */
#define NTSC             0
#define PAL              1

typedef enum {
	TVD_PL_YUV420 = 0,
	TVD_MB_YUV420 = 1,
	TVD_PL_YUV422 = 2,
} TVD_FMT_T;

struct tvd_fmt {
	u8              name[32];
	u32           	fourcc;          /* v4l2 format id */
	TVD_FMT_T    	output_fmt;
	int   	        depth;
	u32           	width;
	u32           	height;
	enum v4l2_field field;
	enum v4l2_mbus_pixelcode    bus_pix_code;
	unsigned char   planes_cnt;
};

struct tvd_buffer {
	struct vb2_buffer vb;
	struct list_head list;
	struct tvd_fmt *fmt;
	enum vb2_buffer_state state;
	void *paddr;
	void *vaddr;
};

int tvd_open_special(int tvd_fd);
int tvd_close_special(int tvd_fd);

int vidioc_s_fmt_vid_cap_special(int tvd_fd, struct v4l2_format *f);
int vidioc_g_fmt_vid_cap_special(int tvd_fd, struct v4l2_format *f);

int dqbuffer_special(int tvd_fd, struct tvd_buffer **buf);
int qbuffer_special(int tvd_fd, struct tvd_buffer *buf);

#if 1
int vidioc_streamon_special(int tvd_fd, enum v4l2_buf_type i);
int vidioc_streamoff_special(int tvd_fd, enum v4l2_buf_type i);
#else
int vidioc_streamon_special(int tvd_fd, int flag);
int vidioc_streamoff_special(int tvd_fd, int flag);
#endif

void virtual_tvd_register_buffer_done_callback(void *func);
#endif

#endif
