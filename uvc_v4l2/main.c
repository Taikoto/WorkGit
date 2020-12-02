#include <linux/fb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <signal.h>
#include <errno.h>

#include "draw_framebuffer.h"
#include "v4l2_uvc.h"
/*#include "encode_video.h"*/

#define IN_FILE 2

struct dev_desc dev;
struct myframedev_desc myframedev;
/*struct encode_dev_desc encode_dev;*/

int flags = LIBYUV;
int LOOP = 1;

void sighandler(int data)
{
	LOOP = 0;
}

int main(int args, char *argv[])
{
	int ret = 0;
	char key = 0;
	struct pollfd pfd[IN_FILE];
	
	while((ret = getopt(args, argv, "dnm")) != -1) {
		switch(ret) {
			case 'd':
				break;
		}
	}
	//start
	ret = v4l2_app_init(&dev);
	if(ret < 0)
		return -1;
	ret = framebuffer_init(&myframedev);
	if(ret < 0)
		return -1;

	//signal C-c
	signal(SIGINT, sighandler);

	pfd[0].fd = fileno(stdin);
	pfd[0].events= POLLIN;

	pfd[1].fd = dev.f;
	pfd[1].events= POLLIN;
	//process
	while(LOOP) {
		ret = poll(pfd, IN_FILE, -1);
		if(ret < 0) {
			if(ret == EINTR)
				continue;
		}
		if(ret > 0) {
			if((pfd[0].revents & POLLIN) != 0) {
				ret = read(pfd[0].fd, &key, sizeof(key));
				//enter 'q' to exit
				if(ret > 0)
					if(key == 'q')
						break;
			}
			
			if((pfd[1].revents & POLLIN) != 0) {
				video_process(&dev,flags);
				/*hua(&myframedev);*/
				image_display(&myframedev, dev.rgb32, dev.w, dev.h);
			}
		}
	}
	
	black_screen(&myframedev);
	//close
	v4l2_app_close(&dev);
	framebuffer_close(&myframedev);
	printf("close success\n");
	
    return 0;    
}
