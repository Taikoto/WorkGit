/*
 * Copyright 2017 NXP
 */
/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/of_device.h>
#include <linux/i2c.h>
#include <linux/v4l2-mediabus.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>
#include <media/v4l2-subdev.h>
#include <mipi_multiplexer.h>


#define CAMDEV_OUT_WIDTH			1280
#define CAMDEV_OUT_HEIGHT		720
#define CAMDEV_FMT_CODE			MEDIA_BUS_FMT_YUYV8_1X16
#define CAMDEV_FMT_CS			V4L2_COLORSPACE_JPEG

// fixed definitions
#define MIPI_CSI2_SENS_VC0_PAD_SOURCE	0
#define MIPI_CSI2_SENS_VC1_PAD_SOURCE	1
#define MIPI_CSI2_SENS_VC2_PAD_SOURCE	2
#define MIPI_CSI2_SENS_VC3_PAD_SOURCE	3
#define MIPI_CSI2_SENS_VCX_PADS_NUM		4

/*!
 * Maintains the information on the current state of the sesor.
 */
struct imxdpu_videomode {
	char name[64];		/* may not be needed */

	uint32_t pixelclock;	/* Hz */

	/* htotal (pixels) = hlen + hfp + hsync + hbp */
	uint32_t hlen;
	uint32_t hfp;
	uint32_t hbp;
	uint32_t hsync;

	/* field0 - vtotal (lines) = vlen + vfp + vsync + vbp */
	uint32_t vlen;
	uint32_t vfp;
	uint32_t vbp;
	uint32_t vsync;

	/* field1  */
	uint32_t vlen1;
	uint32_t vfp1;
	uint32_t vbp1;
	uint32_t vsync1;

	uint32_t flags;

	uint32_t format;
	uint32_t dest_format; /*buffer format for capture*/

	int16_t clip_top;
	int16_t clip_left;
	uint16_t clip_width;
	uint16_t clip_height;

};

enum cam_config
{
	CFG_TVI_720P_30 = 0,
	CFG_TVI_720P_25
};

struct sensor_data {
	struct v4l2_subdev	subdev;
	struct media_pad pads[MIPI_CSI2_SENS_VCX_PADS_NUM];
	struct i2c_client *i2c_client;
	struct v4l2_mbus_framefmt format;
	struct v4l2_captureparm streamcap;
	char running;

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
	int v_channel;
	bool is_mipi;
	struct imxdpu_videomode cap_mode;

	unsigned int sensor_num;       /* sensor num connect max9271 */
	unsigned char sensor_is_there; /* Bit 0~3 for 4 cameras, 0b1= is there; 0b0 = is not there */

	/* for mipi-multiplexer driver */
	struct i2c_client *slave_client0;
	u32 camcfg;
};

static inline int camdev_get_fps(struct sensor_data *camdev_data)
{
	u32 cfg = camdev_data->camcfg;

	if(CFG_TVI_720P_25 == cfg)
	{
		return 25;
	}
	else if(CFG_TVI_720P_30 == cfg)
	{
		return 30;
	}

	return 0;
}

static inline struct sensor_data *subdev_to_sensor_data(struct v4l2_subdev *sd)
{
	return container_of(sd, struct sensor_data, subdev);
}

