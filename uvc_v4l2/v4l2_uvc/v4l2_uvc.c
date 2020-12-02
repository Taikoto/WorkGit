#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <limits.h>
#include "draw_framebuffer.h"
#include "v4l2_uvc.h"
#include "libyuv.h"

#define FRAME_NUM		 4
#define IMAGE_H		     480
#define IMAGE_W			 640
#define NUMERATOR		 1
#define DENOMINATOR      30
#define DEV              "/dev/video9"
#define FALSE            ((void *)-1)


static void yuv422_to_rgb888(const unsigned char *im_yuv, unsigned char *dst, int width, int height) 
{
    const int IM_SIZE = width * height;
    unsigned char Y = 0;
    unsigned char U = 0;
    unsigned char V = 0;
    int B = 0;
    int G = 0;
    int R = 0;
    int i;
	
    for(i = 0; i < IM_SIZE; ++i) {
        if(!(i & 1)) {
            U = im_yuv[2 * i + 1];
            V = im_yuv[2 * i + 3];
        }
		
        Y = im_yuv[2 * i];
        B = Y + 1.772 * (U - 128);
        G = Y - 0.34414 * (U - 128) - (0.71414 * (V - 128));
        R = Y + 1.402 * (V - 128);
        if(B > UCHAR_MAX) {
            B = UCHAR_MAX;
        }
		
        if(G > UCHAR_MAX) {
            G = UCHAR_MAX;
        }
		
        if(R > UCHAR_MAX) {
            R = UCHAR_MAX;
        }
		
        dst[3*i] = B;
        dst[3*i+1] = G;
        dst[3*i+2] = R;
    }
}

static void libyuv_yuyv2argb(char *yuyv, char *argb, int w, int h)
{
	YUY2ToARGB((const uint8_t*)yuyv, IMAGE_W * 2, (uint8_t *)argb, 4 * w, w, h);
}

static void argb2rgb24(char *rgb32, char *rgb24, int w, int h)
{
	int i,j = 0;
	
	for(i = 0;i < w * h;i ++) {
		*(rgb24 + j * 3) = *(rgb32 + i * 4);
		*(rgb24 + j * 3 + 1) = *(rgb32 + i * 4 + 1);
		*(rgb24 + j * 3 + 2) = *(rgb32 + i * 4 + 2);
		j ++;
	}
}

static void rgb888_to_rgb32(char *rgb888, char *rgb32, int w, int h)
{
	unsigned int size = w * h;
	int i;
	
	for(i = 0;i < size;i ++) {
		*(rgb32 + i * 4) = *(rgb888 + i * 3);
		*(rgb32 + i * 4 + 1) = *(rgb888 + i * 3 + 1);
		*(rgb32 + i * 4 + 2) = *(rgb888 + i * 3 + 2);
		*(rgb32 + i * 4 + 3) = 0;
	}
}

