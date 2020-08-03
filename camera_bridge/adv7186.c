/*
 * Copyright (C) 2012-2015 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2018 NXP
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/v4l2-mediabus.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>
#include <media/tp2825_config.h>

#define CHIP_IDENT			"adv7186"

struct camdev_datafmt {
	u32	code;
	enum v4l2_colorspace		colorspace;
};

struct camdev {
	struct v4l2_subdev		subdev;
	struct i2c_client *i2c_client;
	struct v4l2_pix_format pix;
	const struct camdev_datafmt	*fmt;
	struct v4l2_captureparm streamcap;
	struct media_pad pad;
	bool on;
	bool mipi_csi;

	/* control settings */
	int brightness;
	int hue;
	int contrast;
	int saturation;
	int red;
	int green;
	int blue;
	int ae_mode;

	u32 mclk;
	u8 mclk_source;
	struct clk *sensor_clk;
	int csi;
	v4l2_std_id std_id;

	void (*io_init)(void);
};

/*!
 * Maintains the information on the current state of the sesor.
 */
static struct camdev camdev_data;
static int rst_gpio;

//0: dvr
//1: rearview
//2: right view
static int g_rear_flag = 1;


/*! List of input video formats supported. The video formats is corresponding
 * with v4l2 id in video_fmt_t
 */
typedef enum {
	SDV_NTSC = 0,	/*!< Locked on (M) NTSC video signal. */
	SDV_PAL,		/*!< (B, G, H, I, N)PAL video signal. */
	SDV_NOT_LOCKED,	/*!< Not locked on a signal. */
} video_fmt_idx;


/*! Video format structure. */
typedef struct {
	int v4l2_id;		/*!< Video for linux ID. */
	char name[16];		/*!< Name (e.g., "NTSC", "PAL", etc.) */
	u16 raw_width;		/*!< Raw width. */
	u16 raw_height;		/*!< Raw height. */
	u16 active_width;	/*!< Active width. */
	u16 active_height;	/*!< Active height. */
	int frame_rate;		/*!< Frame rate. */
} video_fmt_t;

/*! Description of video formats supported.
 *
 *  PAL: raw=720x625, active=720x576.
 *  NTSC: raw=720x525, active=720x480.
 */
static video_fmt_t video_fmts[] = {
	{			/*! NTSC */
	 .v4l2_id = V4L2_STD_NTSC,	 	
	 .name = "NTSC",
	 
#if TP2825_720X480
	 .raw_width = 720,	/* SENS_FRM_WIDTH */
	 .raw_height = 525,	/* SENS_FRM_HEIGHT */
	 .active_width = 720,	/* ACT_FRM_WIDTH plus 1 */
	 .active_height = 480,	/* ACT_FRM_WIDTH plus 1 */
	 .frame_rate = 30,	 
#endif

#if (TP2825_1280X720 || TP2825_1280X720_V1)
	.raw_width = 1280,//1980,
	.raw_height = 720,//720,
	.active_width = 1280,
	.active_height = 720,
#endif

#if TP2825_960X480
	.raw_width = 960,
	.raw_height = 480,
	.active_width = 960,
	.active_height = 480,
#endif

	
#if TP2825_50HZ
	.frame_rate = 50,
#endif

#if TP2825_25HZ	
	.frame_rate = 25,
#endif

#if (TP2825_30HZ )
	.frame_rate = 30,
#endif

#if (TP2825_60HZ )
	.frame_rate = 60,
#endif
	 },
	 
	{			/*! (B, G, H, I, N) PAL */
	 .v4l2_id = V4L2_STD_PAL,
	 .name = "PAL", 
#if TP2825_720X480	 
	 .raw_width = 720,
	 .raw_height = 625,
	 .active_width = 720,
	 .active_height = 576,
	 .frame_rate = 25,
#endif

#if (TP2825_1280X720 || TP2825_1280X720_V1)
	.raw_width = 1280,//1980,
	.raw_height = 720,//720,
	.active_width = 1280,
	.active_height = 720,
#endif

#if TP2825_960X480
	.raw_width = 960,
	.raw_height = 480,
	.active_width = 960,
	.active_height = 480,
#endif
	
#if TP2825_50HZ
	.frame_rate = 50,
#endif

#if TP2825_25HZ	
	.frame_rate = 25,
#endif

#if (TP2825_30HZ )
	.frame_rate = 30,
#endif

#if (TP2825_60HZ )
	.frame_rate = 60,
#endif

	 },
	{			/*! Unlocked standard */
#if 0	
	 .v4l2_id = V4L2_STD_ALL,
	 .name = "Autodetect",
	 .raw_width = 720,
	 .raw_height = 525,
	 .active_width = 720,
	 .active_height = 480,
	 .frame_rate = 30,
#else//lqc add test

	.v4l2_id = V4L2_STD_ALL,
	.name = "Autodetect",
	.raw_width = 1280,
	.raw_height = 720,
	.active_width = 1280,
	.active_height = 720,
	.frame_rate = 30,
#endif

	 },
};

