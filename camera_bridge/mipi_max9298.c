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
#include <mipi_multiplexer.h>
#include <linux/semaphore.h>

/* mclk is not used??? */
#define CAMDEV_XCLK_MIN 6000000
#define CAMDEV_XCLK_MAX 24000000

#define CAMDEV_SENS_PAD_SOURCE		0
#define CAMDEV_SENS_PADS_NUM		1

enum camdev_mode {
	camdev_mode_MIN = 0,
	camdev_mode_NATIVE_RES = camdev_mode_MIN,
	camdev_mode_MAX = camdev_mode_NATIVE_RES
};

struct camdev_datafmt {
	u32	code;
	enum v4l2_colorspace		colorspace;
};

typedef struct camera_info
{
	char *id, *tx_chip;
	char ident[32]; /* 32 is kernel/.../video2dev.h v4l2_dbg_match.name[] config */
	size_t width, height, fps;
	size_t pix_fmt;
	size_t hs_setting;
	struct camdev_datafmt cs;
	const u8 *cfg_dat;
	int cfg_len;
}camera_info_t;

struct camdev {
	struct v4l2_subdev		subdev;
	struct i2c_client *i2c_client, *rclient0, *rclient1;
	struct v4l2_pix_format pix;
	const struct camdev_datafmt	*fmt;
	struct v4l2_captureparm streamcap;
	struct media_pad pads[CAMDEV_SENS_PADS_NUM];
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

	void (*io_init)(void);

	camera_info_t const *ci_a, *ci_b; // ci_a: ch a, reverse camera[default]. ci_b: ch b, recording camera
	int ci_cur; //0: reverse-camera(ch a), 1: recording camera(ch b)

	dtsi_t *dts;

	int debug; // debug state
};

/*!
 * Maintains the information on the current state of the sesor.
 */
static struct camdev camdev_data;
static struct semaphore vsem;
static struct mutex cmutx;


#if 0
static struct regulator *io_regulator;
static struct regulator *core_regulator;
static struct regulator *analog_regulator;
#endif

#define rx_write_reg(r, v, lb)		camdev_write_reg16(r, v, lb)
#define rx_read_reg(r, p, lb)		camdev_read_reg16(r, p, lb)

#define tx_write_reg(r, v, lb)		camdev1_write_reg(r, v, lb)
#define tx_read_reg(r, p, lb)		camdev1_read_reg(r, p, lb)

#define rx_raw_write_reg(r, v)		camdev_raw_write_reg16(r, v)
#define rx_raw_read_reg(r, p)		camdev_raw_read_reg16(r, p)

#define tx_raw_write_reg(r, v)		camdev1_raw_write_reg(r, v)
#define tx_raw_read_reg(r, p)		camdev1_raw_read_reg(r, p)

static inline camera_info_t const *to_camera_info(void)
{
	return camdev_data.ci_cur ? camdev_data.ci_b : camdev_data.ci_a;
}

static struct camdev *to_camdev(const struct i2c_client *client)
{
	return container_of(i2c_get_clientdata(client), struct camdev, subdev);
}

static inline void camdev_power_down(int enable)
{

}

static inline void camdev_cam_reset(void)
{
	int hs1_gpio = camdev_data.dts->hs1_gpio;
	int hs2_gpio = camdev_data.dts->hs2_gpio;

	if(-1 == hs1_gpio) return;
	if(-1 == hs2_gpio) return;

	pr_info("camdev: --- do camera hw reset ---\n");

	gpio_set_value_cansleep(hs1_gpio, 0);
	gpio_set_value_cansleep(hs2_gpio, 0);
	msleep(20);
	gpio_set_value_cansleep(hs1_gpio, 1);
	gpio_set_value_cansleep(hs2_gpio, 1);
	msleep(20);
}

static inline void camdev_max9298_reset(void)
{
	int rst_gpio = camdev_data.dts->reset_gpio;

	if(-1 == rst_gpio) return;

	pr_info("camdev: --- do max9298 hw reset ---\n");
	
	gpio_set_value_cansleep(rst_gpio, 0);
	msleep(10);
	
	gpio_set_value_cansleep(rst_gpio, 1);
	msleep(100); // GMSL2 needs 100ms to lock, GMSL1 10ms
}

static int camdev_regulator_enable(struct device *dev)
{
#if 0
	int ret = 0;

	io_regulator = devm_regulator_get(dev, "DOVDD");
	if (!IS_ERR(io_regulator)) {
		regulator_set_voltage(io_regulator,
				      OV5640_VOLTAGE_DIGITAL_IO,
				      OV5640_VOLTAGE_DIGITAL_IO);
		ret = regulator_enable(io_regulator);
		if (ret) {
			dev_err(dev, "set io voltage failed\n");
			return ret;
		} else {
			dev_dbg(dev, "set io voltage ok\n");
		}
	} else {
		io_regulator = NULL;
		dev_warn(dev, "cannot get io voltage\n");
	}

	core_regulator = devm_regulator_get(dev, "DVDD");
	if (!IS_ERR(core_regulator)) {
		regulator_set_voltage(core_regulator,
				      OV5640_VOLTAGE_DIGITAL_CORE,
				      OV5640_VOLTAGE_DIGITAL_CORE);
		ret = regulator_enable(core_regulator);
		if (ret) {
			dev_err(dev, "set core voltage failed\n");
			return ret;
		} else {
			dev_dbg(dev, "set core voltage ok\n");
		}
	} else {
		core_regulator = NULL;
		dev_warn(dev, "cannot get core voltage\n");
	}

	analog_regulator = devm_regulator_get(dev, "AVDD");
	if (!IS_ERR(analog_regulator)) {
		regulator_set_voltage(analog_regulator,
				      OV5640_VOLTAGE_ANALOG,
				      OV5640_VOLTAGE_ANALOG);
		ret = regulator_enable(analog_regulator);
		if (ret) {
			dev_err(dev, "set analog voltage failed\n");
			return ret;
		} else {
			dev_dbg(dev, "set analog voltage ok\n");
		}
	} else {
		analog_regulator = NULL;
		dev_warn(dev, "cannot get analog voltage\n");
	}

	return ret;
#else
	return 0;
#endif
}

static int camdev_set_clk_rate(void)
{
	u32 tgt_xclk;	/* target xclk */
	int ret;

	/* mclk */
	tgt_xclk = camdev_data.mclk;
	tgt_xclk = min(tgt_xclk, (u32)CAMDEV_XCLK_MAX);
	tgt_xclk = max(tgt_xclk, (u32)CAMDEV_XCLK_MIN);
	camdev_data.mclk = tgt_xclk;

	pr_debug("camdev:   Setting mclk to %d MHz\n", tgt_xclk / 1000000);
	ret = clk_set_rate(camdev_data.sensor_clk, camdev_data.mclk);
	if (ret < 0)
		pr_debug("camdev: set rate filed, rate=%d\n", camdev_data.mclk);
	return ret;
}

static int video_detect(struct i2c_client *client)
{
	u8 v;
	struct i2c_client *cli = camdev_data.rclient0;
	int ch;
	u16 rstat[] = {0x01dc, 0x01fc};

	ch = camdev_data.ci_cur;

	rx_read_reg(rstat[ch], &v, exit0);

	if(0 == (v & 0x01)) // video loss
	{
		pr_info("camdev[max9298] video loss, do video off -> on\n");
		rx_write_reg(0xf00, 0, exit0);
		mdelay(10);
		rx_write_reg(0xf00, 1u << ch, exit0);
		mdelay(100); // GMSL2 need 100ms to lock video, GMSL1 10ms

		rx_read_reg(rstat[ch], &v, exit0);

		if(0 == (v & 0x01)) // video still loss
		{
			pr_info("camdev[max9298] video still loss after video link off -> on\n");
			goto exit0;
		}
	}

	pr_info("camdev[max9298] video detected\n");

	return 0;

exit0:
	return -1;
}

