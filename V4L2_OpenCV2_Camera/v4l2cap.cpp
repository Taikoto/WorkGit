#include "v4l2cap.h"


V4L2Capture::V4L2Capture(char *devName, int width, int height, int width_cap, int height_cap)
{
	// TODO Auto-generated constructor stub
	this->devName = devName;
	this->fd_cam = -1;
	this->buffers = NULL;
	this->n_buffers = 0;
	this->frameIndex = -1;
	this->width = width;
	this->height = height;
	this->widthCap = width_cap;
	this->heightCap = height_cap;
}

V4L2Capture::~V4L2Capture(void)
{
	// TODO Auto-generated destructor stub
}

/**********打开设备**********/
int V4L2Capture::openDevice(void)
{
	/*设备的打开*/
	printf("video dev : %s\n", devName);
	fd_cam = open(devName, O_RDWR);
	if(fd_cam < 0) {
		perror("Can't open video device");
	}
	
	return TRUE;
}

/**********关闭设备**********/
int V4L2Capture::closeDevice(void)
{
	if(fd_cam > 0) {
		int ret = 0;
		if((ret = close(fd_cam)) < 0) {
			perror("Can't close video device");
		}
		return TRUE;
	} else {
		return FALSE;
	}
}

/**********初始化设备（预览模式）**********/
int V4L2Capture::initDevice(void)
{
	int ret;
	struct v4l2_capability cam_cap;		//显示设备信息
	struct v4l2_cropcap cam_cropcap;	//设置摄像头的捕捉能力
	struct v4l2_fmtdesc cam_fmtdesc;	//查询所有支持的格式：VIDIOC_ENUM_FMT
	struct v4l2_crop cam_crop;		    //图像的缩放
	struct v4l2_format cam_format;		//设置摄像头的视频制式、帧格式等

	/* 使用IOCTL命令VIDIOC_QUERYCAP，获取摄像头的基本信息*/
	ret = ioctl(fd_cam, VIDIOC_QUERYCAP, &cam_cap);
	if(ret < 0) {
		perror("Can't get device information: VIDIOCGCAP");
	}
	printf(
		"Driver Name:%s\nCard Name:%s\nBus info:%s\nDriver Version:%u.%u.%u\n",
		cam_cap.driver, cam_cap.card, cam_cap.bus_info,
		(cam_cap.version >> 16) & 0XFF, (cam_cap.version >> 8) & 0XFF,
		cam_cap.version & 0XFF);

	/* 使用IOCTL命令VIDIOC_ENUM_FMT，获取摄像头所有支持的格式*/
	cam_fmtdesc.index = 0;
	cam_fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	printf("Support format:\n");
	while(ioctl(fd_cam, VIDIOC_ENUM_FMT, &cam_fmtdesc) != -1) {
		printf("\t%d.%s\n", cam_fmtdesc.index + 1, cam_fmtdesc.description);
		cam_fmtdesc.index++;
	}

	/* 使用IOCTL命令VIDIOC_CROPCAP，获取摄像头的捕捉能力*/
	cam_cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(0 == ioctl(fd_cam, VIDIOC_CROPCAP, &cam_cropcap)) {
		printf("Default rec:\n\tleft:%d\n\ttop:%d\n\twidth:%d\n\theight:%d\n",
				cam_cropcap.defrect.left, cam_cropcap.defrect.top,
				cam_cropcap.defrect.width, cam_cropcap.defrect.height);
		/* 使用IOCTL命令VIDIOC_S_CROP，获取摄像头的窗口取景参数*/
		cam_crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		cam_crop.c = cam_cropcap.defrect;		//默认取景窗口大小
		if (-1 == ioctl(fd_cam, VIDIOC_S_CROP, &cam_crop)) {
			//printf("Can't set crop para\n");
		}
	} else {
		printf("Can't set cropcap para\n");
	}

	/* 使用IOCTL命令VIDIOC_S_FMT，设置摄像头帧信息*/
	cam_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	cam_format.fmt.pix.width = width;
	cam_format.fmt.pix.height = height;
	cam_format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;		//要和摄像头支持的类型对应
	cam_format.fmt.pix.field = V4L2_FIELD_INTERLACED;
	ret = ioctl(fd_cam, VIDIOC_S_FMT, &cam_format);
	if(ret < 0) {
		perror("Can't set frame information");
	}
	/* 使用IOCTL命令VIDIOC_G_FMT，获取摄像头帧信息*/
	cam_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(fd_cam, VIDIOC_G_FMT, &cam_format);
	if(ret < 0) {
		perror("Can't get frame information");
	}
	printf("Current data format information:\n\twidth:%d\n\theight:%d\n",
			cam_format.fmt.pix.width, cam_format.fmt.pix.height);
	
	return TRUE;
}

