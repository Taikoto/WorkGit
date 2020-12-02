#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdio.h>
#include "draw_framebuffer.h"

//test
void hua(struct myframedev_desc *myframedev)
{
	int x = 0,y = 0;
	unsigned long int local;
	
	for(x = 0;x < 640;x ++) {
		for(y = 0;y < 480;y ++) {
			local = myframedev->vinfo.bits_per_pixel / 8 * x + myframedev->finfo.line_length * y; 
			*(myframedev->buf +  local) = 255;
			*(myframedev->buf +  local + 1) = 0;
			*(myframedev->buf +  local + 2) = 0;
			/**(myframedev->buf +  local + 3) = 255;*/
		}
	}
}

void black_screen(struct myframedev_desc *myframedev)
{
	int x = 0,y = 0;
	unsigned long int local;
	
	for(x = 0;x < 1024;x ++) {
		for(y = 0;y < 600;y ++) {
			local = myframedev->vinfo.bits_per_pixel / 8 * x + myframedev->finfo.line_length * y; 
			*(myframedev->buf +  local) = 0;
			*(myframedev->buf +  local + 1) = 0;
			*(myframedev->buf +  local + 2) = 0;
			*(myframedev->buf +  local + 3) = 0;
		}
	}
}

void image_display(struct myframedev_desc *myframedev,char *img,int w,int h)
{
	static unsigned long int frame_count = 0;
	int x,y,i = 0;
	unsigned long int local;
	int offset = 0;
	
	for(y = offset;y < h + offset;y ++) {
		for(x = offset;x < w + offset;x ++) {
			/*local = (x + myframedev->vinfo.xoffset) * (myframedev->vinfo.bits_per_pixel >> 3) + (y + myframedev->vinfo.yoffset) * myframedev->finfo.line_length;*/
			local = myframedev->vinfo.bits_per_pixel / 8 * x + myframedev->finfo.line_length * y;
			if(myframedev->vinfo.bits_per_pixel == 32){
				*(myframedev->buf +  local)     = *(img + 4 * i);
				*(myframedev->buf +  local + 1) = *(img + 4 * i + 1);
				*(myframedev->buf +  local + 2) = *(img + 4 * i + 2);
				*(myframedev->buf +  local + 3) = *(img + 4 * i + 3);;
			}else if(myframedev->vinfo.bits_per_pixel == 24){
				*(myframedev->buf +  local)     = *(img + 3 * i);
				*(myframedev->buf +  local + 1) = *(img + 3 * i + 1);
				*(myframedev->buf +  local + 2) = *(img + 3 * i + 2);
			}else
				printf("err:no such bits_per_pixel\n");

			i++;
		}
	}
	
	printf("%ld frame...\n",frame_count);
	frame_count ++;
}

void draw_framebuffer(struct myframedev_desc *myframedev, unsigned char* src, int width, int height)
{
	int x, y;
	unsigned int location = 0;
	int i = 0;
	
	for(y = 0; y < height; y++) {
		for(x = 0; x < width; x++) {
			location = (x + myframedev->vinfo.xoffset) * (myframedev->vinfo.bits_per_pixel >> 3) + (y + myframedev->vinfo.yoffset) * myframedev->finfo.line_length;
			*(myframedev->buf + location) = src[i*3];           //B
			*(myframedev->buf + location + 1) = src[i*3 + 1];	//G
			*(myframedev->buf + location + 2) = src[i*3 + 2];	//R
			i++;
		}
	}
}

int framebuffer_init(struct myframedev_desc *myframedev)
{
	myframedev->f1 = open("/dev/fb0", O_RDWR);

	if(!myframedev->f1) {
		printf("open /dev/fb0 err\n");
		return -1;
	}
	
	if(ioctl(myframedev->f1, FBIOGET_VSCREENINFO, &myframedev->vinfo) < 0) {
		printf("FBIOGET_VSCREENINFO er\n");
		return -1;
	}
	
	if(ioctl(myframedev->f1, FBIOGET_FSCREENINFO, &myframedev->finfo) < 0) {
		printf("FBIOGET_FSCREENINFO er\n");
		return -1;
	}
	
	myframedev->screen_size = myframedev->vinfo.xres * myframedev->vinfo.yres * myframedev->vinfo.bits_per_pixel / 8;
	myframedev->buf = (char *)mmap(NULL, myframedev->screen_size, PROT_WRITE | PROT_READ, MAP_SHARED, myframedev->f1, 0);
	if(myframedev->buf == MAP_FAILED) {
		printf("framebuffer mmap err\n");
		return -1;
	}

	printf("xres:%d\n",myframedev->vinfo.xres);
	printf("yres:%d\n",myframedev->vinfo.yres);
	printf("bits_per_pixel:%d\n",myframedev->vinfo.bits_per_pixel);

	return 0;
}

void framebuffer_close(struct myframedev_desc *myframedev)
{
	munmap(myframedev->buf, myframedev->screen_size);
	close(myframedev->f1);
	printf("draw_framebuffer close\n");
}