static int init_device(int tx_reset, int rx_reset)
{
	int i, i0;
	u8 len, r, addr, v;
	u16 rr;
	const camera_info_t *ci = to_camera_info();
	const u8 *conf = ci->cfg_dat;
	struct i2c_client *cli = camdev_data.rclient0, *cli1 = camdev_data.rclient1;

	mutex_lock(&cmutx);

	if(tx_reset)
		camdev_cam_reset();

	if(rx_reset)
		camdev_max9298_reset();

	for(i = 0; i < ci->cfg_len;)
	{
		i0 = i;
		len = conf[i++];

		if(0x00 == len)
		{
			msleep(conf[i++]);
			continue;
		}

		addr = conf[i++] >> 1;
		
		if(0x40 == addr)
		{
			r = conf[i++];
			v = conf[i++];
			tx_write_reg(r, v, exit0);
		}
		else if(0x48 == addr)
		{
			rr = conf[i++] << 8;
			rr |= conf[i++];
			v = conf[i++];
			rx_write_reg(rr, v, exit0);
		}

		if(len ^ (i - i0 - 1))
		{
			pr_err("camdev: %s: index error [len = %d : i0 = %d : i = %d]!!!\n", __func__, len, i0, i);
			goto exit0;
		}
	}

	if(0 == camdev_data.debug)// debug control
		goto exit1;

	// dump regs
	for(i = 0; i < ci->cfg_len;)
	{
		i0 = i;
		len = conf[i++];

		if(0x00 == len)
		{
			i++;
			continue;
		}

		addr = conf[i++] >> 1;

		if(0x40 == addr)
		{
			r = conf[i++];
			v = conf[i++];
			//tx_write_reg(r, v, exit0);
			tx_read_reg(r, &v, exit0);
			pr_info("camdev[dump] 0x40 : 0x%02x : 0x%02x\n", r, v);
		}
		else if(0x48 == addr)
		{
			rr = conf[i++] << 8;
			rr |= conf[i++];
			v = conf[i++];
			//rx_write_reg(rr, v, exit0);
			rx_read_reg(rr, &v, exit0);
			pr_info("camdev[dump] 0x48 : 0x%04x : 0x%02x\n", rr, v);
		}

		if(len ^ (i - i0 - 1))
		{
			pr_err("camdev dump: %s: index error [len = %d : i0 = %d : i = %d]!!!\n", __func__, len, i0, i);
			goto exit0;
		}
	}

exit1:
	pr_info("camdev: max9298(CH %c)-%s(%s) mipi config success\n", 'A' + camdev_data.ci_cur, ci->tx_chip, ci->id);

	mutex_unlock(&cmutx);

	return 0;

exit0:
	mutex_unlock(&cmutx);
	return -1;
}

static int camdev_chip_ident(struct v4l2_subdev *sd, struct v4l2_dbg_chip_ident *chip)
{
	const camera_info_t *ci = to_camera_info();
	
//	((struct v4l2_dbg_chip_ident *)id)->match.type = V4L2_CHIP_MATCH_SUBDEV;//V4L2_CHIP_MATCH_I2C_DRIVER;

	strcpy(chip->match.name, ci->ident);

	return 0;
}

/*!
 * camdev_s_power - V4L2 sensor interface handler for VIDIOC_S_POWER ioctl
 * @s: pointer to standard V4L2 device structure
 * @on: indicates power mode (on or off)
 *
 * Turns the power on or off, depending on the value of on and returns the
 * appropriate error code.
 */
static int camdev_s_power(struct v4l2_subdev *sd, int on)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camdev *sensor = to_camdev(client);

	if(!on)
	{
		pr_info("camdev: release sema, process '%s', id %d\n", current->comm, current->pid);
		up(&vsem);
	}
	else
	{
		pr_info("camdev: request sema... process '%s', id %d\n", current->comm, current->pid);
		down(&vsem);
		pr_info("camdev: acquire sema, process '%s', id %d\n", current->comm, current->pid);

		if(-1 == video_detect(client))
		{
			init_device(0, 1);
		}
	}

	sensor->on = on;
	return 0;
}

/*!
 * camdev_g_parm - V4L2 sensor interface handler for VIDIOC_G_PARM ioctl
 * @s: pointer to standard V4L2 sub device structure
 * @a: pointer to standard V4L2 VIDIOC_G_PARM ioctl structure
 *
 * Returns the sensor's video CAPTURE parameters.
 */
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
		pr_debug("camdev:   type is unknown - %d\n", a->type);
		ret = -EINVAL;
		break;
	}

	return ret;
}

/*!
 * camdev_s_parm - V4L2 sensor interface handler for VIDIOC_S_PARM ioctl
 * @s: pointer to standard V4L2 sub device structure
 * @a: pointer to standard V4L2 VIDIOC_S_PARM ioctl structure
 *
 * Configures the sensor to use the input parameters, if possible.  If
 * not possible, reverts to the old parameters and returns the
 * appropriate error code.
 */
static int camdev_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *a)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camdev *sensor = to_camdev(client);
	struct v4l2_fract *timeperframe = &a->parm.capture.timeperframe;
//	u32 tgt_fps;	/* target frames per secound */
	enum camdev_mode mode = a->parm.capture.capturemode;
	const camera_info_t *ci = to_camera_info();
	int ret = 0;

	switch (a->type) {
	/* This is the only case currently handled. */
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
		/* Check that the new frame rate is allowed. */

		timeperframe->denominator = ci->fps;
		timeperframe->numerator = 1;

		sensor->streamcap.timeperframe = *timeperframe;
		sensor->streamcap.capturemode = mode;
		sensor->pix.width = ci->width;
		sensor->pix.height = ci->height;

		break;

	/* These are all the possible cases. */
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	case V4L2_BUF_TYPE_VBI_CAPTURE:
	case V4L2_BUF_TYPE_VBI_OUTPUT:
	case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
	case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
		pr_debug("camdev:   type is not " \
			"V4L2_BUF_TYPE_VIDEO_CAPTURE but %d\n",
			a->type);
		ret = -EINVAL;
		break;

	default:
		pr_debug("camdev:   type is unknown - %d\n", a->type);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int camdev_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camdev *sensor = to_camdev(client);

#if 0
	if (enable)
		camdev_start();
	else
		camdev_stop();

#endif

	sensor->on = enable;
	return 0;
}

static int camdev_set_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *format)
{
	struct v4l2_mbus_framefmt *mf = &format->format;
	const camera_info_t *ci = to_camera_info();
	const struct camdev_datafmt *fmt = &ci->cs;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camdev *sensor = to_camdev(client);

	if (format->pad)
		return -EINVAL;

	if (!fmt) {
		mf->code	= fmt->code;
		mf->colorspace	= fmt->colorspace;
	}

	mf->field	= V4L2_FIELD_NONE;

	if (format->which == V4L2_SUBDEV_FORMAT_TRY)
		return 0;

	sensor->fmt = fmt;

	return 0;
}

static int camdev_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *format)
{
	struct v4l2_mbus_framefmt *mf = &format->format;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camdev *sensor = to_camdev(client);
	const camera_info_t *ci = to_camera_info();
	/*const struct camdev_datafmt *fmt = sensor->fmt;*/

	if (format->pad)
		return -EINVAL;

	memset(mf, 0, sizeof(struct v4l2_mbus_framefmt));

	mf->code = ci->cs.code;
	mf->colorspace = ci->cs.colorspace;

	mf->width = sensor->pix.width;
	mf->height = sensor->pix.height;
	mf->field = V4L2_FIELD_NONE;
	mf->reserved[1] = (sensor->mipi_csi) ? ci->hs_setting : 0;

	dev_dbg(&client->dev, "camdev: %s code=0x%x, w/h=(%d,%d), colorspace=%d, field=%d\n",
		__func__, mf->code, mf->width, mf->height, mf->colorspace, mf->field);

	return 0;
}