/*!* Standard index. */
static video_fmt_idx video_idx = SDV_NTSC;


static int camdev_probe(struct i2c_client *adapter,
				const struct i2c_device_id *device_id);
static int camdev_remove(struct i2c_client *client);

static const struct i2c_device_id camdev_id[] = {
	{CHIP_IDENT, 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, camdev_id);

static struct i2c_driver camdev_i2c_driver = {
	.driver = {
		  .owner = THIS_MODULE,
		  .name  = CHIP_IDENT,
		  },
	.probe  = camdev_probe,
	.remove = camdev_remove,
	.id_table = camdev_id,
};

static int camdev_set_clk_rate(void)
{
	u32 tgt_xclk;	/* target xclk */
	int ret;	/* mclk */

	tgt_xclk = 27000000;
	camdev_data.mclk = tgt_xclk;

	pr_debug("   Setting mclk to %d MHz\n", tgt_xclk / 1000000);

	ret = clk_set_rate(camdev_data.sensor_clk, camdev_data.mclk);
	if (ret < 0)
		pr_debug("set rate filed, rate=%d\n", camdev_data.mclk);
	return ret;
}

static struct camdev *to_camdev(const struct i2c_client *client)
{
	return container_of(i2c_get_clientdata(client), struct camdev, subdev);
}

static inline void camdev_reset(void)
{
	gpio_set_value_cansleep(rst_gpio, 0);
	msleep(50);
	gpio_set_value_cansleep(rst_gpio, 1);
	msleep(50);
}

static int i2c_write_reg(u8 reg, u8 val)
{
	s32 ret;

	ret = i2c_smbus_write_byte_data(camdev_data.i2c_client, reg, val);
	if (ret < 0) {
		pr_err("%s:write reg error:reg=0x%2x,val=0x%2x\n", __func__,
			reg, val);
		return -1;
	}
	return 0;
}

static inline int i2c_read(u8 reg)
{
	int val;

	val = i2c_smbus_read_byte_data(camdev_data.i2c_client, reg);
	if (val < 0) {
		pr_err("%s:read reg error: reg=x%2x\n", __func__, reg);
		return -1;
	}
	return val;
}

static int camdev_g_std(struct v4l2_subdev *sd, v4l2_std_id *norm)
{
	*norm = (video_idx == SDV_PAL) ? V4L2_STD_PAL : V4L2_STD_NTSC;
	if (*norm != camdev_data.std_id) 
	{
		camdev_data.std_id = *norm;
		camdev_data.pix.width = video_fmts[video_idx].raw_width;
		camdev_data.pix.height = video_fmts[video_idx].raw_height;		
	}
	//*norm = V4L2_STD_ALL;
	
	return 0;
}

static int camdev_link_setup(struct media_entity *entity,
			   const struct media_pad *local,
			   const struct media_pad *remote, u32 flags)
{
	return 0;
}

static int camdev_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *a)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camdev *sensor = to_camdev(client);
	struct v4l2_captureparm *cparm = &a->parm.capture;
	int ret = 0;

	switch (a->type) {
	/* This is the only case currently handled. */
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
		memset(a, 0, sizeof(*a));
		a->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		cparm->capability = sensor->streamcap.capability;
		cparm->timeperframe = sensor->streamcap.timeperframe;
		cparm->capturemode = sensor->streamcap.capturemode;
		ret = 0;
		break;

	/* These are all the possible cases. */
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	case V4L2_BUF_TYPE_VBI_CAPTURE:
	case V4L2_BUF_TYPE_VBI_OUTPUT:
	case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
	case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
		ret = -EINVAL;
		break;

	default:
		pr_debug("   type is unknown - %d\n", a->type);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int camdev_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *a)
{
	return 0;
}

static int camdev_s_stream(struct v4l2_subdev *sd, int enable)
{
	return 0;
}

static int camdev_chip_ident(struct v4l2_subdev *sd, struct v4l2_dbg_chip_ident *chip)
{
//	((struct v4l2_dbg_chip_ident *)id)->match.type = V4L2_CHIP_MATCH_SUBDEV;//V4L2_CHIP_MATCH_I2C_DRIVER;

	strcpy(chip->match.name, CHIP_IDENT);

	return 0;
}

static int camdev_s_power(struct v4l2_subdev *sd, int on)
{
	return 0;
}

static int camdev_enum_framesizes(struct v4l2_subdev *sd,
			       struct v4l2_subdev_pad_config *cfg,
			       struct v4l2_subdev_frame_size_enum *fse)
{
	if (fse->index >= 1)
		return -EINVAL;

	fse->max_width = fse->min_width = video_fmts[video_idx].active_width;
	fse->max_height = fse->min_height = video_fmts[video_idx].active_height;

	return 0;
}

static int camdev_set_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *format)
{
	return 0;
}