static int video_cap_init(struct dev_desc *dev)
{
	//get video capabilities
	if(ioctl(dev->f, VIDIOC_QUERYCAP, &dev->cap) < 0) {
		printf("VIDIOC_QUERYCAP er\n");
		return -1;
	}

	printf("cap.driver:%s\n",dev->cap.driver);
	printf("cap.bus_info:%s\n",dev->cap.bus_info);
	printf("cap.card:%s\n",dev->cap.card);
	if((dev->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == V4L2_CAP_VIDEO_CAPTURE) {
		printf("support V4L2_CAP_VIDEO_CAPTURE\n");
	}
	
	//show all support format
	dev->fmtdesc.index = 0;
	dev->fmtdesc.type = V4L2_CAP_VIDEO_CAPTURE;
	printf("support format:\n");
	while(ioctl(dev->f, VIDIOC_ENUM_FMT, &dev->fmtdesc) != -1) {
		printf("\t%d fmtdesc.description:%s\n",dev->fmtdesc.index,dev->fmtdesc.description);
		dev->fmtdesc.index ++;
	}
	
	dev->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	dev->fmt.fmt.pix.height = IMAGE_H;
	dev->fmt.fmt.pix.width = IMAGE_W;
	dev->fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
	dev->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	
	if(ioctl(dev->f, VIDIOC_S_FMT, &dev->fmt) < 0) {
		printf("VIDIOC_S_FMT err\n");
		return -1;
	}

	//show current frame format
	dev->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(ioctl(dev->f, VIDIOC_G_FMT, &dev->fmt) < 0) {
		printf("VIDIOC_G_FMT err\n");
		return -1;
	}

	dev->image_size = dev->fmt.fmt.pix.height * dev->fmt.fmt.pix.width * 2;
	dev->h = dev->fmt.fmt.pix.height;
	dev->w = dev->fmt.fmt.pix.width;
	printf("width:%u,height:%u\n",dev->fmt.fmt.pix.width,dev->fmt.fmt.pix.height);

	/*printf("pix.pixelformat:%c%c%c%c\n",dev->fmt.fmt.pix.pixelformat & 0xFF, (dev->fmt.fmt.pix.pixelformat >> 8) & 0xFF,(dev->fmt.fmt.pix.pixelformat >> 16) & 0xFF, (dev->fmt.fmt.pix.pixelformat >> 24) & 0xFF);*/
	
	//设置、查看 帧数率
	dev->streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	dev->streamparm.parm.capture.timeperframe.denominator = DENOMINATOR;
	dev->streamparm.parm.capture.timeperframe.numerator = NUMERATOR;
	if(ioctl(dev->f, VIDIOC_S_PARM, &dev->streamparm) < 0) {
		printf("VIDIOC_S_PARM err\n");
		return -1;
	}
	if(ioctl(dev->f, VIDIOC_G_PARM, &dev->streamparm) < 0) {
		printf("VIDIOC_G_PARM err\n");
		return -1;
	}
	printf("denominator:%d\n",dev->streamparm.parm.capture.timeperframe.denominator);
	printf("numerator:%d\n",dev->streamparm.parm.capture.timeperframe.numerator);

	return 0;
}

static int video_mem_ops(struct dev_desc *dev)
{
	int i;
	//申请帧缓冲
	dev->req.count = FRAME_NUM;
	dev->req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	dev->req.memory = V4L2_MEMORY_MMAP;
	if(ioctl(dev->f, VIDIOC_REQBUFS, &dev->req) < 0) {
		printf("VIDIOC_REQBUFS err\n");
		return -1;
	}

	//request userspace address
	dev->buffer = malloc(FRAME_NUM * sizeof(*dev->buffer));
	if(!dev->buffer) {
		printf("no memory\n");
		return -1;
	}

	//try to map addr
	for(i = 0;i < FRAME_NUM;i ++) {
		dev->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		dev->buf.index = i;
		dev->buf.memory = V4L2_MEMORY_MMAP;
		if(ioctl(dev->f, VIDIOC_QUERYBUF, &dev->buf) < 0) {
			printf("VIDIOC_QUERYBUF err\n");
			return -1;
		}
		//map
		dev->buffer[i].length = dev->buf.length;
		dev->buffer[i].start = mmap(NULL, dev->buf.length, PROT_WRITE | PROT_READ, MAP_SHARED, dev->f, dev->buf.m.offset);
		if(dev->buffer[i].start == MAP_FAILED) {
			printf("addr mmap failed\n");
			return -1;
		}
	}
	
	return 0;
}

/*static void video_file_name(int i, int j, char *fname, char *hz)
{
	char t1[] = "./workspace";
	char t3[5] = "0";
	unsigned int num = i * 4 + j;
	char t2[64] = "0";
	memcpy(t3, hz, sizeof(hz));
	strcpy(fname, t1);
	sprintf(t2, "%d", num);
	strcat(fname, t2);
	strcat(fname, t3);
}*/

static int v4l2_app_start(struct dev_desc *dev)
{
	int i;
	
	//入队
	for(i = 0;i < FRAME_NUM;i ++) {
		dev->buf.index = i;
		if(ioctl(dev->f, VIDIOC_QBUF, &dev->buf) < 0) {
			printf("%d VIDIOC_QBUF err\n",i);
			return -1;
		}
	}
	//start
	dev->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(ioctl(dev->f, VIDIOC_STREAMON ,&dev->type) < 0) {
		printf("VIDIOC_STREAMON err\n");
		return -1;
	}
	
	return 0;
}

void video_process(struct dev_desc *dev,int flags)
{
	//出队
	if(ioctl(dev->f, VIDIOC_DQBUF ,&dev->buf) < 0) {
		printf("VIDIOC_DQBUF err\n");
		return ;
	}
	
	if(flags == LIBYUV) {
		libyuv_yuyv2argb(dev->buffer[dev->buf.index].start, dev->rgb32, dev->w, dev->h);
	}
	
	memcpy(dev->yuyv, dev->buffer[dev->buf.index].start, dev->w * dev->h * 2);
	
	if(ioctl(dev->f, VIDIOC_QBUF, &dev->buf) < 0) {
		printf("VIDIOC_QBUF err %d\n",__LINE__);
		return ;
	}
}

void v4l2_app_close(struct dev_desc *dev)
{
	int i;
	
	if(ioctl(dev->f, VIDIOC_STREAMOFF, &dev->type) < 0) {
		printf("VIDIOC_STREAMOFF err\n");
		return ;
	}
	for(i = 0;i < FRAME_NUM;i ++) {
		if(munmap(dev->buffer[i].start, dev->buffer[i].length) < 0) {
			printf("munmap err\n");
			return ;
		}
	}
	
	free(dev->buffer);
	free(dev->rgb32);
	free(dev->rgb888);
	free(dev->yuyv);
	close(dev->f);
	printf("uvc close\n");
}

int v4l2_app_init(struct dev_desc *dev)
{
	dev->f = open(DEV, O_RDWR);
	if(!dev->f) {
		printf("v4l2 open err\n");
		return -1;
	}
	
	video_cap_init(dev);
	video_mem_ops(dev);
	v4l2_app_start(dev);
	dev->rgb888 = (char *)malloc(dev->h * dev->w * 3);
	dev->rgb32 = (char *)malloc(dev->h * dev->w * 4);
	dev->yuyv = (char *)malloc(dev->h * dev->w * 2);
	
	return 0;
}