static int camdev_enum_code(struct v4l2_subdev *sd,
			    struct v4l2_subdev_pad_config *cfg,
			    struct v4l2_subdev_mbus_code_enum *code)
{
	const camera_info_t *ci = to_camera_info();

	if (code->pad || code->index > 0)
		return -EINVAL;
	
	code->code = ci->cs.code;
	return 0;
}

/*!
 * camdev_enum_framesizes - V4L2 sensor interface handler for
 *			   VIDIOC_ENUM_FRAMESIZES ioctl
 * @s: pointer to standard V4L2 device structure
 * @fsize: standard V4L2 VIDIOC_ENUM_FRAMESIZES ioctl structure
 *
 * Return 0 if successful, otherwise -EINVAL.
 */
static int camdev_enum_framesizes(struct v4l2_subdev *sd,
			       struct v4l2_subdev_pad_config *cfg,
			       struct v4l2_subdev_frame_size_enum *fse)
{
	const camera_info_t *ci = to_camera_info();
	
	if (fse->index > 0)
		return -EINVAL;

	fse->max_width = ci->width;
	fse->min_width = fse->max_width;
	fse->max_height = ci->height;
	fse->min_height = fse->max_height;
	return 0;
}

/*!
 * camdev_enum_frameintervals - V4L2 sensor interface handler for
 *			       VIDIOC_ENUM_FRAMEINTERVALS ioctl
 * @s: pointer to standard V4L2 device structure
 * @fival: standard V4L2 VIDIOC_ENUM_FRAMEINTERVALS ioctl structure
 *
 * Return 0 if successful, otherwise -EINVAL.
 */
static int camdev_enum_frameintervals(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_frame_interval_enum *fie)
{
	const camera_info_t *ci = to_camera_info();

	if (fie->index < 0 || fie->index > 0)
		return -EINVAL;

	if (fie->width == 0 || fie->height == 0 ||
	    fie->code == 0) {
		pr_warning("camdev: Please assign pixel format, width and height.\n");
		return -EINVAL;
	}

	fie->interval.numerator = 1;
	fie->interval.denominator = ci->fps;

	return 0;
}

static int camdev_link_setup(struct media_entity *entity,
			   const struct media_pad *local,
			   const struct media_pad *remote, u32 flags)
{
	return 0;
}

static struct v4l2_subdev_video_ops camdev_subdev_video_ops = {
	.g_parm = camdev_g_parm,
	.s_parm = camdev_s_parm,
	.s_stream = camdev_s_stream,
};

static const struct v4l2_subdev_pad_ops camdev_subdev_pad_ops = {
	.enum_frame_size       = camdev_enum_framesizes,
	.enum_frame_interval   = camdev_enum_frameintervals,
	.enum_mbus_code        = camdev_enum_code,
	.set_fmt               = camdev_set_fmt,
	.get_fmt               = camdev_get_fmt,
};

static struct v4l2_subdev_core_ops camdev_subdev_core_ops = {
	.g_chip_ident = camdev_chip_ident,
	.s_power	= camdev_s_power,
};

static struct v4l2_subdev_ops camdev_subdev_ops = {
	.core	= &camdev_subdev_core_ops,
	.video = &camdev_subdev_video_ops,
	.pad	= &camdev_subdev_pad_ops,
};

static const struct media_entity_operations camdev_sd_media_ops = {
	.link_setup = camdev_link_setup,
};

static const u8 dayun_720p[] =
{
	/*********************************************************************/						 
	/* MAX96705/MAX9296	HIM pullup												 */ 	
	/*********************************************************************/	 

	0x04,0x90,0x0F,0x00,0x01,

	0x04,0x90,0x00,0x06,0x9F,	//enable GMSL1 mode
	0x04,0x90,0x0B,0x06,0x87,	//enable HIM
	0x04,0x90,0x03,0x13,0x00,	// disable mipi output 
	0x04,0x90,0x0B,0x0D,0x80,	//Enable Auto ACK
	0x00,0x2,

	0x03,0x80,0x04,0x43,	//Enable Configuration Link
	0x00,0x05, 			 //Delay 5ms		

	0x03,0x80,0x07,0x85,	//Enable DBL, Edge Select, HS/VS Encoding
	0x00,0x0A,				//Delay 2ms Wait
	0x04,0x90,0x0B,0x0D,0x00,	//disable i2c Auto ACK
	0x00,0x02,				//delay 2ms
	0x04,0x90,0x0B,0x07,0x05,	// don't double on deserializer
	0x00,0x02,
	0x03,0x80,0x67,0xC4,	// double align with HS
	0x03,0x80,0x0F,0xBF,
	

	0x04,0x90,0x04,0x0B,0x07,	// Enable 3 Mappings
	0x04,0x90,0x04,0x2D,0x15,	// Destionation Controller = Controller 1. Controller 1 sends data to MIPI Port A
	// For the following MSB 2 bits = VC, LSB 6 bits =DT
	0x04,0x90,0x04,0x0D,0x1E,	// SRC	DT = RAW12
	0x04,0x90,0x04,0x0E,0x1E,	// DEST DT = RAW12
	0x04,0x90,0x04,0x0F,0x00,	// SRC	DT = Frame Start
	0x04,0x90,0x04,0x10,0x00,	// DEST DT = Frame Start
	0x04,0x90,0x04,0x11,0x01,	// SRC	DT = Frame End
	0x04,0x90,0x04,0x12,0x01,	// DEST DT = Frame End
	
	0x04,0x90,0x03,0x30,0x04,	// set des in 2x4 mode 
	0x04,0x90,0x04,0x4A,0xD0,	// Four lane output from MIPI Port A
	0x04,0x90,0x03,0x20,0x2F,	// Set MIPI speed to be 200Mbps
	
	// Set DT, VC, BPP	FOR PIPE X
	0x04,0x90,0x03,0x13,0x80,	// BPP = 16
	0x04,0x90,0x03,0x16,0x1E,	// DT = 1E (YUV422 8-bit)
	0x04,0x90,0x03,0x1D,0x6F,	// Enable BPP/DT/VC.

	
	// Disable Link B to enable LOCK output. LOCK will not assert w/o all active links having locked_g1
	//0x04,0x90,0x0F,0x00,0x01,
	// Disable processing HS and DE signals
	0x04,0x90,0x0B,0x0F,0x01,

	// Swap MSBs with LSBs
	//0x04,0x90,0x01,0xC0,0x07,
	//0x04,0x90,0x01,0xC1,0x06,
	//0x04,0x90,0x01,0xC2,0x05,
	//0x04,0x90,0x01,0xC3,0x04,
	//0x04,0x90,0x01,0xC4,0x03,
	//0x04,0x90,0x01,0xC5,0x02,
	//0x04,0x90,0x01,0xC6,0x01,
	//0x04,0x90,0x01,0xC7,0x00,
	//0x04,0x90,0x01,0xC8,0x0F,
	//0x04,0x90,0x01,0xC9,0x0E,
	//0x04,0x90,0x01,0xCA,0x0D,
	//0x04,0x90,0x01,0xCB,0x0C,
	//0x04,0x90,0x01,0xCC,0x0B,
	//0x04,0x90,0x01,0xCD,0x0A,
	//0x04,0x90,0x01,0xCE,0x09,
	//0x04,0x90,0x01,0xCF,0x08,

	// Swap YU and YV positions and MSBs/LSBs
	//0x04,0x90,0x01,0xC0,0x0F,
	//0x04,0x90,0x01,0xC1,0x0E,
	//0x04,0x90,0x01,0xC2,0x0D,
	//0x04,0x90,0x01,0xC3,0x0C,
	//0x04,0x90,0x01,0xC4,0x0B,
	//0x04,0x90,0x01,0xC5,0x0A,
	//0x04,0x90,0x01,0xC6,0x09,
	//0x04,0x90,0x01,0xC7,0x08,
	//0x04,0x90,0x01,0xC8,0x07,
	//0x04,0x90,0x01,0xC9,0x06,
	//0x04,0x90,0x01,0xCA,0x05,
	//0x04,0x90,0x01,0xCB,0x04,
	//0x04,0x90,0x01,0xCC,0x03,
	//0x04,0x90,0x01,0xCD,0x02,
	//0x04,0x90,0x01,0xCE,0x01,
	//0x04,0x90,0x01,0xCF,0x00,

	// Swap MSBs with LSBs and swap YU and YV positions
	0x04,0x90,0x01,0xC0,0x08,
	0x04,0x90,0x01,0xC1,0x09,
	0x04,0x90,0x01,0xC2,0x0A,
	0x04,0x90,0x01,0xC3,0x0B,
	0x04,0x90,0x01,0xC4,0x0C,
	0x04,0x90,0x01,0xC5,0x0D,
	0x04,0x90,0x01,0xC6,0x0E,
	0x04,0x90,0x01,0xC7,0x0F,
	0x04,0x90,0x01,0xC8,0x00,
	0x04,0x90,0x01,0xC9,0x01,
	0x04,0x90,0x01,0xCA,0x02,
	0x04,0x90,0x01,0xCB,0x03,
	0x04,0x90,0x01,0xCC,0x04,
	0x04,0x90,0x01,0xCD,0x05,
	0x04,0x90,0x01,0xCE,0x06,
	0x04,0x90,0x01,0xCF,0x07,
		
	0x03,0x80,0x04,0x83,	//Enable all serializer and disable configuration link
	0x00,0x32, 		//delay 50ms
 

	0x04,0x90,0x03,0x13,0x82	//enable mipi output
};