static int camdev_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *format)
{
	struct v4l2_mbus_framefmt *mf = &format->format;

	if (format->pad)
		return -EINVAL;

	memset(mf, 0, sizeof(struct v4l2_mbus_framefmt));

	mf->code = MEDIA_BUS_FMT_VYUY8_2X8;
	//mf->width = 720;
	
#if TP2825_1280X720	
	mf->width = 1280;
	mf->field = V4L2_FIELD_SEQ_TB;//V4L2_FIELD_NONE;//V4L2_FIELD_SEQ_TB;
	mf->colorspace = V4L2_COLORSPACE_SMPTE240M;//V4L2_COLORSPACE_SMPTE170M;//V4L2_COLORSPACE_SMPTE240M;//V4L2_COLORSPACE_RAW;
	//mf->colorspace = V4L2_COLORSPACE_SMPTE170M;//V4L2_COLORSPACE_SMPTE170M;//V4L2_COLORSPACE_SMPTE240M;//V4L2_COLORSPACE_RAW;
#endif

#if TP2825_960X480
	mf->width = 960;
	mf->field = V4L2_FIELD_INTERLACED;
	mf->colorspace = V4L2_COLORSPACE_SMPTE170M;
#endif
	
#if TP2825_720X480
	mf->width = 720;
	mf->field = V4L2_FIELD_INTERLACED;
	mf->colorspace = V4L2_COLORSPACE_SMPTE170M;
#endif

	mf->height = video_fmts[video_idx].active_height;
	printk("camdev_get_fmt mf->width=%d mf->field=%d\n",mf->width,mf->field);
	return 0;
}

static const struct v4l2_subdev_video_ops camdev_video_ops = {
	.g_std = camdev_g_std,
	.g_parm = camdev_g_parm,
	.s_parm = camdev_s_parm,
	.s_stream = camdev_s_stream,
};

static const struct v4l2_subdev_core_ops camdev_core_ops = {
	.g_chip_ident = camdev_chip_ident,
	.s_power = camdev_s_power,
};

static const struct v4l2_subdev_pad_ops camdev_pad_ops = {
	.enum_frame_size = camdev_enum_framesizes,
	.set_fmt = camdev_set_fmt,
	.get_fmt = camdev_get_fmt,
};

static const struct v4l2_subdev_ops camdev_ops = {
	.core = &camdev_core_ops,
	.video = &camdev_video_ops,
	.pad = &camdev_pad_ops,
};

static const struct media_entity_operations camdev_sd_media_ops = {
	.link_setup = camdev_link_setup,
};

#if TP2825_1280X720_V1

#endif


#if TP2825_960X480 ////960X480,60hz,30fps,ok

#endif



#if TP2825_1280X720


#if TP2825_50HZ 
#endif


#if (TP2825_25HZ || TP2825_30HZ) //1280X720,30hz,30fps or 25hz 25fps,hu
		
	#if TP2825_25HZ
	#else
	#endif
	
		
	#if TP2825_25HZ	
	#else
	#endif
	
#endif

#endif

#if TP2825_720X480 //tw9990

#endif

static const char *vin_type[] =
{
	"DVR",
	"REARVIEW",
	"BLINDZONE"
};

static ssize_t set_vin_type(int t, ssize_t ret)
{
	u8 blindBuf[3] = {0};

	pr_info("camdev: vin switch to %d (%s)\n", t, vin_type[t]);
	
	switch(t)
	{
		case 0:

			camdev_data.i2c_client->addr =0x4C>>1;
			blindBuf[0]=0x02;
			blindBuf[1]=0x84;
			blindBuf[2]=0x40;
			
			if (i2c_master_send(camdev_data.i2c_client, blindBuf, 3) < 0) 
			{
				pr_err("camdev writeerro, %s,%d\n",__FUNCTION__,__LINE__);
				return -EIO;
			}

			mdelay(500);

			break;

		case 1:
		
			camdev_data.i2c_client->addr =0x4C>>1;
			blindBuf[0]=0x02;
			blindBuf[1]=0x85;
			blindBuf[2]=0x50;
			
			if (i2c_master_send(camdev_data.i2c_client, blindBuf, 3) < 0) 
			{
				pr_err("camdev writeerro, %s,%d\n",__FUNCTION__,__LINE__);
				return -EIO;
			}
		
			mdelay(600);
		
			break;

		case 2:
		
			camdev_data.i2c_client->addr =0x4C>>1;
			blindBuf[0]=0x02;
			blindBuf[1]=0x83;
			blindBuf[2]=0x30;
			
			if (i2c_master_send(camdev_data.i2c_client, blindBuf, 3) < 0) 
			{
				pr_err("camdev writeerro, %s,%d\n",__FUNCTION__,__LINE__);
				return -EIO;
			}
		
			mdelay(500);
		
			break;
	}

	g_rear_flag = t;

	return ret;
}