static int camdev_hardware_preinit(struct sensor_data *camdev_data)
{
	struct i2c_client *cli = camdev_data->slave_client0;

	//video setting
	camdev_write_reg(0x40, 0x04, exit0); //video page, write all
	
	camdev_write_reg(0x02, 0xc2, exit0);
	camdev_write_reg(0x07, 0xc0, exit0);
	camdev_write_reg(0x0b, 0xc0, exit0);
	camdev_write_reg(0x0c, 0x13, exit0);
	camdev_write_reg(0x0d, 0x50, exit0);

	camdev_write_reg(0x11, 0x50, exit0); // update 4-ch contrast, default 0x40

	camdev_write_reg(0x15, 0x13, exit0);
	camdev_write_reg(0x16, 0x15, exit0);
	camdev_write_reg(0x17, 0x00, exit0);
	camdev_write_reg(0x18, 0x19, exit0);
	camdev_write_reg(0x19, 0xd0, exit0);
	camdev_write_reg(0x1a, 0x25, exit0);
	
	if(30 == camdev_get_fps(camdev_data))
	{
		camdev_write_reg(0x1c, 0x06, exit0);  //1280*720, 30/60fps
		camdev_write_reg(0x1d, 0x72, exit0);  //1280*720, 30/60fps
	}
	else if(25 == camdev_get_fps(camdev_data))
	{
		camdev_write_reg(0x1c, 0x07, exit0);  //1280*720, 25/50fps
		camdev_write_reg(0x1d, 0xbc, exit0);  //1280*720, 25/50fps
	}
	
	camdev_write_reg(0x20, 0x30, exit0);
	camdev_write_reg(0x21, 0x84, exit0);
	camdev_write_reg(0x22, 0x36, exit0);
	camdev_write_reg(0x23, 0x3c, exit0);
	
	camdev_write_reg(0x2b, 0x60, exit0);
	camdev_write_reg(0x2c, 0x0a, exit0);
	camdev_write_reg(0x2d, 0x30, exit0);
	camdev_write_reg(0x2e, 0x70, exit0);

	camdev_write_reg(0x30, 0x48, exit0);
	camdev_write_reg(0x31, 0xbb, exit0);
	camdev_write_reg(0x32, 0x2e, exit0);
	camdev_write_reg(0x33, 0x90, exit0);
	
	camdev_write_reg(0x35, 0x25, exit0);
	camdev_write_reg(0x38, 0x4e, exit0);
	camdev_write_reg(0x3d, 0x40, exit0);
	camdev_write_reg(0x3e, 0x80, exit0);
	camdev_write_reg(0x39, 0x18, exit0);
	
	camdev_write_reg(0x4e, 0x00, exit0);
	camdev_write_reg(0xf5, 0xf0, exit0);

	//mipi setting
	camdev_write_reg(0x40, 0x08, exit0); //MIPI page 
	camdev_write_reg(0x01, 0xf8, exit0);
	camdev_write_reg(0x02, 0x01, exit0);
	camdev_write_reg(0x08, 0x0f, exit0);
	camdev_write_reg(0x13, 0x24, exit0);
	camdev_write_reg(0x14, 0x04, exit0);
	camdev_write_reg(0x15, 0x00, exit0);
	camdev_write_reg(0x20, 0x44, exit0);
	camdev_write_reg(0x34, 0x1b, exit0);

	/* Disable MIPI CSI2 output */
	//camdev_write_reg(0x23, 0x02, exit0);

	msleep(5);

	return 0;

exit0:
	return -1;
}

static int camdev_hardware_init(struct sensor_data *camdev_data)
{
	struct i2c_client *cli = camdev_data->slave_client0;

	camdev_data->running = 0;

	/* Disable CSI Output */
	camdev_write_reg(0x23, 0x02, exit0);

	return 0;

exit0:
	return -1;
}