static const u8 dayun_720p_b[] =
{
/*********************************************************************/ 					 
/* MAX96705/MAX9296 HIM pullup												 */ 	
/*********************************************************************/  

	0x04,0x90,0x0F,0x00,0x02,

	0x04,0x90,0x00,0x06,0x5F,	//enable GMSL1 mode
	0x04,0x90,0x0C,0x06,0x87,	//enable HIM
	0x04,0x90,0x03,0x13,0x00,	// disable mipi output 
	0x04,0x90,0x0C,0x0D,0x80,	//Enable i2c Auto ACK
	0x00,0x02,				//delay 2ms


	0x03,0x80,0x04,0x43,	//Enable Configuration Link
	0x00,0x05,				//Delay 5ms
	0x03,0x80,0x07,0x85,	//Enable DBL, Edge Select, HS/VS Encoding
	0x00,0x0A,		//delay 10ms
	0x04,0x90,0x0C,0x0D,0x00,	//disable i2c Auto ACK
	0x00,0x05,				//delay 2ms

	0x04,0x90,0x0C,0x07,0x05,	// don't double on deserializer
	0x00,0x02,			//
	0x03,0x80,0x67,0xC4,	// double align with HS
	0x03,0x80,0x0F,0xBF,	//trig for 360 camera			  


	// Send RAW12, FS, and FE from Pipe Y to Controller 1
	0x04,0x90,0x04,0x4B,0x07,	// Enable 3 Mappings
	0x04,0x90,0x04,0x6D,0x15,	// Destionation Controller = Controller 1. Controller 1 sends data to MIPI Port A
	// For the following MSB 2 bits = VC, LSB 6 bits =DT
	0x04,0x90,0x04,0x4D,0x1E,	// SRC	DT = RAW12
	0x04,0x90,0x04,0x4E,0x1E,	// DEST DT = RAW12
	0x04,0x90,0x04,0x4F,0x00,	// SRC	DT = Frame Start
	0x04,0x90,0x04,0x50,0x00,	// DEST DT = Frame Start
	0x04,0x90,0x04,0x51,0x01,	// SRC	DT = Frame End
	0x04,0x90,0x04,0x52,0x01,	// DEST DT = Frame End
		
	0x04,0x90,0x03,0x30,0x04,	// set des in 2x4 mode 
	0x04,0x90,0x04,0x4A,0xD0,	// Four lane output from MIPI Port A
		
	// Set DT, VC, BPP	FOR PIPE Y
	0x04,0x90,0x03,0x19,0x10,	// BPP = 16 for 5 bit
	//0x04,0x90,0x03,0x1A,0x00, // BPP = 16 for 2 LSB
	0x04,0x90,0x03,0x16,0x40,	// DT = 1E (YUV422 8-bit) for 2 MSB
	0x04,0x90,0x03,0x17,0x0E,	// DT =1E for 4 LSB
	0x04,0x90,0x03,0x1d,0x80,	// Enable BPP/DT/VC
	0x04,0x90,0x03,0x20,0x2F,	//mipi speed to set at 400Mbps/lane
		
	//ignore first frame output
	0x04,0x90,0x03,0x25,0x80,

	// Disable Link A to enable LOCK output. LOCK will not assert w/o all active links having locked_g1
	// 0x04,0x90,0x0F,0x00,0x02,

	// Disable processing HS and DE signals
	0x04,0x90,0x0C,0x0F,0x01,

	// Swap MSBs with LSBs
	//0x04,0x90,0x01,0xE0,0x07,
	//0x04,0x90,0x01,0xE1,0x06,
	//0x04,0x90,0x01,0xE2,0x05,
	//0x04,0x90,0x01,0xE3,0x04,
	//0x04,0x90,0x01,0xE4,0x03,
	//0x04,0x90,0x01,0xE5,0x02,
	//0x04,0x90,0x01,0xE6,0x01,
	//0x04,0x90,0x01,0xE7,0x00,
	//0x04,0x90,0x01,0xE8,0x0F,
	//0x04,0x90,0x01,0xE9,0x0E,
	//0x04,0x90,0x01,0xEA,0x0D,
	//0x04,0x90,0x01,0xEB,0x0C,
	//0x04,0x90,0x01,0xEC,0x0B,
	//0x04,0x90,0x01,0xED,0x0A,
	//0x04,0x90,0x01,0xEE,0x09,
	//0x04,0x90,0x01,0xEF,0x08,

	// Swap YU and YV positions and MSBs/LSBs
	//0x04,0x90,0x01,0xE0,0x0F,
	//0x04,0x90,0x01,0xE1,0x0E,
	//0x04,0x90,0x01,0xE2,0x0D,
	//0x04,0x90,0x01,0xE3,0x0C,
	//0x04,0x90,0x01,0xE4,0x0B,
	//0x04,0x90,0x01,0xE5,0x0A,
	//0x04,0x90,0x01,0xE6,0x09,
	//0x04,0x90,0x01,0xE7,0x08,
	//0x04,0x90,0x01,0xE8,0x07,
	//0x04,0x90,0x01,0xE9,0x06,
	//0x04,0x90,0x01,0xEA,0x05,
	//0x04,0x90,0x01,0xEB,0x04,
	//0x04,0x90,0x01,0xEC,0x03,
	//0x04,0x90,0x01,0xED,0x02,
	//0x04,0x90,0x01,0xEE,0x01,
	//0x04,0x90,0x01,0xEF,0x00,

	// Swap MSBs with LSBs and swap YU and YV positions
	0x04,0x90,0x01,0xE0,0x08,
	0x04,0x90,0x01,0xE1,0x09,
	0x04,0x90,0x01,0xE2,0x0A,
	0x04,0x90,0x01,0xE3,0x0B,
	0x04,0x90,0x01,0xE4,0x0C,
	0x04,0x90,0x01,0xE5,0x0D,
	0x04,0x90,0x01,0xE6,0x0E,
	0x04,0x90,0x01,0xE7,0x0F,
	0x04,0x90,0x01,0xE8,0x00,
	0x04,0x90,0x01,0xE9,0x01,
	0x04,0x90,0x01,0xEA,0x02,
	0x04,0x90,0x01,0xEB,0x03,
	0x04,0x90,0x01,0xEC,0x04,
	0x04,0x90,0x01,0xED,0x05,
	0x04,0x90,0x01,0xEE,0x06,
	0x04,0x90,0x01,0xEF,0x07,

	0x03,0x80,0x04,0x83,	//Enable all serializer and disable configuration link
	0x00,0x32,		//delay 50ms
 
	0x04,0x90,0x03,0x13,0x02	// enable mipi output
};