static int init_device(void)
{
	int i=0; int rev=0;
	u8 au8Buf[30] = {0};

	pr_info("%s: %s hard_reset\n", __func__, CHIP_IDENT);
#if 0
	if (cvbs) {
		/* Set CVBS input on AIN1 */
		;//i2c_write_reg(ADV7180_INPUT_CTL, 0x00);
	} else {
		/*
		 * Set YPbPr input on AIN1,4,5 and normal
		 * operations(autodection of all stds).
		 */
		;//i2c_write_reg(ADV7180_INPUT_CTL, 0x09);
	}
#endif
	i2c_write_reg(0xff, 0x80);
	mdelay(10);
	/* Datasheet recommends */
	
	
	au8Buf[0]=0xf1; 
	au8Buf[1]=0x90;
	au8Buf[2]=0x94;
	if (i2c_master_send(camdev_data.i2c_client, au8Buf, 3) < 0) {
		pr_err("write***************88err************1\n"); 		
		//return -1;
	}
	//mdelay(100);
		
	
	mdelay(1);
	i2c_write_reg(0xf8, 0x4C);
	mdelay(1);
	au8Buf[0]=0xfd; 
	au8Buf[1]=0x44;
	au8Buf[2]=0x48;
	if (i2c_master_send(camdev_data.i2c_client, au8Buf, 3) < 0) {
		pr_err("write***************88err************2\n"); 		
//		return -1;
	}
	rev = i2c_read(0xfd);
	printk("adv7186_readfd=====%x\n",rev);
	mdelay(1);
	au8Buf[0]=0xe9; 
	//au8Buf[1]=0xe0;//zhubin
	au8Buf[1]=0x88;//lqc
	au8Buf[2]=0xa4;
	if (i2c_master_send(camdev_data.i2c_client, au8Buf, 3) < 0) {
		pr_err("write***************88err************3\n"); 		
//		return -1;
	}
	rev = i2c_read(0xe9);
	printk("adv7186_reade9=====%x\n",rev);
	mdelay(1);
	i2c_write_reg(0xec, 0xa0);
	i2c_write_reg(0xeb, 0xa8);
	i2c_write_reg(0xfb, 0x84);//zhubin

	mdelay(1);
	camdev_data.i2c_client->addr =0xA0>>1;
	au8Buf[0]=0x00;
	au8Buf[1]=0x01;
	au8Buf[2]=0x20;//00
	if (i2c_master_send(camdev_data.i2c_client, au8Buf, 3) < 0) {
		pr_err("***************88err************4\n");
			
	//		return -1;
	}
	rev = i2c_read(0x00);
	printk("adv7186_read00=====%x\n",rev);
	mdelay(1);
	i2c_write_reg(0x03, 0x40);
	i2c_write_reg(0x05, 0x28);
	camdev_data.i2c_client->addr =0xC0>>1; 
	i2c_write_reg(0x0c, 0x40);
	i2c_write_reg(0x15, 0x80);
	i2c_write_reg(0x19, 0x80);
	i2c_write_reg(0x33, 0x40);
	camdev_data.i2c_client->addr =0x4C>>1; 
	i2c_write_reg(0x00, 0x00);
	mdelay(1);

#if 0
	au8Buf[0]=0x02;  /* mean ain5*/
	au8Buf[1]=0x85;
	au8Buf[2]=0x50;
	
	g_rear_flag = 1; //deault rear;
	if (i2c_master_send(camdev_data.i2c_client, au8Buf, 3) < 0) {
	pr_err("***************88err************6\n");
				
	//		return -1;
	}
#else
	set_vin_type(g_rear_flag, 0);
#endif
	
	rev = i2c_read(0x02);
	printk("adv7186_read02=====%x\n",rev);
	i2c_write_reg(0x05, 0x00);
	i2c_write_reg(0x0f, 0x80);
	i2c_write_reg(0xdb, 0x80);
	camdev_data.i2c_client->addr =0xA4>>1; 
	mdelay(1);
	au8Buf[0]=0x00;
	au8Buf[1]=0x9e;
	au8Buf[2]=0xf1;
	if (i2c_master_send(camdev_data.i2c_client, au8Buf, 3) < 0) {
		pr_err("***************88err************7\n");
			
//		return -1;
	}
	rev = i2c_read(0x00);
	printk("adv7186_read00=====%x\n",rev);
	i2c_write_reg(0xb0, 0x04);
	i2c_write_reg(0xb2, 0x82);
	i2c_write_reg(0x6b, 0x0c);
	i2c_write_reg(0xa6, 0x00);
	mdelay(1);
	camdev_data.i2c_client->addr =0x90>>1;
	au8Buf[0]=0x00;
	au8Buf[1]=0x7f;
	au8Buf[2]=0x00;
	if (i2c_master_send(camdev_data.i2c_client, au8Buf, 3) < 0) {
		pr_err("***************88err************8\n");
			
//		return -1;
	}
	rev = i2c_read(0x00);
	printk("adv7186_read00=====%x\n",rev);
	mdelay(1);
	au8Buf[0]=0x03;
	au8Buf[1]=0xe4;
	au8Buf[2]=0x0b;
	au8Buf[3]=0xc3;
	au8Buf[4]=0xfe;
	if (i2c_master_send(camdev_data.i2c_client, au8Buf, 5) < 0) {
		pr_err("***************88err************9\n");
			
//		return -1;
	}
	rev = i2c_read(0x03);
	printk("adv7186_read03=====%x\n",rev);
	mdelay(1);
	i2c_write_reg(0x2B, 0x22);
	i2c_write_reg(0xA7, 0x00);
	i2c_write_reg(0xD4, 0x60);
	camdev_data.i2c_client->addr =0xC0>>1;
	i2c_write_reg(0x1E, 0x83);//0x81
	
	camdev_data.i2c_client->addr =0x84>>1;
	i2c_write_reg(0xA5, 0x00);
	
	camdev_data.i2c_client->addr =0x90>>1;
	i2c_write_reg(0x12, 0x00);//0x04
	camdev_data.i2c_client->addr =0xA4>>1;
	i2c_write_reg(0xBA, 0x09);
	mdelay(1);
	camdev_data.i2c_client->addr =0x94>>1;
	au8Buf[0]=0x94;
	au8Buf[1]=0x00;
	au8Buf[2]=0x0c;
	if (i2c_master_send(camdev_data.i2c_client, au8Buf, 3) < 0) {
		pr_err("***************88err************10\n"); 	
//		return -1;
	}
	rev = i2c_read(0x94);
	printk("adv7186_read94=====%x\n",rev);
	//mdelay(100);
	i2c_write_reg(0x97, 0x00);
	mdelay(1);
	au8Buf[0]=0xa0;
	for(i=1;i<17;i++)
		au8Buf[i]=0x03; 	
	if (i2c_master_send(camdev_data.i2c_client, au8Buf, 15) < 0) {
		pr_err("***************88err************11\n"); 	
//		return -1;
	}
	mdelay(1);
		
	camdev_data.i2c_client->addr =0x84>>1; 
	au8Buf[0]=0x13;
	au8Buf[1]=0x07;
	au8Buf[2]=0xff;
	if (i2c_master_send(camdev_data.i2c_client, au8Buf, 3) < 0) {
		pr_err("***************88err************12\n"); 		
		return -1;
	}
	//adv7186_data.i2c_client->addr =0xA8>>1;
	//adv7186_write_reg(0x28, 0x80);
	camdev_data.i2c_client->addr =0xA4>>1;
	i2c_write_reg(0x10, 0xAA);
	mdelay(10);
	au8Buf[0]=0x6f;
	au8Buf[1]=0x04;
	au8Buf[2]=0x08;
	au8Buf[3]=0x00;
	au8Buf[4]=0x68;
	if (i2c_master_send(camdev_data.i2c_client, au8Buf, 5) < 0) {
		pr_err("***************88err************12\n");
			
//		return -1;
	}
	rev = i2c_read(0x6f);
	printk("adv7186_read6f=====%x\n",rev);
	i2c_write_reg(0x93, 0xB1);
	//mdelay(100);
#if 1
	 //camdev_data.i2c_client->addr =0xA4>>1;	 
	 //i2c_write_reg(0x60, 0x94);
	 //i2c_write_reg(0x61, 0xbf);
	 //i2c_write_reg(0x62, 0xf8);

	 //camdev_data.i2c_client->addr =0x90>>1;	
	 //i2c_write_reg(0x03, 0x44); 


	 camdev_data.i2c_client->addr =0xA4>>1;	 
	 i2c_write_reg(0x60, 0x73);
	 i2c_write_reg(0x61, 0x90);
	 i2c_write_reg(0x62, 0xff);

#else	
	 camdev_data.i2c_client->addr =0xA4>>1;
	 i2c_write_reg(0x01, 0xff);
	 i2c_write_reg(0x60, 0x73);
	 i2c_write_reg(0x61, 0x90);
	 i2c_write_reg(0x62, 0xff);
	 i2c_write_reg(0x6B, 0x00);
	 i2c_write_reg(0x70, 0x00);
	 i2c_write_reg(0x72, 0x08);
	 i2c_write_reg(0x74, 0x14);
	 i2c_write_reg(0x75, 0x03);
	 i2c_write_reg(0x76, 0x38);
	 i2c_write_reg(0x79, 0x03);
	 i2c_write_reg(0x80, 0x1E);
	 i2c_write_reg(0x81, 0x50);
	 i2c_write_reg(0x82, 0x05);
	 i2c_write_reg(0x93, 0x00);
	 i2c_write_reg(0xB0, 0x05);
	 i2c_write_reg(0xB3, 0x03);
												   
	 camdev_data.i2c_client->addr =0xA8>>1;
	 i2c_write_reg(0xFF, 0x00);
												   
	 camdev_data.i2c_client->addr =0x44>>1;
	 i2c_write_reg(0xB6, 0x20);
	 i2c_write_reg(0xE0, 0x03);
	 i2c_write_reg(0xE1, 0x49);
	 i2c_write_reg(0xE2, 0x40);
	 i2c_write_reg(0xE6, 0x5B);
	 i2c_write_reg(0xE7, 0x00);
	 i2c_write_reg(0xE8, 0x01);
	 i2c_write_reg(0xE9, 0x0C);
	 i2c_write_reg(0xEA, 0x23);
	 i2c_write_reg(0xED, 0x00);
	 i2c_write_reg(0xEE, 0x04);
	 i2c_write_reg(0xEF, 0x02);
	 i2c_write_reg(0xF0, 0x00);
	 i2c_write_reg(0xFF, 0x98);
												   
	 camdev_data.i2c_client->addr =0x48>>1;
	 i2c_write_reg(0x3D, 0x00);
	 i2c_write_reg(0x3D, 0x10);
	 i2c_write_reg(0x3F, 0x00);
	 i2c_write_reg(0x41, 0x00);
	 i2c_write_reg(0x42, 0x00);
	 i2c_write_reg(0x43, 0x80);
	 i2c_write_reg(0x44, 0x00);
	 i2c_write_reg(0x47, 0x00);
	 i2c_write_reg(0x48, 0x80);
	 i2c_write_reg(0x49, 0x40);
	 i2c_write_reg(0x4A, 0x80);
	 i2c_write_reg(0x4C, 0x01);
	 i2c_write_reg(0x4D, 0x24);
	 i2c_write_reg(0x4E, 0x00);
	 i2c_write_reg(0x4F, 0x01);
	 i2c_write_reg(0x51, 0x10);
	 i2c_write_reg(0x53, 0x00);
	 i2c_write_reg(0x55, 0x30);
	 i2c_write_reg(0x56, 0xA0);
	 i2c_write_reg(0x57, 0x00);
	 i2c_write_reg(0x59, 0x01);
	 i2c_write_reg(0x5A, 0x28);
	 i2c_write_reg(0x5B, 0x08);
	 i2c_write_reg(0x5C, 0x18);
	 i2c_write_reg(0x5D, 0xC6);
	 i2c_write_reg(0x5E, 0x24);
												   
	 camdev_data.i2c_client->addr =0xA0>>1;
	 i2c_write_reg(0x01, 0x00);
	 i2c_write_reg(0x05, 0x24);
												   
	 camdev_data.i2c_client->addr =0x90>>1;
	 //i2c_write_reg(0x03, 0xC4);
	 i2c_write_reg(0x04, 0xC0);
	 i2c_write_reg(0x05, 0xC4);
	 i2c_write_reg(0x0E, 0x3F);
	 i2c_write_reg(0x12, 0x05);
	 i2c_write_reg(0x22, 0x40);
	 i2c_write_reg(0x23, 0x40);
	 i2c_write_reg(0x25, 0x7F);
	 i2c_write_reg(0x26, 0xBF);
	 i2c_write_reg(0x4F, 0x00);
	 i2c_write_reg(0x54, 0x04);
	 i2c_write_reg(0x55, 0x88);
	 i2c_write_reg(0x7B, 0x6F);
	 i2c_write_reg(0xDD, 0xFC);
#endif
#if 0
	printk("\n*****ddr bist test***************************************************\n");
	//camdev_data.i2c_client->addr =0xC0>>1;	
	//i2c_write_reg(0xFF, 0x80);//Reset
	//i2c_write_reg(0xF1, 0x90);//Set SDP Map Address
	//i2c_write_reg(0xF2, 0x94);//Set SDP_IO Map Address
	//i2c_write_reg(0xF8, 0x4C);// Set AFE/DPLL Map Address
	//i2c_write_reg(0xFD, 0x44);// Set CP Map Address
	//i2c_write_reg(0xFE, 0x48);// Set VDP Map Address
	//i2c_write_reg(0xE9, 0x88);// Set VSP Map Address
	//i2c_write_reg(0xEA, 0xA4);// ; Set VPP Map Address
	//i2c_write_reg(0xEB, 0xA8);// ; Set XMEM_GAMMA Map Address 
	//i2c_write_reg(0xEC, 0xA0);// ; Set VFE Map Address
	
	camdev_data.i2c_client->addr =0xA8>>1; 
	i2c_write_reg(0x2B, 0x08) ; //Enable DDR2 Memory
	i2c_write_reg(0x14, 0x90) ; //ADI Recommended Write
	i2c_write_reg(0x27, 0x26) ; //Enable A12 Output

	camdev_data.i2c_client->addr =0xC0>>1; 
	i2c_write_reg(0x0C, 0x40) ; //Power up Core

	camdev_data.i2c_client->addr =0x4C>>1; 
	i2c_write_reg(0x00, 0x00) ; //Power up ADC

	camdev_data.i2c_client->addr =0xA0>>1; 
	i2c_write_reg(0x00, 0x00) ; //Vid STD
	i2c_write_reg(0x01, 0x00) ; //PRIM MODE - SDP core
	i2c_write_reg(0x0C, 0x00) ; //Power up VDP


	camdev_data.i2c_client->addr =0xA8>>1; 
	i2c_write_reg(0x26, 0x12) ; //ADI Recommended Write
	i2c_write_reg(0x23, 0x10) ; //ADI Recommended Write
	i2c_write_reg(0x24, 0x10) ; //ADI Recommended Write
	i2c_write_reg(0x25, 0x06) ; //ADI Recommended Write
	i2c_write_reg(0x05, 0x01) ; //ADI Recommended Write
	i2c_write_reg(0x06, 0x40) ; //ADI Recommended Write
	i2c_write_reg(0x07, 0x53) ; //ADI Recommended Write
	i2c_write_reg(0x08, 0x92) ; //ADI Recommended Write
	//i2c_write_reg(0x11, 0x22) ; //Memory size is 256 Mbit
	i2c_write_reg(0x11, 0x32) ; //Memory size is 1G Mbit
	i2c_write_reg(0x1E, 0x43) ; //ADI Recommended Write
	i2c_write_reg(0x1F, 0x02) ; //ADI Recommended Write
	i2c_write_reg(0x20, 0x80) ; //ADI Recommended Write
	i2c_write_reg(0x21, 0xC1) ; //ADI Recommended Write


	camdev_data.i2c_client->addr =0x94>>1; 
	i2c_write_reg(0x80, 0x80) ; //ADI Recommended Write
	i2c_write_reg(0x81, 0x03) ; //ADI Recommended Write
	i2c_write_reg(0x82, 0x8E) ; //ADI Recommended Write
	i2c_write_reg(0x83, 0x00) ; //ADI Recommended Write
	i2c_write_reg(0x84, 0x07) ; //ADI Recommended Write
	i2c_write_reg(0x85, 0x1C) ; //ADI Recommended Write

	camdev_data.i2c_client->addr =0x90>>1; 
	i2c_write_reg(0x12, 0x01) ; //Enable 3D comb

	camdev_data.i2c_client->addr =0x94>>1; 
	i2c_write_reg(0x7C, 0x18) ; //ADI Recommended Write

	camdev_data.i2c_client->addr =0xC0>>1; 
	i2c_write_reg(0xFF, 0x0C) ; //SDP and Memory reset


	camdev_data.i2c_client->addr =0x94>>1; 
	i2c_write_reg(0xD9, 0x00) ; //ADI Recommended Write
	i2c_write_reg(0xD9, 0x80) ; //ADI Recommended Write


	camdev_data.i2c_client->addr =0xA8>>1; 
	i2c_write_reg(0x28, 0xE5) ; //ADI Recommended Write

	camdev_data.i2c_client->addr =0x90>>1; 
	i2c_write_reg(0x12, 0x01) ; //Enable 3D comb

	camdev_data.i2c_client->addr =0xC0>>1; 
	i2c_write_reg(0xFF, 0x0C) ; //SDP and Memory reset

	//camdev_data.i2c_client->addr =0xA8>>1;	
	//i2c_write_reg(0x2B, 0x0A) ; //Bist enable

	//rev = i2c_read(0x34);
#endif




	return 0;
}

