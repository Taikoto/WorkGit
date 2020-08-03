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

typedef enum i2c_cmd
{
	RXW = 0, /* mipi rx write */
	RXR, /* mipi rx read */
	TXW, /* mipi tx write */
	TXR, /* mipi tx read */
	TXTW, /* mipi tx table write */
	DLY /* mipi config delay, ms */
}i2c_cmd_t;

typedef struct cmd_set
{
	i2c_cmd_t c; /* i2c cmd */
	u16 reg; /* i2c register addr */
	u16 val; /* i2c register val or table index */
}cmd_set_t;

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
	const cmd_set_t *cmd;
	int cmd_count;
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

	camera_info_t const *ci;

	dtsi_t *dts;
};

/*!
 * Maintains the information on the current state of the sesor.
 */
static struct camdev camdev_data;

#if 0
static struct regulator *io_regulator;
static struct regulator *core_regulator;
static struct regulator *analog_regulator;
#endif

//static int checkid(void);

static const u8 max96705_data[][0x22] =
{
	{
		0x03, 0x04, 0x05, 0x06, 0x07, 0x40, 0x40, 0x0e,
		0x0f, 0x0e, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x00, 0x01, 0x02, 0x40, 0x40, 0x40, 0x40, 0x0e,
		0x0f, 0x40
	},
	{
		0x04, 0x03, 0x02, 0x01, 0x00, 0x40, 0x40, 0x0e,
		0x0f, 0x0e, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
		0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10,
		0x07, 0x06, 0x05, 0x40, 0x40, 0x40, 0x40, 0x0e,
		0x0f, 0x40
	},
	{
		0x13, 0x14, 0x15, 0x16, 0x17, 0x40, 0x40, 0x0e,
		0x0f, 0x0e, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x10, 0x11, 0x12, 0x40, 0x40, 0x40, 0x40, 0x0e,
		0x0f, 0x40
	},
	{
		0x14, 0x13, 0x12, 0x11, 0x10, 0x40, 0x40, 0x0e,
		0x0f, 0x0e, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
		0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00,
		0x17, 0x16, 0x15, 0x40, 0x40, 0x40, 0x40, 0x0e,
		0x0f, 0x40
	}
};

static struct camdev *to_camdev(const struct i2c_client *client)
{
	return container_of(i2c_get_clientdata(client), struct camdev, subdev);
}

static inline void camdev_power_down(int enable)
{

}