static const u8 dayun_sv_720p[] =
{
	//
	// SERDES Setup Script for RGB888 --> MAX9291/9275---> MAX9296A (G1 mode, ES4.0)
	// Must use GMSL PHY A. If PHY B is needed, separate deserializer register writes are required.
	// blow 1920*720P@60 FPS
	// Four lane MIPI CSI-2 leaving MAX9296A MIPI Port A @ 600Mbps
	// Author(s): XS
	// Revision 1: Initial
	// Revision 2: 
	//
	//[Words to Write][I2C Slave address][I2C Register Address][Data Word 0][Data Word 1]

	/*********************************************************************/
	/*                    Reverse Channel                                */
	/* It is recommended to enable HIM on power up via HW Pullups        */
	/* MAX96705 Pin 13 GPO/HIM 30K pullup                                */
	/* MAX9286  HIM pullup                                               */		
	/*********************************************************************/

	0x04,0x90,0x0F,0x00,0x01,

	0x04,0x90,0x00,0x06,0x9F,	//enable GMSL1 mode
	//0x04,0x90,0x0B,0x0D,0x80,	//Enable Auto ACK
	//0x00,0x14,
	/*********************************************************************/
	/*                   Recommended HIM Setup                           */		
	/*                   HIM Mode Enable VIA HW                          */
	/*********************************************************************/		
	
	 //0x03,0x80,0x04,0x43,	//Enable Configuration Link
	 //0x00,0x05,              //Delay 5ms		

	/*********************************************************************/
	/*                   MAX9286 Pre MAX9271 Setup                       */
	/*********************************************************************/
	//0x03,0x90,0x15,0x03,	//Disable CSI output
	//0x03,0x90,0x12,0xF7,	//CSI Lanes, CSI DBL, GMSL DBL, Data Type
	//0x03,0x90,0x01,0x02,	//Frame Sync Semi-Auto mode 
	//0x03,0x90,0x00,0xFF,    //Enable GMSL Links

	//0x03,0x80,0x07,0x22,    //BWS =1 for 32 bit mode
	//0x00,0xFF,              //Delay 2ms Wait
	0x04,0x90,0x0B,0x07,0x08,	// 0x08 for BWS = open at high bandwidth mode(27bit, 24+3); 0x20 for BWS= high at 32 bit mode
	//0x04,0x90,0x0b,0x96,0x00,// Use OLDI RGB888 , if use VESA, set 0x10
	//0x00,0x02,	
	//0x03,0x80,0x0F,0xBF,
	//
	//
	// Mapping Control
	// Send RGB888, FS, and FE from Pipe X to Controller 1
	0x04,0x90,0x04,0x0B,0x07,	// Enable 3 Mappings
	0x04,0x90,0x04,0x2D,0x15,	// Destionation Controller = Controller 1. Controller 1 sends data to MIPI Port A
	// For the following MSB 2 bits = VC, LSB 6 bits =DT
	0x04,0x90,0x04,0x0D,0x24,	// SRC  DT = rgb888
	0x04,0x90,0x04,0x0E,0x24,	// DEST DT = rgb888
	0x04,0x90,0x04,0x0F,0x00,	// SRC  DT = Frame Start
	0x04,0x90,0x04,0x10,0x00,	// DEST DT = Frame Start
	0x04,0x90,0x04,0x11,0x01,	// SRC  DT = Frame End
	0x04,0x90,0x04,0x12,0x01,	// DEST DT = Frame End
	
	0x04,0x90,0x03,0x30,0x04,	// set des in 2x4 mode 
	0x04,0x90,0x04,0x4A,0xD0,	// Four lane output from MIPI Port A
	0x04,0x90,0x03,0x20,0x24,	// Set MIPI speed to be 1200Mbps

	// Set DT, VC, BPP  FOR PIPE X
	0x04,0x90,0x03,0x13,0x88,	// BPP = 24, diable CSI output
	0x04,0x90,0x03,0x16,0xA4,	// DT = 24 (RGB 888) For X and Y 2 high bits
	0x04,0x90,0x03,0x17,0x04,	// DT = 24 (RGB888) for Y 4 low bits 
	0x04,0x90,0x03,0x1D,0x6F,	// Enable BPP/DT/VC.

	// 24->12 bit conversion
	// 0x04,0x90,0x04,0x33,0x01,
	// Disable Link B to enable LOCK output. LOCK will not assert w/o all active links having locked_g1
	//0x04,0x90,0x0F,0x00,0x01,
	// Enable processing HS and DE signals
	0x04,0x90,0x0B,0x0F,0x09,
	// Invert VSYNC in deserializer if the mipi waveform looks many HSs.
	 0x04,0x90,0x01,0xD9,0x59, 
	// Enable GMSL1 to GMSL2 color mapping
	0x04,0x90,0x0B,0x96,0x83,
	// HIGHIMM = 1 and use 18/19 for HS/VS
	 0x04,0x90,0x0B,0x06,0xE8, 
	// Shift GMSL1 HVD out of the way
	0x04,0x90,0x0B,0xA7,0x45,

	// Swap MSBs with LSBs and swap YU and YV positions.
	//	0x04,0x90,0x01,0xC0,0x06,
	//	0x04,0x90,0x01,0xC1,0x07,
	//	0x04,0x90,0x01,0xC2,0x00,
	//	0x04,0x90,0x01,0xC3,0x01,
	//	0x04,0x90,0x01,0xC4,0x02,
	//	0x04,0x90,0x01,0xC5,0x03,
	//	0x04,0x90,0x01,0xC6,0x04,
	//	0x04,0x90,0x01,0xC7,0x05,
	//	0x04,0x90,0x01,0xC8,0x0e,
	//	0x04,0x90,0x01,0xC9,0x0f,
	//	0x04,0x90,0x01,0xCA,0x08,
	//	0x04,0x90,0x01,0xCB,0x09,
	//	0x04,0x90,0x01,0xCC,0x0a,
	//	0x04,0x90,0x01,0xCD,0x0b,
	//	0x04,0x90,0x01,0xCE,0x0c,
	//	0x04,0x90,0x01,0xCF,0x0d,
	//	0x04,0x90,0x01,0xD0,0x16,
	//	0x04,0x90,0x01,0xD1,0x17,
	//	0x04,0x90,0x01,0xD2,0x10,
	//	0x04,0x90,0x01,0xD3,0x11,
	//	0x04,0x90,0x01,0xD4,0x12,
	//	0x04,0x90,0x01,0xD5,0x13,
	//	0x04,0x90,0x01,0xD6,0x14,
	//	0x04,0x90,0x01,0xD7,0x15,




	/*********************************************************************/
	/*                      Image Sensor Setup                           */
	/*********************************************************************/

	/*********************************************************************/
	/*               MAX9286 Post Image Sensor Setup                     */
	/*********************************************************************/
	//0x00,0x80, 		
	//0x03,0x80,0x04,0x83,	//Enable all serializer and disable configuration link
 
	//0x03,0x90,0x15,0x0b,	//Enable CSI output
	0x04,0x90,0x03,0x13,0x8A	// BPP = 24, enable CSI output
};