/**********初始化设备（拍照模式）**********/
int V4L2Capture::initDeviceCap(void)
{
	int ret;
	struct v4l2_format cam_format;		//设置摄像头的视频制式、帧格式等

	/* 使用IOCTL命令VIDIOC_S_FMT，设置摄像头帧信息*/
	cam_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	cam_format.fmt.pix.width = widthCap;
	cam_format.fmt.pix.height = heightCap;
	cam_format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;		//要和摄像头支持的类型对应
	cam_format.fmt.pix.field = V4L2_FIELD_INTERLACED;
	ret = ioctl(fd_cam, VIDIOC_S_FMT, &cam_format);
	if(ret < 0) {
		perror("Can't set frame information");
	}
	/* 使用IOCTL命令VIDIOC_G_FMT，获取摄像头帧信息*/
	cam_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(fd_cam, VIDIOC_G_FMT, &cam_format);
	if(ret < 0) {
		perror("Can't get frame information");
	}
	printf("Current data format information:\n\twidth:%d\n\theight:%d\n",
			cam_format.fmt.pix.width, cam_format.fmt.pix.height);
	
	return TRUE;
}

/**********申请缓存**********/
int V4L2Capture::initBuffers(void)
{
	int ret;
	/* 使用IOCTL命令VIDIOC_REQBUFS，申请帧缓冲*/
	struct v4l2_requestbuffers req;
	CLEAR(req);
	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	ret = ioctl(fd_cam, VIDIOC_REQBUFS, &req);
	if(ret < 0) {
		perror("Request frame buffers failed");
	}
	if(req.count < 2) {
		perror("Request frame buffers while insufficient buffer memory");
	}
	buffers = (struct cam_buffer*) calloc(req.count, sizeof(*buffers));
	if(!buffers) {
		perror("Out of memory");
	}
	for(n_buffers = 0; n_buffers < req.count; n_buffers++) {
		struct v4l2_buffer buf;
		CLEAR(buf);
		// 查询序号为n_buffers 的缓冲区，得到其起始物理地址和大小
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;
		ret = ioctl(fd_cam, VIDIOC_QUERYBUF, &buf);
		if(ret < 0) {
			printf("VIDIOC_QUERYBUF %d failed\n", n_buffers);
			return FALSE;
		}
		buffers[n_buffers].length = buf.length;
		//printf("buf.length= %d\n",buf.length);
		// 映射内存
		buffers[n_buffers].start = mmap(
				NULL, // start anywhere
				buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd_cam,
				buf.m.offset);
		if(MAP_FAILED == buffers[n_buffers].start) {
			printf("mmap buffer%d failed\n", n_buffers);
			return FALSE;
		}
	}
	
	return TRUE;
}

/**********释放缓存**********/
int V4L2Capture::freeBuffers(void)
{
	unsigned int i;
	
	for(i = 0; i < n_buffers; ++i) {
		if(-1 == munmap(buffers[i].start, buffers[i].length)) {
			printf("munmap buffer%d failed\n", i);
			return FALSE;
		}
	}
	free(buffers);
	
	return TRUE;
}

/**********开始采集**********/
int V4L2Capture::startCapture(void)
{
	unsigned int i;
	
	for(i = 0; i < n_buffers; i++) {
		struct v4l2_buffer buf;
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		if(-1 == ioctl(fd_cam, VIDIOC_QBUF, &buf)) {
			printf("VIDIOC_QBUF buffer%d failed\n", i);
			return FALSE;
		}
	}
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(-1 == ioctl(fd_cam, VIDIOC_STREAMON, &type)) {
		printf("VIDIOC_STREAMON error");
		return FALSE;
	}
	
	return TRUE;
}


/**********停止采集**********/
int V4L2Capture::stopCapture(void)
{
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	if(-1 == ioctl(fd_cam, VIDIOC_STREAMOFF, &type)) {
		printf("VIDIOC_STREAMOFF error\n");
		return FALSE;
	}
	
	return TRUE;
}

