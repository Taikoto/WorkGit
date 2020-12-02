#ifndef V4L2_UVC_H__
#define V4L2_UVC_H__

#include <linux/videodev2.h>

enum WAY_YUYV2RGB24 {
	NATIVE = 0,
	FFMPEG,
	myFFMPEG,
	LIBYUV,
};
struct buffer_desc {
	void *start;      
	unsigned int length;
	long long timestamp;
};
struct dev_desc {
	struct v4l2_capability cap;
	struct v4l2_fmtdesc fmtdesc;
	struct v4l2_format fmt_test;
	struct v4l2_format fmt;
	struct v4l2_streamparm streamparm;
	struct v4l2_requestbuffers req;
	struct buffer_desc *buffer;
	struct v4l2_buffer buf;
	enum v4l2_buf_type type;
	unsigned int image_size;
	int w;
	int h;
	char *rgb888;
	char *rgb32;
	char *yuyv;
	int f;
};

int v4l2_app_init(struct dev_desc *dev);
void v4l2_app_close(struct dev_desc *dev);
void video_process(struct dev_desc *dev,int flags);

#endif