static const u8 dayun_sv_720p_b[] =
{
/*********************************************************************/ 					 
/* MAX96705/MAX9296 HIM pullup												 */ 	
/*********************************************************************/  

	// Disable Link A to enable LOCK output. LOCK will not assert w/o all active links having locked_g1
	0x04,0x90,0x0F,0x00,0x02,

	0x04,0x90,0x00,0x06,0x5F,	//enable GMSL1 mode
	0x04,0x90,0x0C,0x06,0x87,	//enable HIM
	0x04,0x90,0x03,0x13,0x00,	// disable mipi output 
	0x04,0x90,0x0C,0x0D,0x80,	//Enable i2c Auto ACK
	0x00,0x02,				//delay 2ms


	0x03,0x80,0x04,0x43,	//Enable Configuration Link
	0x00,0x05,				//Delay 5ms
	0x03,0x80,0x07,0x85,	//Enable DBL, Edge Select, HS/VS Encoding
	0x00,0x0A,		//delay 10ms
	0x04,0x90,0x0C,0x0D,0x00,	//disable i2c Auto ACK
	0x00,0x05,				//delay 2ms
	0x04,0x90,0x0C,0x07,0x05,	// don't double on deserializer
	0x00,0x02,			//
	0x03,0x80,0x67,0xC4,	// double align with HS
	0x03,0x80,0x0F,0xBF,	//trig for 360 camera			  

	// Send RAW12, FS, and FE from Pipe Y to Controller 1
	0x04,0x90,0x04,0x4B,0x07,	// Enable 3 Mappings
	0x04,0x90,0x04,0x6D,0x15,	// Destionation Controller = Controller 1. Controller 1 sends data to MIPI Port A
	// For the following MSB 2 bits = VC, LSB 6 bits =DT
	0x04,0x90,0x04,0x4D,0x1E,	// SRC	DT = RAW12
	0x04,0x90,0x04,0x4E,0x1E,	// DEST DT = RAW12
	0x04,0x90,0x04,0x4F,0x00,	// SRC	DT = Frame Start
	0x04,0x90,0x04,0x50,0x00,	// DEST DT = Frame Start
	0x04,0x90,0x04,0x51,0x01,	// SRC	DT = Frame End
	0x04,0x90,0x04,0x52,0x01,	// DEST DT = Frame End
		
	0x04,0x90,0x03,0x30,0x04,	// set des in 2x4 mode 
	0x04,0x90,0x04,0x4A,0xD0,	// Four lane output from MIPI Port A
		
	// Set DT, VC, BPP	FOR PIPE Y
	0x04,0x90,0x03,0x19,0x10,	// BPP = 16 for 5 bit
	//0x04,0x90,0x03,0x1A,0x00, // BPP = 16 for 2 LSB
	0x04,0x90,0x03,0x16,0x40,	// DT = 1E (YUV422 8-bit) for 2 MSB
	0x04,0x90,0x03,0x17,0x0E,	// DT =1E for 4 LSB
	0x04,0x90,0x03,0x1d,0x80,	// Enable BPP/DT/VC
	0x04,0x90,0x03,0x20,0x2F,	//mipi speed to set at 400Mbps/lane
		
	//ignore first frame output
	0x04,0x90,0x03,0x25,0x80,

	// Disable Link A to enable LOCK output. LOCK will not assert w/o all active links having locked_g1
	//0x04,0x90,0x0F,0x00,0x02,

	// Disable processing HS and DE signals
	0x04,0x90,0x0C,0x0F,0x01,

	// Swap MSBs with LSBs
	//0x04,0x90,0x01,0xE0,0x07,
	//0x04,0x90,0x01,0xE1,0x06,
	//0x04,0x90,0x01,0xE2,0x05,
	//0x04,0x90,0x01,0xE3,0x04,
	//0x04,0x90,0x01,0xE4,0x03,
	//0x04,0x90,0x01,0xE5,0x02,
	//0x04,0x90,0x01,0xE6,0x01,
	//0x04,0x90,0x01,0xE7,0x00,
	//0x04,0x90,0x01,0xE8,0x0F,
	//0x04,0x90,0x01,0xE9,0x0E,
	//0x04,0x90,0x01,0xEA,0x0D,
	//0x04,0x90,0x01,0xEB,0x0C,
	//0x04,0x90,0x01,0xEC,0x0B,
	//0x04,0x90,0x01,0xED,0x0A,
	//0x04,0x90,0x01,0xEE,0x09,
	//0x04,0x90,0x01,0xEF,0x08,

	// Swap YU and YV positions and MSBs/LSBs
	//0x04,0x90,0x01,0xE0,0x0F,
	//0x04,0x90,0x01,0xE1,0x0E,
	//0x04,0x90,0x01,0xE2,0x0D,
	//0x04,0x90,0x01,0xE3,0x0C,
	//0x04,0x90,0x01,0xE4,0x0B,
	//0x04,0x90,0x01,0xE5,0x0A,
	//0x04,0x90,0x01,0xE6,0x09,
	//0x04,0x90,0x01,0xE7,0x08,
	//0x04,0x90,0x01,0xE8,0x07,
	//0x04,0x90,0x01,0xE9,0x06,
	//0x04,0x90,0x01,0xEA,0x05,
	//0x04,0x90,0x01,0xEB,0x04,
	//0x04,0x90,0x01,0xEC,0x03,
	//0x04,0x90,0x01,0xED,0x02,
	//0x04,0x90,0x01,0xEE,0x01,
	//0x04,0x90,0x01,0xEF,0x00,

	// Swap MSBs with LSBs and swap YU and YV positions
	0x04,0x90,0x01,0xE0,0x08,
	0x04,0x90,0x01,0xE1,0x09,
	0x04,0x90,0x01,0xE2,0x0A,
	0x04,0x90,0x01,0xE3,0x0B,
	0x04,0x90,0x01,0xE4,0x0C,
	0x04,0x90,0x01,0xE5,0x0D,
	0x04,0x90,0x01,0xE6,0x0E,
	0x04,0x90,0x01,0xE7,0x0F,
	0x04,0x90,0x01,0xE8,0x00,
	0x04,0x90,0x01,0xE9,0x01,
	0x04,0x90,0x01,0xEA,0x02,
	0x04,0x90,0x01,0xEB,0x03,
	0x04,0x90,0x01,0xEC,0x04,
	0x04,0x90,0x01,0xED,0x05,
	0x04,0x90,0x01,0xEE,0x06,
	0x04,0x90,0x01,0xEF,0x07,

	0x03,0x80,0x04,0x83,	//Enable all serializer and disable configuration link
	0x00,0x32,		//delay 50ms
 
	0x04,0x90,0x03,0x13,0x02	// enable mipi output
};