static ssize_t camdev_vin_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	pr_info("## query vin state... ");
	
	return scnprintf(buf, PAGE_SIZE, "%d (%s)\n", g_rear_flag, vin_type[g_rear_flag]);
}

static ssize_t camdev_vin_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	u8 i;
	ssize_t ret = (ssize_t)count;

	if((kstrtou8(buf, 0, &i) == 0) && (i < 3))
	{
		if(i == g_rear_flag)
		{
			pr_info("## camdev vin switch to the same channel, skip\n");
			goto exit0;
		}
	
		return set_vin_type(i, ret);
	}
	else
	{
		pr_err("## %s error, invalid input string para, use '0', '1' or '2'\n", __func__);
		ret = -EINVAL;
	}

exit0:
	return ret;
}

static DEVICE_ATTR(vin, S_IWUSR | S_IRUSR, camdev_vin_show,
		 camdev_vin_store);

static struct attribute *camdev_attrs[] = {
	&dev_attr_vin.attr,
	NULL
};

static const struct attribute_group camdev_attr_group = {
	.attrs = camdev_attrs
};

/*!
 * I2C probe function
 *
 * @param adapter            struct i2c_adapter *
 * @return  Error code indicating success or failure
 */
static int camdev_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct pinctrl *pinctrl;
	struct device *dev = &client->dev;
	struct v4l2_subdev *sd;
	const char *vmode;
	int retval;

	printk("%s_probe imx8\n\n", CHIP_IDENT);

	pinctrl = devm_pinctrl_get_select_default(dev);
	if (IS_ERR(pinctrl)) {
		dev_err(dev, "setup pinctrl failed\n");
		return PTR_ERR(pinctrl);
	}

	/* request power down pin */
	/* ---- no power down gpio on the board ---- */

	/* request reset pin */
	rst_gpio = of_get_named_gpio(dev->of_node, "rst-gpios", 0);
	if (!gpio_is_valid(rst_gpio)) {
		dev_err(dev, "no sensor reset pin available\n");
		return -EINVAL;
	}
	retval = devm_gpio_request_one(dev, rst_gpio, GPIOF_OUT_INIT_LOW,
					"camdev_reset");

	/* Set mclk rate before clk on */
	camdev_set_clk_rate();

	retval = clk_prepare_enable(camdev_data.sensor_clk);

	if (retval < 0) {
		dev_err(dev, "%s: enable sensor clk fail\n", __func__);
		return -EINVAL;
	}

	/* Set initial values for the sensor struct. */
	memset(&camdev_data, 0, sizeof(camdev_data));

	retval = of_property_read_u32(dev->of_node, "csi_id",
				&(camdev_data.csi));
	if (retval) {
		dev_err(dev, "csi_id invalid\n");
		clk_disable_unprepare(camdev_data.sensor_clk);
		return retval;
	}

	if(of_property_read_string(dev->of_node, "vmode", &vmode))
	{
		dev_err(dev, "camdev error: no vmode config available\n");
		clk_disable_unprepare(camdev_data.sensor_clk);
		return -ENODEV;
	}

	if(!strcmp(vmode, "ntsc"))
	{
		video_idx = SDV_NTSC;
	}
	else if(!strcmp(vmode, "pal"))
	{
		video_idx = SDV_PAL;
	}
	else
	{
		dev_err(dev, "camdev error: unknown vmode %s\n", vmode);
		clk_disable_unprepare(camdev_data.sensor_clk);
		return -ENODEV;		
	}

	pr_info("camdev: vmode is %s\n", (video_idx == SDV_NTSC) ? "NTSC" : "PAL");

	camdev_data.i2c_client = client;

	camdev_data.pix.pixelformat = V4L2_PIX_FMT_VYUY;  /* YUV422 */
	camdev_data.pix.width = video_fmts[video_idx].raw_width;
	camdev_data.pix.height = video_fmts[video_idx].raw_height;
	camdev_data.streamcap.capturemode = 0;