/**********获取图像**********/
int V4L2Capture::getFrame(void **frame_buf, unsigned int *len)
{
	struct v4l2_buffer queue_buf;
	
	CLEAR(queue_buf);
	queue_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	queue_buf.memory = V4L2_MEMORY_MMAP;
	if(-1 == ioctl(fd_cam, VIDIOC_DQBUF, &queue_buf)) {
		printf("VIDIOC_DQBUF error\n");
		return FALSE;
	}
	//printf("queue_buf.index=%d\n",queue_buf.index);
	//pthread_rwlock_wrlock(&rwlock);
	*frame_buf = buffers[queue_buf.index].start;
	*len = buffers[queue_buf.index].length;
	frameIndex = queue_buf.index;
	//pthread_rwlock_unlock(&rwlock);
	
	return TRUE;
}

/**********返回队列**********/
int V4L2Capture::backFrame(void)
{
	if(frameIndex != -1) {
		struct v4l2_buffer queue_buf;
		CLEAR(queue_buf);
		queue_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		queue_buf.memory = V4L2_MEMORY_MMAP;
		queue_buf.index = frameIndex;
		if(-1 == ioctl(fd_cam, VIDIOC_QBUF, &queue_buf)) {
			printf("VIDIOC_QBUF error\n");
			return FALSE;
		}
		return TRUE;
	}
	
	return FALSE;
}

/**********预览切换至拍照**********/
int V4L2Capture::pre2cap(void)
{
	if(V4L2Capture::stopCapture() == FALSE){
		printf("StopCapture fail~~\n");
		exit(2);
	}
	if(V4L2Capture::freeBuffers() == FALSE){
		printf("FreeBuffers fail~~\n");
		exit(2);
	}
	if(V4L2Capture::closeDevice() == FALSE){
		printf("CloseDevice fail~~\n");
		exit(1);
	}
	if(V4L2Capture::openDevice() == FALSE){
		printf("OpenDevice fail~~\n");
		exit(1);
	}
	if(V4L2Capture::initDeviceCap() == FALSE){
		printf("InitDeviceCap fail~~\n");
		exit(1);
	}
	if(V4L2Capture::initBuffers() == FALSE){
		printf("InitBuffers fail~~\n");
		exit(2);
	}
	if(V4L2Capture::startCapture() == FALSE){
		printf("StartCapture fail~~\n");
		exit(2);
	}
	
	return TRUE;
}

/**********拍照切换至预览**********/
int V4L2Capture::cap2pre(void)
{
	if(V4L2Capture::stopCapture() == FALSE){
		printf("StopCapture fail~~\n");
		exit(2);
	}
	if(V4L2Capture::freeBuffers() == FALSE){
		printf("FreeBuffers fail~~\n");
		exit(2);
	}
	if(V4L2Capture::closeDevice() == FALSE){
		printf("CloseDevice fail~~\n");
		exit(1);
	}
	if(V4L2Capture::openDevice() == FALSE){
		printf("OpenDevice fail~~\n");
		exit(1);
	}
	if(V4L2Capture::initDevice() == FALSE){
		printf("InitDevice fail~~\n");
		exit(1);
	}
	if(V4L2Capture::initBuffers() == FALSE){
		printf("InitBuffers fail~~\n");
		exit(2);
	}
	if(V4L2Capture::startCapture() == FALSE){
		printf("StartCapture fail~~\n");
		exit(2);
	}
	
	return TRUE;
}

/**********预览开启**********/
int V4L2Capture::preBegin(void)
{
	if(V4L2Capture::openDevice() == FALSE){
		printf("OpenDevice fail~~\n");
		exit(1);
	}
	printf("first~~\n");
	if(V4L2Capture::initDevice() == FALSE){
		printf("InitDevice fail~~\n");
		exit(1);
	}
	printf("second~~\n");
	if(V4L2Capture::initBuffers() == FALSE){
		printf("InitBuffers fail~~\n");
		exit(2);
	}
	printf("third~~\n");
	if(V4L2Capture::startCapture() == FALSE){
		printf("StartCapture fail~~\n");
		exit(2);
	}
	printf("fourth~~\n");
	
	return TRUE;
}

/**********预览结束**********/
int V4L2Capture::preEnd(void)
{
	if(V4L2Capture::stopCapture() == FALSE){
		printf("StopCapture fail~~\n");
		exit(2);
	}
	if(V4L2Capture::freeBuffers() == FALSE){
		printf("FreeBuffers fail~~\n");
		exit(2);
	}
	if(V4L2Capture::closeDevice() == FALSE){
		printf("CloseDevice fail~~\n");
		exit(1);
	}
	
	return TRUE;
}