/* imx8x internal clk config
         {1920, 1080, 30, 0x0B},
         {1920, 1080, 15, 0x10},
         {1280, 720,  30, 0x11},
         {1280, 720,  15, 0x16},
         {640,  480,  30, 0x1E},
         {640,  480,  15, 0x23},
         {320,  240,  30, 0x1E},
         {320,  240,  15, 0x23},
--------->
o8.1.0_1.2.0_8qxp-beta2£º
* 0~ 80Mbps: 0xB
* 80~250Mbps: 0x8
* 250~1.5Gbps: 0x6
static u8 rxhs_settle[3] = {0xB, 0x8, 0x6};
 
android_p9.0.0_1.0.0-beta£º
* 0~ 80Mbps: 0xB
* 80~250Mbps: 0x8
* 250~1.5Gbps: 0x6
static u8 rxhs_settle[3] = {0xD, 0xA, 0x7};*/
static const camera_info_t camera_list[] =
{
	{
		.id = "dayun-720p",
		.tx_chip = "max96705",
		.ident = "max9298",
		.width = 1280,
		.height = 720,
		.fps = 25,
		.pix_fmt = V4L2_PIX_FMT_UYVY,
		.hs_setting = 6,
		.cs = {.code = MEDIA_BUS_FMT_UYVY8_2X8, .colorspace = V4L2_COLORSPACE_JPEG},
		.cfg_dat = dayun_720p,
		.cfg_len = sizeof(dayun_720p)
	},

	{
		.id = "dayun-sv-720p",
		.tx_chip = "max9275",
		.ident = "max9298",
		.width = 1280,
		.height = 720,
		.fps = 30,
		.pix_fmt = V4L2_PIX_FMT_RGB24,
		.hs_setting = 6,
		.cs = {.code = MEDIA_BUS_FMT_RGB888_1X24, .colorspace = V4L2_COLORSPACE_SRGB},
		.cfg_dat = dayun_sv_720p,
		.cfg_len = sizeof(dayun_sv_720p)
	},

	{
		/* for car internal recording camera only(channel b)!
		   not for set or detect in probe stage!
		*/
		.id = "dayun-720p-b",
		.tx_chip = "max96705",
		.ident = "max9298",
		.width = 1280,
		.height = 720,
		.fps = 25,
		.pix_fmt = V4L2_PIX_FMT_UYVY,
		.hs_setting = 6,
		.cs = {.code = MEDIA_BUS_FMT_UYVY8_2X8, .colorspace = V4L2_COLORSPACE_JPEG},
		.cfg_dat = dayun_720p_b,
		.cfg_len = sizeof(dayun_720p_b)
	},

	{
		/* for car internal recording camera only(channel b sv)!
		   not for set or detect in probe stage!
		*/
		.id = "dayun-sv-720p-b",
		.tx_chip = "max96705",
		.ident = "max9298",
		.width = 1280,
		.height = 720,
		.fps = 25,
		.pix_fmt = V4L2_PIX_FMT_UYVY,
		.hs_setting = 6,
		.cs = {.code = MEDIA_BUS_FMT_UYVY8_2X8, .colorspace = V4L2_COLORSPACE_JPEG},
		.cfg_dat = dayun_sv_720p_b,
		.cfg_len = sizeof(dayun_sv_720p_b)
	},

	{
		.id = NULL
	}
};

static int camdev_serializer_detect(struct i2c_client *cli, struct i2c_client *cli1, const char **id)
{
	u8 r0, r1, rid;

	pr_info("camdev: camera type using auto-detect mode\n");

	/******** detect max96705 ********/
	// ref dayun_720p array
	rx_read_reg(0xf00, &r0, exit0); // backup
	rx_write_reg(0xf00, 0x01, exit0);
	rx_read_reg(0xb0d, &r1, exit0); // backup
	rx_write_reg(0xb0d, 0x80, exit0);
	msleep(20);

	if(0 == tx_raw_read_reg(0x1e, &rid))
	{
		if(0x40 == (rid & 0x40))
		{
			pr_info("camdev: max96705 detected, standard HD camera found\n");
			*id = "dayun-720p";
		}
		else
		{
			pr_err("camdev error: max96705 id register read out 0x%02x, expected 0x41\n", rid);
			goto exit0;
		}
	}
	else
	{
		// need max9275 detect code ??????
		pr_info("camdev: max9275 detected, surr-view HD camera found\n");
		*id = "dayun-sv-720p";
	}

	rx_write_reg(0xb0d, r1, exit0); // restore
	rx_write_reg(0xf00, r0, exit0); // restore

	return 0;

exit0:
	return -1;
}

static int camdev_probe1(void)
{
	int retval;
	const camera_info_t *ci = to_camera_info();
	struct device *dev = &camdev_data.i2c_client->dev;

//	camdev_data.io_init = camdev_reset;
//	camdev_data.i2c_client = client;

	camdev_data.pix.pixelformat = ci->pix_fmt;
	camdev_data.pix.width = ci->width;
	camdev_data.pix.height =  ci->height;
	camdev_data.streamcap.capability = V4L2_MODE_HIGHQUALITY |
					   V4L2_CAP_TIMEPERFRAME;
	camdev_data.streamcap.capturemode = 0;
	camdev_data.streamcap.timeperframe.denominator = ci->fps;
	camdev_data.streamcap.timeperframe.numerator = 1;

//	camdev_data.rclient0 = cli;
//	camdev_data.rclient1 = cli1;

	camdev_regulator_enable(dev);

	retval = init_device(0, 0);
	if (retval < 0) {
		pr_err("camdev error: camdev init fail\n");
		//goto exit1;
	}

	return 0;
}

static int camdev_remove1(void)
{
	camdev_power_down(1);

	return 0;
}


/* add sysfs interface */
static ssize_t camdev_reload_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	u32 ch;

	if(count > 4)
	{
		pr_info("camdev: sysfs config value count %d exceed 4-byte, skip\n", (int)count);

		goto exit0;
	}
	
	ch = buf[0] - '0';
	
	if(ch > 2)
	{
		pr_info("camdev: invalid input, only '0'/'1'/'2' for 1st char supported!\n");
		goto exit0;
	}

	pr_info("camdev: max9296 config reload %s hw-reset, process '%s', id %d\n",
		(0 == ch) ? "without" : "with", current->comm, current->pid);

	if(init_device((2 == ch), (0 != ch)) < 0)
	{
		goto exit0;
	}

	return (ssize_t)count;

exit0:
	return -1;
}

static ssize_t camdev_vswitch_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char c;

	pr_info("##camdev: query vswitch state(cur=%d), process '%s', id %d\n", camdev_data.ci_cur,
		current->comm, current->pid);
	
	c = camdev_data.ci_cur ? '1' : '0';
	return scnprintf(buf, PAGE_SIZE, "%c\n", c);
}

static ssize_t camdev_vswitch_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	u8 i;
	ssize_t ret = (ssize_t)count;

	if((kstrtou8(buf, 0, &i) == 0) && (i < 2))
	{
		pr_info("##camdev vswitch: init = %d, process '%s', id %d\n", i,
			current->comm, current->pid);

		if(i == camdev_data.ci_cur)
		{
			pr_info("##camdev: vswitch to same config, skip\n");
			goto exit0;
		}

		pr_info("##camdev: switching video source from %d to %d\n", camdev_data.ci_cur, i);

		camdev_data.ci_cur = i;
		
		camdev_remove1();
		msleep(10);
		if(camdev_probe1() < 0)
		{
			ret = -EINVAL;
		}
	}
	else
	{
		pr_err("##camdev: camdev_vswitch_store error, invalid input string para, use '0' or '1'\n");
		ret = -EINVAL;
	}

exit0:
	return ret;
}

static ssize_t camdev_debug_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char c;

	pr_info("##camdev: query debug state(%d), process '%s', id %d\n", camdev_data.debug,
		current->comm, current->pid);
	
	c = camdev_data.debug ? '1' : '0';
	return scnprintf(buf, PAGE_SIZE, "%c\n", c);
}

static ssize_t camdev_debug_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	u8 i;
	ssize_t ret = (ssize_t)count;

	if((kstrtou8(buf, 0, &i) == 0) && (i < 2))
	{
		pr_info("##camdev: set debug state to %d, , process '%s', id %d\n", i,
			current->comm, current->pid);

		camdev_data.debug = i;
	}
	else
	{
		pr_err("##camdev: camdev_debug_store error, invalid input string para, use '0' or '1'\n");
		ret = -EINVAL;
	}

	return ret;

}