#if TP2825_50HZ	
	camdev_data.streamcap.timeperframe.denominator = 50;
#endif

#if TP2825_60HZ	
	camdev_data.streamcap.timeperframe.denominator = 60;
#endif

#if TP2825_30HZ	
	camdev_data.streamcap.timeperframe.denominator = 30;
#endif

#if TP2825_25HZ	
	camdev_data.streamcap.timeperframe.denominator = 25;
#endif

#if TP2825_720X480
	camdev_data.streamcap.timeperframe.denominator = (video_idx == SDV_NTSC) ? 30 : 25;
#endif

	camdev_data.streamcap.timeperframe.numerator = 1;
	camdev_reset();

	init_device();

	sd = &camdev_data.subdev;
	v4l2_i2c_subdev_init(sd, client, &camdev_ops);

	msleep(10);
	
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	sd->entity.function = MEDIA_ENT_F_CAM_SENSOR;
	camdev_data.pad.flags = MEDIA_PAD_FL_SOURCE;

	retval = media_entity_pads_init(&sd->entity, 1, &camdev_data.pad);
	sd->entity.ops = &camdev_sd_media_ops;

	retval = v4l2_async_register_subdev(sd);
	if (retval < 0) {
		dev_err(&client->dev,
					"%s--Async register failed, ret=%d\n", __func__, retval);
		media_entity_cleanup(&sd->entity);
		clk_disable_unprepare(camdev_data.sensor_clk);
		return -ENODEV;
	}
	msleep(10);

	retval = sysfs_create_group(&dev->kobj, &camdev_attr_group);
	if (retval) {
		dev_err(dev, "camdev error: Failure %d creating sysfs group\n", retval);
		media_entity_cleanup(&sd->entity);
		clk_disable_unprepare(camdev_data.sensor_clk);
		return retval;
	}

	clk_disable_unprepare(camdev_data.sensor_clk);
	
	pr_info("%s: SDV camera %s, is found\n", __func__, CHIP_IDENT);
	return 0;
}

/*!
 * I2C detach function
 *
 * @param client            struct i2c_client *
 * @return  Error code indicating success or failure
 */
static int camdev_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct device *dev = &client->dev;

	clk_disable_unprepare(camdev_data.sensor_clk);

	sysfs_remove_group(&dev->kobj, &camdev_attr_group);

	v4l2_async_unregister_subdev(sd);

	return 0;
}


module_i2c_driver(camdev_i2c_driver);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("CSI-SD Camera Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("3.0");
MODULE_ALIAS("CSI");
