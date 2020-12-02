#ifndef FRAMEBUFFER_H__
#define FRAMEBUFFER_H__ 

#include <linux/fb.h>

struct myframedev_desc {
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	long int screen_size;
	int f1;
	char *buf;
};

int framebuffer_init(struct myframedev_desc *myframedev);
void framebuffer_close(struct myframedev_desc *myframedev);
void image_display(struct myframedev_desc *myframedev,char *img,int h,int w);
void hua(struct myframedev_desc *myframedev);
void black_screen(struct myframedev_desc *myframedev);
void draw_framebuffer(struct myframedev_desc *myframedev, unsigned char* src, int width, int height);

#endif