static int camdev_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *a)
{
	struct v4l2_captureparm *cparm = &a->parm.capture;
	struct sensor_data *camdev_data = subdev_to_sensor_data(sd);
	int ret = 0;

	switch (a->type) {
	/* This is the only case currently handled. */
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
		memset(a, 0, sizeof(*a));
		a->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		cparm->capability = camdev_data->streamcap.capability;
		cparm->timeperframe = camdev_data->streamcap.timeperframe;
		cparm->capturemode = camdev_data->streamcap.capturemode;
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

/*!
 * ioctl_s_parm - V4L2 sensor interface handler for VIDIOC_S_PARM ioctl
 * @s: pointer to standard V4L2 device structure
 * @a: pointer to standard V4L2 VIDIOC_S_PARM ioctl structure
 *
 * Configures the sensor to use the input parameters, if possible.  If
 * not possible, reverts to the old parameters and returns the
 * appropriate error code.
 */
static int camdev_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *a)
{
	struct sensor_data *camdev_data = subdev_to_sensor_data(sd);
	struct v4l2_fract *timeperframe = &a->parm.capture.timeperframe;
	int ret = 0;

	switch (a->type) {
	/* This is the only case currently handled. */
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:

		timeperframe->denominator = camdev_get_fps(camdev_data);
		timeperframe->numerator = 1;

		 /* TODO Reserved to extension */
		camdev_data->streamcap.timeperframe = *timeperframe;
		camdev_data->streamcap.capturemode = a->parm.capture.capturemode;
		camdev_data->format.width = CAMDEV_OUT_WIDTH;
		camdev_data->format.height = CAMDEV_OUT_HEIGHT;

		break;

	/* These are all the possible cases. */
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	case V4L2_BUF_TYPE_VBI_CAPTURE:
	case V4L2_BUF_TYPE_VBI_OUTPUT:
	case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
	case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
		pr_debug("   type is not "\
				 "V4L2_BUF_TYPE_VIDEO_CAPTURE but %d\n",
			a->type);
		ret = -EINVAL;
		break;

	default:
		pr_debug("   type is unknown - %d\n", a->type);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int camdev_enum_mbus_code(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_mbus_code_enum *code)
{
	struct sensor_data *camdev_data = subdev_to_sensor_data(sd);

	code->code = camdev_data->format.code;
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
	struct sensor_data *camdev_data = subdev_to_sensor_data(sd);

	if (fse->index > 1)
		return -EINVAL;

	fse->max_width = camdev_data->format.width;
	fse->min_width = fse->max_width;

	fse->max_height = camdev_data->format.height;
	fse->min_height = fse->max_height;
	return 0;
}
static int camdev_enum_frame_interval(struct v4l2_subdev *sd,
				   struct v4l2_subdev_pad_config *cfg,
				   struct v4l2_subdev_frame_interval_enum *fie)
{
	struct sensor_data *camdev_data = subdev_to_sensor_data(sd);

	if (fie->index < 0 || fie->index > 8)
		return -EINVAL;

	if (fie->width == 0 || fie->height == 0 ||
	    fie->code == 0) {
		pr_warning("Please assign pixel format, width and height.\n");
		return -EINVAL;
	}

	fie->interval.numerator = 1;

	 /* TODO Reserved to extension */

	fie->interval.denominator = camdev_get_fps(camdev_data);
	return 0;
}

static int camdev_get_fmt(struct v4l2_subdev *sd,
			   struct v4l2_subdev_pad_config *cfg,
			   struct v4l2_subdev_format *fmt)
{
	struct sensor_data *camdev_data = subdev_to_sensor_data(sd);
	struct v4l2_mbus_framefmt *mf = &fmt->format;

	if (fmt->pad)
		return -EINVAL;

	mf->code = camdev_data->format.code;
	mf->width =  camdev_data->format.width;
	mf->height = camdev_data->format.height;
	mf->colorspace = camdev_data->format.colorspace;
	mf->field = camdev_data->format.field;
	mf->reserved[0] = camdev_data->format.reserved[0];

	return 0;
}

static int camdev_set_fmt(struct v4l2_subdev *sd,
			 struct v4l2_subdev_pad_config *cfg,
			 struct v4l2_subdev_format *fmt)
{
	struct sensor_data *camdev_data = subdev_to_sensor_data(sd);
	struct v4l2_mbus_framefmt *mf = &fmt->format;

	if (fmt->pad)
		return -EINVAL;

	mf->code = camdev_data->format.code;
	mf->colorspace = camdev_data->format.colorspace;
	mf->field = V4L2_FIELD_NONE;

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY)
		return 0;

	return 0;
}

static int camdev_get_frame_desc(struct v4l2_subdev *sd, unsigned int pad,
				  struct v4l2_mbus_frame_desc *fd)
{
	return 0;
}

static int camdev_set_frame_desc(struct v4l2_subdev *sd,
					unsigned int pad,
					struct v4l2_mbus_frame_desc *fd)
{
	return 0;
}

static int camdev_chip_ident(struct v4l2_subdev *sd, struct v4l2_dbg_chip_ident *chip)
{
//	const camera_info_t *ci = to_camera_info();
	
//	((struct v4l2_dbg_chip_ident *)id)->match.type = V4L2_CHIP_MATCH_SUBDEV;//V4L2_CHIP_MATCH_I2C_DRIVER;

	strcpy(chip->match.name, "tp2854-4ch-hd");

	return 0;
}

static int camdev_set_power(struct v4l2_subdev *sd, int on)
{
	return 0;
}

static int camdev_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_data *camdev_data = subdev_to_sensor_data(sd);
	struct i2c_client *cli = camdev_data->slave_client0;

	dev_dbg(sd->dev, "%s\n", __func__);
	if (enable) {
		if (!camdev_data->running) {
			/* Enable CSI output, set virtual channel according to the link number */
			camdev_write_reg(0x23, 0x00, exit0);
			pr_info("##camdev: CSI output disable\n");
		}
		camdev_data->running++;

	} else {

		if (camdev_data->running) {
			/* Disable CSI Output */
			camdev_write_reg(0x23, 0x02, exit0);
			pr_info("##camdev: CSI output enable\n");
		}
		camdev_data->running--;
	}

	return 0;

exit0:
	return -EIO;
}

static int camdev_link_setup(struct media_entity *entity,
			   const struct media_pad *local,
			   const struct media_pad *remote, u32 flags)
{
	return 0;
}

static const struct v4l2_subdev_pad_ops camdev_pad_ops = {
	.enum_mbus_code = camdev_enum_mbus_code,
	.enum_frame_size = camdev_enum_framesizes,
	.enum_frame_interval = camdev_enum_frame_interval,
	.get_fmt = camdev_get_fmt,
	.set_fmt = camdev_set_fmt,
	.get_frame_desc = camdev_get_frame_desc,
	.set_frame_desc = camdev_set_frame_desc,
};

static const struct v4l2_subdev_core_ops camdev_core_ops = {
	.g_chip_ident = camdev_chip_ident,
	.s_power	= camdev_set_power,
};

static const struct v4l2_subdev_video_ops camdev_video_ops = {
	.s_parm =	camdev_s_parm,
	.g_parm =	camdev_g_parm,
	.s_stream = camdev_s_stream,
};

static const struct v4l2_subdev_ops camdev_subdev_ops = {
	.core	= &camdev_core_ops,
	.pad	= &camdev_pad_ops,
	.video = &camdev_video_ops,
};

static const struct media_entity_operations camdev_sd_media_ops = {
	.link_setup = camdev_link_setup,
};

/* add sysfs interface */
static ssize_t camdev_reload_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct sensor_data *camdev_data = subdev_to_sensor_data(sd);

	if(count > 4)
	{
		pr_info("camdev: sysfs config value count %d exceed 4-byte, skip\n", (int)count);

		goto exit0;
	}

	if(camdev_hardware_preinit(camdev_data) < 0)
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
	struct device *dev = &client->dev;
	struct sensor_data *camdev_data;
	struct v4l2_subdev *sd;
	int retval;
	const char *camera_id;

	camdev_data = devm_kzalloc(dev, sizeof(*camdev_data), GFP_KERNEL);
	retval = (NULL == camdev_data) ? -ENOMEM : 0;
	if(retval) {
		dev_err(dev, "error: camdev_data alloc fail\n");
		goto exit0;
	}

	if(of_property_read_string_index(dev->of_node, "camera_id", 0, &camera_id))
	{
		dev_err(dev, "camdev error: no camer_id config available\n");
		goto exit0;
	}

	if(!strcmp(camera_id, "tvi-720p@25x4"))
	{
		camdev_data->camcfg = CFG_TVI_720P_25;
	}
	else if(!strcmp(camera_id, "tvi-720p@30x4"))
	{
		camdev_data->camcfg = CFG_TVI_720P_30;
	}
	else
	{
		dev_err(dev, "camdev error: camer_id '%s' is not supported!\n", camera_id);
		goto exit0;
	}

	camdev_data->slave_client0 = real_client;

	/* Set initial values for the sensor struct. */
	camdev_data->sensor_clk = devm_clk_get(dev, "csi_mclk");
	retval = IS_ERR(camdev_data->sensor_clk) ? -ENODEV : 0; 
	if(retval) {
		/* assuming clock enabled by default */
		camdev_data->sensor_clk = NULL;
		dev_err(dev, "error: clock-frequency missing or invalid\n");
		goto exit0;
	}

	retval = of_property_read_u32(dev->of_node, "mclk", &(camdev_data->mclk));
	if (retval) {
		dev_err(dev, "error: mclk missing or invalid\n");
		goto exit0;
	}

	retval = of_property_read_u32(dev->of_node, "mclk_source",
		(u32 *)&(camdev_data->mclk_source));
	if (retval) {
		dev_err(dev, "error: mclk_source missing or invalid\n");
		goto exit0;
	}

	clk_prepare_enable(camdev_data->sensor_clk);

	camdev_data->i2c_client = client;
	camdev_data->format.code = CAMDEV_FMT_CODE;
	camdev_data->format.width = CAMDEV_OUT_WIDTH;
	camdev_data->format.height = CAMDEV_OUT_HEIGHT;
	camdev_data->format.colorspace = CAMDEV_FMT_CS;
	/*****************************************
	 * Pass mipi phy clock rate Mbps
	 * fcsi2 = PCLk * WIDTH * CHANNELS / LANES
	 * fsci2 = 72MPCLK * 8 bit * 4 channels / 4 lanes
	 ****************************************/
	camdev_data->format.reserved[0] = 72 * 8;
	camdev_data->format.field = V4L2_FIELD_NONE;
	camdev_data->streamcap.capturemode = 0;

	camdev_data->is_mipi = 1;

	retval = camdev_hardware_preinit(camdev_data);
	if(-1 == retval)
	{
		dev_err(dev, "error: %s return fail\n", __func__);
		goto exit1;
	}
	
	/////////// for debug /////////////////
	camdev_data->sensor_num = 4;

	if(camdev_data->sensor_num == 0) {
		dev_err(dev, "error: cameras are not found,\n");
		goto exit1;
	}

	camdev_data->streamcap.capability = V4L2_CAP_TIMEPERFRAME;
	camdev_data->streamcap.timeperframe.denominator = camdev_get_fps(camdev_data);
	camdev_data->streamcap.timeperframe.numerator = 1;
	camdev_data->v_channel = 0;

	//////// cap_mode member is not used /////////////
	camdev_data->cap_mode.clip_top = 0;
	camdev_data->cap_mode.clip_left = 0;

	camdev_data->cap_mode.clip_height = CAMDEV_OUT_HEIGHT;
	camdev_data->cap_mode.clip_width = CAMDEV_OUT_WIDTH;

	camdev_data->cap_mode.hlen = camdev_data->cap_mode.clip_width;

	camdev_data->cap_mode.hfp = 0;
	camdev_data->cap_mode.hbp = 0;
	camdev_data->cap_mode.hsync = 625;
	camdev_data->cap_mode.vlen = camdev_data->cap_mode.clip_height;
	camdev_data->cap_mode.vfp = 0;
	camdev_data->cap_mode.vbp = 0;
	camdev_data->cap_mode.vsync = 40;
	camdev_data->cap_mode.vlen1 = 0;
	camdev_data->cap_mode.vfp1 = 0;
	camdev_data->cap_mode.vbp1 = 0;
	camdev_data->cap_mode.vsync1 = 0;
	camdev_data->cap_mode.pixelclock = 27000000;

	sd = &camdev_data->subdev;
	v4l2_i2c_subdev_init(sd, client, &camdev_subdev_ops);
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

	sd->entity.function = MEDIA_ENT_F_CAM_SENSOR;
	camdev_data->pads[MIPI_CSI2_SENS_VC0_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	camdev_data->pads[MIPI_CSI2_SENS_VC1_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	camdev_data->pads[MIPI_CSI2_SENS_VC2_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	camdev_data->pads[MIPI_CSI2_SENS_VC3_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	retval = media_entity_pads_init(&sd->entity, MIPI_CSI2_SENS_VCX_PADS_NUM,
							camdev_data->pads);
	if (retval < 0) {
		dev_err(dev, "error: mipi media device pad init fail\n");
		goto exit1;
	}

	camdev_data->subdev.entity.ops = &camdev_sd_media_ops;
	retval = v4l2_async_register_subdev(&camdev_data->subdev);
	if (retval < 0) {
		dev_err(dev, "error: %s--Async register failed, ret=%d\n", __func__, retval);
		goto exit2;
	}

	retval = camdev_hardware_init(camdev_data);
	if (retval < 0) {
		dev_err(dev, "error: camera init failed\n");
		goto exit3;
	}

	retval = sysfs_create_group(&dev->kobj, &camdev_attr_group);
	if (retval) {
		dev_err(dev, "camdev error: Failure %d creating sysfs group\n",
			retval);
		goto exit3;
	}

	pr_info("camdev: 4-CH mipi camera is found, name is TP2854L\n");
	
	return 0;

exit3:
	v4l2_async_unregister_subdev(sd);

exit2:
	media_entity_cleanup(&sd->entity);

exit1:
	clk_disable_unprepare(camdev_data->sensor_clk);

exit0:
	return -ENODEV;
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
	struct sensor_data *camdev_data = subdev_to_sensor_data(sd);

	sysfs_remove_group(&dev->kobj, &camdev_attr_group);

	v4l2_async_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
	clk_disable_unprepare(camdev_data->sensor_clk);

	kfree(camdev_data);

	return 0;
}

int tp2854_driver_register(dtsi_t  *dts, pf_camdev_probe *probe, pf_camdev_remove *remove)
{
#if 0
	/* Set initial values for the sensor struct. */
	memset(&camdev_data, 0, sizeof(camdev_data));

	camdev_data.dts = dts;

	// do hardware reset
	camdev_reset();
#endif

	*probe = camdev_probe;
	*remove = camdev_remove;

	return 0;
}
