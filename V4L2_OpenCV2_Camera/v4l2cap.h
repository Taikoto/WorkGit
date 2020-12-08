#ifndef V4L2CAP_H_
#define V4L2CAP_H_

#include "include.h"

#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define TRUE 1
#define FALSE 0

class V4L2Capture {
public:
	V4L2Capture(char *devName, int width, int height, int width_cap, int height_cap);
	virtual ~V4L2Capture();

	int openDevice(void);
	int closeDevice(void);
	int initDevice(void);
	int initDeviceCap(void);
	int startCapture(void);
	int stopCapture(void);
	int freeBuffers(void);
	int getFrame(void **,unsigned int *);
	int backFrame(void);
	int pre2cap(void);
	int cap2pre(void);
	int preBegin(void);
	int preEnd(void);

	int initBuffers(void);

	struct cam_buffer
	{
		void* start;
		unsigned int length;
	};
	char *devName;
	int widthCap;
	int heightCap;
	int width;
	int height;
	int fd_cam;
	cam_buffer *buffers;
	unsigned int n_buffers;
	int frameIndex;
};

#endif /* V4L2CAP_H_ */