static inline void camdev_reset(void)
{
	int rst_gpio = camdev_data.dts->reset_gpio;

	if(-1 == rst_gpio) return;


	pr_info("camdev: --- do hw reset ---\n");
		
	gpio_set_value_cansleep(rst_gpio, 0);
	msleep(10);
		
	gpio_set_value_cansleep(rst_gpio, 1);
	msleep(10);
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

#define rx_write_reg(r, v, lb)		camdev_write_reg(r, v, lb)
#define rx_read_reg(r, p, lb)		camdev_read_reg(r, p, lb)

#define tx_write_reg(r, v, lb)		camdev1_write_reg(r, v, lb)
#define tx_read_reg(r, p, lb)		camdev1_read_reg(r, p, lb)

#define rx_raw_write_reg(r, v)		camdev_raw_write_reg(r, v)
#define rx_raw_read_reg(r, p)		camdev_raw_read_reg(r, p)

#define tx_raw_write_reg(r, v)		camdev1_raw_write_reg(r, v)
#define tx_raw_read_reg(r, p)		camdev1_raw_read_reg(r, p)

#if 0
static int checkid(void)
{
	u8 v;
	struct i2c_client *client = camdev_data.i2c_client;
	struct device *dev = &client->dev;
	struct i2c_client *cli = camdev_data.rclient0, *cli1 = camdev_data.rclient1;
	
	rx_read_reg(0x1e, &v, exit0);

	pr_info("camdev[max9288]: readout rx-chip id: 0x%02x\n", v);

	return 0;

exit0:
	pr_info("camdev[max9288]: chipid read fail in func 'checkid()'\n");
	return -1;
}
#endif

static int video_detect(void)
{
#if 1
	u8 v;
	struct i2c_client *cli = camdev_data.rclient0, *cli1 = camdev_data.rclient1;

	rx_read_reg(0x04, &v, exit0);

	if(0 == (v & 0x80)) // video loss
	{
		pr_info("camdev[max9288] video loss, do video off -> on[max9275,reg04:0x47->0x87]\n");
		tx_write_reg(0x04, 0x47, exit1);
		mdelay(10);
		tx_write_reg(0x04, 0x87, exit0);
		mdelay(10);

		rx_read_reg(0x04, &v, exit0);

		if(0 == (v & 0x80)) // video still loss
		{
			pr_info("camdev[max9288] video still loss after max9275 video link off -> on\n");
			goto exit0;
		}
	}

	pr_info("camdev[max9288] video detected\n");

	return 0;

exit1:
	mdelay(10);
	tx_write_reg(0x04, 0x87, exit0);
	mdelay(10);

exit0:
	return -1;
#else
	return -1;
#endif
}

static int init_device(void)
{
	int i, j;
	u8 r, v;
	const camera_info_t *ci = camdev_data.ci;
	const cmd_set_t *cmd = ci->cmd;
	struct i2c_client *cli = camdev_data.rclient0, *cli1 = camdev_data.rclient1;
	
	for(i = 0; i < ci->cmd_count; i++)
	{
		r = (u8)cmd->reg;
		v = (u8)cmd->val;

		switch(cmd->c)
		{
			case RXR:

				rx_read_reg(r, &v, exit0);
				pr_info("camdev: mipi-rx rd[0x%02x] = 0x%02x\n", r, v);
				break;

			case RXW:

				rx_write_reg(r, v, exit0);
				//pr_info("mipi-rx wr[0x%02x] = 0x%02x\n", r, v);
				break;

			case TXR:

				tx_read_reg(r, &v, exit0);
				pr_info("camdev: mipi-tx rd[0x%02x] = 0x%02x\n", r, v);
				break;

			case TXW:

				tx_write_reg(r, v, exit0);
				//pr_info("mipi-tx wr[0x%02x] = 0x%02x\n", r, v);
				break;

			case TXTW:

				//pr_info("mipi-tx-table wr from 0x%02x, count %d\n", r, (int)sizeof(max96705_data[v]));

				for(j = 0; j < sizeof(max96705_data[v]); j++)
				{
					tx_write_reg(r + j, max96705_data[v][j], exit0);
				}
				
				break;

			case DLY:

				//pr_info("mipi config sleep %dms\n", r);
				msleep(r);
				break;

			default:

				pr_err("camdev: mipi config cmd not support: 0x%02x 0x%02x 0x%02x\n",
					cmd->c, r, v);
				goto exit0;
		}

		cmd++;
	}

	pr_info("camdev: %s: max9288-%s(%s) width = %d height = %d mipi config success\n", __func__, ci->tx_chip,ci->id,(int)ci->width,(int)ci->height);

	return 0;

exit0:
	return -1;
}

static int camdev_chip_ident(struct v4l2_subdev *sd, struct v4l2_dbg_chip_ident *chip)
{
	const camera_info_t *ci = camdev_data.ci;
	
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
static struct semaphore sem;
static int camdev_s_power(struct v4l2_subdev *sd, int on)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camdev *sensor = to_camdev(client);

	if(!on)
	{
		pr_info("camdev: release sema, process '%s', id %d\n", current->comm, current->pid);
		up(&sem);
	}
	else
	{
		pr_info("camdev: request sema... process '%s', id %d\n", current->comm, current->pid);
		down(&sem);
		pr_info("camdev: acquire sema, process '%s', id %d\n", current->comm, current->pid);

		if(-1 == video_detect())
		{
			camdev_reset();
			init_device();
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
	const camera_info_t *ci = camdev_data.ci;
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
	const camera_info_t *ci = camdev_data.ci;
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
	const camera_info_t *ci = camdev_data.ci;
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
	if (code->pad || code->index > 0)
		return -EINVAL;
	
	code->code = camdev_data.ci->cs.code;
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
	const camera_info_t *ci = camdev_data.ci;
	
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
	const camera_info_t *ci = camdev_data.ci;

	if (fie->index < 0 || fie->index > 0)
		return -EINVAL;

	if (fie->width == 0 || fie->height == 0 ||
	    fie->code == 0) {
		pr_warning("Please assign pixel format, width and height.\n");
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

static const cmd_set_t f202_sv_1520x720[] =
{
	{RXW, 0x05, 0x29}, // equalizer boost gain, 5.2db -> 10.7db
	{RXW, 0x02, 0x1f}, // disable AUDIOEN=0
	{RXW, 0x60, 0x00}, // VC=0 INPUTBW=1 YUV422-8bit
	{DLY, 20, 0},
	{RXW, 0x65, 0x37}, // DE_SEL=1 CSI DATALANE enable(D0~D3)
	{DLY, 20, 0},
	{RXW, 0x17, 0x1e}, // BSW, must be compatible with TX side(i2c, video link control), default=0x1d
	{DLY, 20, 0},
	{RXR, 0x04, 0}, // check LOCK bit 7 set?
	{RXW, 0x09, 0xc0}, // VSYNC_OUT=1 AUTOPPL=1
	{DLY, 20, 0}
};

static const cmd_set_t f202_sv_720p[] =
{
	{RXW, 0x02, 0x1f}, // disable AUDIOEN=0
	{RXW, 0x60, 0x00}, // VC=0 INPUTBW=1 YUV422-8bit
	{DLY, 20, 0},
	{RXW, 0x65, 0x37}, // DE_SEL=1 CSI DATALANE enable(D0~D3)
	{DLY, 20, 0},
	{RXW, 0x17, 0x1e}, // BSW, must be compatible with TX side(i2c, video link control), default=0x1d
	{DLY, 20, 0},
	{RXR, 0x04, 0}, // check LOCK bit 7 set?
	{RXW, 0x09, 0xc0}, // VSYNC_OUT=1 AUTOPPL=1
	{DLY, 20, 0}
};

static const cmd_set_t dayun_720p[] =
{
	{RXW, 0x04, 0x00}, // disable forwarc control channel
	{RXW, 0x17, 0x1c}, // change BWS to low
	{RXW, 0x04, 0x07}, // enable foward control channel
	{RXW, 0x1c, 0xb6}, // enable i2c local acknowledge
	{TXW, 0x04, 0x47}, // set config link
	{RXW, 0x1c, 0x36}, // disable i2c lock acknowledge
	{TXW, 0x07, 0x80}, // DBL=1
	{TXTW, 0x20, 2}, // table write, index 2.
	{TXW, 0x0f, 0xbf}, // set GPO to H, enable sensor output
	{TXW, 0x67, 0xc4}, // alignment to HS
	{TXW, 0x04, 0x87}, // enable video link
	{RXW, 0x02, 0x0f},
	{RXW, 0x60, 0x23},
	{RXW, 0x65, 0x77},
//	{RXW, 0x14, 0x80}, // 0x80: invert vs,  0x00: not invert vs(default)
	{RXW, 0x09, 0xc0} // VSYNC_OUT=1 AUTOPPL=1
};

static const cmd_set_t byd_720p[] =
{
	{RXW, 0x04, 0x00}, // disable forwarc control channel
	{RXW, 0x17, 0x1c}, // change BWS to low
	{RXW, 0x04, 0x07}, // enable foward control channel
	{RXW, 0x1c, 0xb6}, // enable i2c local acknowledge
	{TXW, 0x04, 0x47}, // set config link
	{RXW, 0x1c, 0x36}, // disable i2c lock acknowledge
	{TXW, 0x07, 0x80}, // DBL=1
	{TXTW, 0x20, 2}, // table write, index 2. for new byd camera firmware(20190117), prev use 0(20190102), old use 2
	{TXW, 0x0f, 0xbf}, // set GPO to H, enable sensor output
	{TXW, 0x67, 0xc4}, // alignment to HS
	{TXW, 0x04, 0x87}, // enable video link
	{RXW, 0x02, 0x0f},
	{RXW, 0x60, 0x23},
	{RXW, 0x65, 0x77},
//	{RXW, 0x14, 0x80}, // 0x80: invert vs,  0x00: not invert vs(default)
	{RXW, 0x09, 0xc0} // VSYNC_OUT=1 AUTOPPL=1
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
		.ident = "max9288",
		.width = 1280,
		.height = 720,
		.fps = 25,
		.pix_fmt = V4L2_PIX_FMT_UYVY,
		.hs_setting = 6,
		.cs = {.code = MEDIA_BUS_FMT_UYVY8_2X8, .colorspace = V4L2_COLORSPACE_JPEG},
		.cmd = dayun_720p,
		.cmd_count = sizeof(dayun_720p) / sizeof(dayun_720p[0])
	},

	{
		.id = "byd-720p",
		.tx_chip = "max96705",
		.ident = "max9288",
		.width = 1280,
		.height = 720,
		.fps = 30,
		.pix_fmt = V4L2_PIX_FMT_UYVY,
		.hs_setting = 6,
		.cs = {.code = MEDIA_BUS_FMT_UYVY8_2X8, .colorspace = V4L2_COLORSPACE_JPEG},
		.cmd = byd_720p,
		.cmd_count = sizeof(byd_720p) / sizeof(byd_720p[0])
	},

	{
		.id = "f202-sv-1520x720",
		.tx_chip = "max9275",
		.ident = "max9288",
		.width = 1520,
		.height = 720,
		.fps = 30,
		.pix_fmt = V4L2_PIX_FMT_RGB24,
		.hs_setting = 0,/* 6 is not ok, captured frame edge not align */
		.cs = {.code = MEDIA_BUS_FMT_RGB888_1X24, .colorspace = V4L2_COLORSPACE_SRGB},
		.cmd = f202_sv_1520x720,
		.cmd_count = sizeof(f202_sv_1520x720) / sizeof(f202_sv_1520x720[0])
	},

	{
		.id = "f202-sv-720p",
		.tx_chip = "max9275",
		.ident = "max9288",
		.width = 1280,
		.height = 720,
		.fps = 30,
		.pix_fmt = V4L2_PIX_FMT_RGB24,
		.hs_setting = 6,
		.cs = {.code = MEDIA_BUS_FMT_RGB888_1X24, .colorspace = V4L2_COLORSPACE_SRGB},
		.cmd = f202_sv_720p,
		.cmd_count = sizeof(f202_sv_720p) / sizeof(f202_sv_720p[0])
	},

	{
		.id = NULL
	}
};

static int camdev_serializer_detect(struct i2c_client *cli, struct i2c_client *cli1, const char **id)
{
	u8 r0, r1, r2, rid;

	pr_info("camdev: camera type using auto-detect mode\n");
	
	/******** detect max96705 ********/
	// ref dayun_720p array

	// backup register content
	rx_read_reg(0x04, &r0, exit0);
	rx_read_reg(0x17, &r1, exit0);
	rx_read_reg(0x1c, &r2, exit0);

	rx_write_reg(0x04, 0x00, exit0);
	rx_write_reg(0x17, 0x1c, exit0);
	rx_write_reg(0x04, 0x07, exit0);
	rx_write_reg(0x1c, 0xb6, exit0);

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
		*id = "f202-sv-720p";
	}

	// restore register content
	rx_write_reg(0x1c, r2, exit0);
	rx_write_reg(0x17, r1, exit0);
	rx_write_reg(0x04, r0, exit0);

	return 0;

exit0:
	return -1;
}

/* add sysfs interface */
static ssize_t camdev_reload_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	if(count > 4)
	{
		pr_info("camdev: sysfs config value count %d exceed 4-byte, skip\n", (int)count);

		goto exit0;
	}

	camdev_reset();

	if(init_device() < 0)
	{
		goto exit0;
	}

	return count;

exit0:
	return -1;
}

static DEVICE_ATTR(reload, S_IWUSR, NULL, camdev_reload_store);

static struct attribute *camdev_attrs[] = {
	&dev_attr_reload.attr,
	NULL
};

static const struct attribute_group camdev_attr_group = {
	.attrs = camdev_attrs,
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
	int retval;
	const camera_info_t *ci = NULL;
	struct i2c_client *cli = real_client, *cli1;
	const char *camera_id = camdev_data.dts->camera_id;
	
	sema_init(&sem, 1);

	cli1 = i2c_new_dummy(client->adapter, 0x40);

	if(NULL == cli1)
	{
		dev_err(dev, "error: slave i2c device adpter create fail[3rd]\n");
		return -ENOMEM;
	}

	if(!strcmp(camera_id, "auto"))
	{
		if(-1 == camdev_serializer_detect(cli, cli1, &camera_id))
		{
			return -ENODEV;
		}
	}

	for(ci = &camera_list[0]; (size_t)ci->id; ci++)
	{
		if(!strcmp(camera_id, ci->id))
		{
			break;
		}
	}

	if(NULL == ci->id)
	{
		dev_err(dev, "camdev error: no valid mipi-camera info found\n");
		return -ENODEV;
	}

	camdev_data.ci = ci;
	
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

	camdev_data.io_init = camdev_reset;
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

	retval = init_device();
	if (retval < 0) {
		clk_disable_unprepare(camdev_data.sensor_clk);
		pr_warning("camdev error: camdev init fail\n");
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
		return retval;

	retval = v4l2_async_register_subdev(sd);
	if (retval < 0) {
		dev_err(dev, "camdev error: %s--Async register failed, ret=%d\n", __func__, retval);
		media_entity_cleanup(&sd->entity);
		return retval;
	}

	clk_disable_unprepare(camdev_data.sensor_clk);

	retval = sysfs_create_group(&dev->kobj, &camdev_attr_group);
	if (retval) {
		dev_err(dev, "camdev error: Failure %d creating sysfs group\n", retval);
		return retval;
	}

	pr_info("camdev: %s using %s, is found\n",
		"max9288",
		camdev_data.mipi_csi ? "MIPI" : "BT656");

	return retval;
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

int max9288_driver_register(dtsi_t *dts, pf_camdev_probe *probe, pf_camdev_remove *remove)
{
	/* Set initial values for the sensor struct. */
	memset(&camdev_data, 0, sizeof(camdev_data));

	camdev_data.dts = dts;

	// do hardware reset
	camdev_reset();

	*probe = camdev_probe;
	*remove = camdev_remove;

	return 0;
}