static DEVICE_ATTR(reload, S_IWUSR | S_IRUSR, NULL,
		camdev_reload_store);
static DEVICE_ATTR(vswitch, S_IWUSR | S_IRUSR, camdev_vswitch_show,
		 camdev_vswitch_store);
static DEVICE_ATTR(debug, S_IWUSR | S_IRUSR, camdev_debug_show,
		 camdev_debug_store);

static struct attribute *camdev_attrs[] = {
	&dev_attr_reload.attr,
	&dev_attr_vswitch.attr,
	&dev_attr_debug.attr,
	NULL
};

static const struct attribute_group camdev_attr_group = {
	.attrs = camdev_attrs
};

/*!
 * camdev I2C probe function
 *
 * @param adapter            struct i2c_adapter *
 * @return  Error code indicating success or failure
 */
static int camdev_probe(struct i2c_client *client, struct i2c_client *real_client,
			const struct i2c_device_id *id)
{
//	struct pinctrl *pinctrl;
	struct device *dev = &client->dev;
	struct v4l2_subdev *sd;
	int retval, sv = 0;
	const camera_info_t *ci = NULL;
	struct i2c_client *cli = real_client, *cli1;
	const char *camera_id = camdev_data.dts->camera_id;

	sema_init(&vsem, 1);
	mutex_init(&cmutx);

	cli1 = i2c_new_dummy(client->adapter, 0x40);

	if(NULL == cli1)
	{
		dev_err(dev, "camdev error: slave i2c device adpter create fail[3rd]\n");
		return -ENOMEM;
	}

	if(!strcmp(camera_id, "auto"))
	{
		if(-1 == camdev_serializer_detect(cli, cli1, &camera_id))
		{
			return -ENODEV;
		}
	}

	sv = (NULL == strstr(camera_id, "sv")) ? 0 : 1;

	pr_info("camdev: find 'sv' from '%s', result sv = %d\n", camera_id, sv);

	for(ci = &camera_list[0]; (size_t)ci->id; ci++)
	{
		if(!strcmp(camera_id, ci->id))
		{
			camdev_data.ci_a = ci;
		}
		else if(!strcmp("dayun-720p-b", ci->id))
		{
			if(0 == sv) camdev_data.ci_b = ci;
		}
		else if(!strcmp("dayun-sv-720p-b", ci->id))
		{
			if(1 == sv) camdev_data.ci_b = ci;
		}
	}

	if(NULL == camdev_data.ci_a)
	{
		dev_err(dev, "camdev error: no valid mipi-camera info found\n");
		return -ENODEV;
	}

	if(NULL == camdev_data.ci_b)
	{
		dev_err(dev, "camdev error: no valid recording mipi-camera info found\n");
		return -ENODEV;
	}

	/* initial ci pointer is 0(reverse camera) */
	camdev_data.ci_cur = 0;
	ci = camdev_data.ci_a;

	camdev_data.sensor_clk = devm_clk_get(dev, "csi_mclk");
	if (IS_ERR(camdev_data.sensor_clk)) {
		dev_err(dev, "camdev error: get csi_mclk failed\n");
		return PTR_ERR(camdev_data.sensor_clk);
	}

	retval = of_property_read_u32(dev->of_node, "mclk",
					&camdev_data.mclk);
	if (retval) {
		dev_err(dev, "camdev error: mclk frequency is invalid\n");
		return retval;
	}

	retval = of_property_read_u32(dev->of_node, "mclk_source",
					(u32 *) &(camdev_data.mclk_source));
	if (retval) {
		dev_err(dev, "camdev error: mclk_source invalid\n");
		return retval;
	}

	retval = of_property_read_u32(dev->of_node, "csi_id",
				&(camdev_data.csi));
	if (retval) {
		dev_err(dev, "camdev error: csi_id invalid\n");
		return retval;
	}

	camdev_data.mipi_csi  = of_property_read_bool(dev->of_node, "mipi_csi");

	/* Set mclk rate before clk on */
	camdev_set_clk_rate();

	retval = clk_prepare_enable(camdev_data.sensor_clk);
	if (retval < 0) {
		dev_err(dev, "camdev error: %s: enable sensor clk fail\n", __func__);
		return -EINVAL;
	}

	camdev_data.io_init = camdev_max9298_reset;
	camdev_data.i2c_client = client;

	camdev_data.pix.pixelformat = ci->pix_fmt;
	camdev_data.pix.width = ci->width;
	camdev_data.pix.height =  ci->height;
	camdev_data.streamcap.capability = V4L2_MODE_HIGHQUALITY |
					   V4L2_CAP_TIMEPERFRAME;
	camdev_data.streamcap.capturemode = 0;
	camdev_data.streamcap.timeperframe.denominator = ci->fps;
	camdev_data.streamcap.timeperframe.numerator = 1;

	camdev_data.rclient0 = cli;
	camdev_data.rclient1 = cli1;

	camdev_regulator_enable(dev);

	retval = init_device(0, 0);
	if (retval < 0) {
		clk_disable_unprepare(camdev_data.sensor_clk);
		pr_err("camdev error: camdev init fail\n");
		return -ENODEV;
	}

	sd = &camdev_data.subdev;
	v4l2_i2c_subdev_init(sd, client, &camdev_subdev_ops);

	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	sd->entity.function = MEDIA_ENT_F_CAM_SENSOR;
	camdev_data.pads[CAMDEV_SENS_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE;

	retval = media_entity_pads_init(&sd->entity, CAMDEV_SENS_PADS_NUM,
							camdev_data.pads);
	sd->entity.ops = &camdev_sd_media_ops;
	if (retval < 0)
	{
		pr_err("camdev error media_entity_pads_init() fail\n");
		clk_disable_unprepare(camdev_data.sensor_clk);
		return retval;
	}

	retval = v4l2_async_register_subdev(sd);
	if (retval < 0) {
		pr_err("camdev error: %s--Async register failed, ret=%d\n", __func__, retval);
		clk_disable_unprepare(camdev_data.sensor_clk);
		media_entity_cleanup(&sd->entity);
		return retval;
	}

	clk_disable_unprepare(camdev_data.sensor_clk);

	retval = sysfs_create_group(&dev->kobj, &camdev_attr_group);
	if (retval) {
		dev_err(dev, "camdev error: Failure %d creating sysfs group\n", retval);
		clk_disable_unprepare(camdev_data.sensor_clk);
		media_entity_cleanup(&sd->entity);
		return retval;
	}

	pr_info("camdev: %s using %s, is found\n",
		"max9298[9296]",
		camdev_data.mipi_csi ? "MIPI" : "BT656");

	return 0;
}

/*!
 * camdev I2C detach function
 *
 * @param client            struct i2c_client *
 * @return  Error code indicating success or failure
 */
static int camdev_remove(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct v4l2_subdev *sd = i2c_get_clientdata(client);

	sysfs_remove_group(&dev->kobj, &camdev_attr_group);

	v4l2_async_unregister_subdev(sd);

	clk_unprepare(camdev_data.sensor_clk);

	camdev_power_down(1);

#if 0
	if (analog_regulator)
		regulator_disable(analog_regulator);

	if (core_regulator)
		regulator_disable(core_regulator);

	if (io_regulator)
		regulator_disable(io_regulator);
#endif

	return 0;
}

int max9298_driver_register(dtsi_t *dts, pf_camdev_probe *probe, pf_camdev_remove *remove)
{
	/* Set initial values for the sensor struct. */
	memset(&camdev_data, 0, sizeof(camdev_data));

	camdev_data.dts = dts;

	// reset camera 1 & 2
	camdev_cam_reset();

	// do hardware reset(max9296)
	camdev_max9298_reset();

	*probe = camdev_probe;
	*remove = camdev_remove;

	return 0;
}

