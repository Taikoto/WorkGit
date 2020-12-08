#include "include.h"
#include "v4l2cap.h"
#include "thread.h"


#define IMAGEWIDTH_CAP    1280 //拍照分辨率
#define IMAGEHEIGHT_CAP   720//拍照分辨率

#define IMAGEWIDTH    640      //预览分辨率
#define IMAGEHEIGHT   480     //预览分辨率

#define TRUE    1
#define FALSE   0

unsigned char flag;

int main(int argc, char **argv)
{
	IplImage* img,* img_cap;
	CvMat cvmat,cvmat_cap;
	double t;
	flag = 0;
	unsigned char *frame = NULL;
	unsigned int frameSize = 0;
	string videoDev = "/dev/video0";//制定设备号
	
	if(argc < 1) {
		fprintf(stderr, "-Usage: %s <dev>\n", argv[0]);
		return -1;
	} else if(argc >= 1) {
		videoDev = argv[1];
	}

	V4L2Capture *vcap = new V4L2Capture(const_cast<char*>(videoDev.c_str()),
			IMAGEWIDTH, IMAGEHEIGHT, IMAGEWIDTH_CAP, IMAGEHEIGHT_CAP);
	vcap->preBegin();//预览模式开启

	cvNamedWindow("one",CV_WINDOW_AUTOSIZE);//创建显示窗口

	pthread_t id;
	printf("Main thread id is %d \n",pthread_self());
	if(!pthread_create(&id,NULL,thread,NULL)) {
		printf("succeed!\n");
	} else {
		printf("Fail to Create Thread");
		return -1;
	}

	while(1) {
		//如果flag为1，则抓取一张照片
		if(flag == 1) {
			vcap->pre2cap();                      //预览模式切换至拍照模式
			//这里多获取几次图像队列，以便得到更高的图像质量（刚打开设备时图像模糊）
			vcap->getFrame((void **) &frame, &frameSize);
			vcap->backFrame();
			vcap->getFrame((void **) &frame, &frameSize);
			vcap->backFrame();
			vcap->getFrame((void **) &frame, &frameSize);
			cvmat_cap = cvMat(IMAGEHEIGHT_CAP,IMAGEWIDTH_CAP,CV_8UC3,frame);//CV_8UC3
			img_cap = cvDecodeImage(&cvmat_cap,1);//OpenCV图像解码

			if(!img_cap)
				printf("No img_cap\n");
			cvSaveImage("cap.jpg",img_cap);       //保存图片
			cvReleaseImage(&img_cap);             //释放img_cap
			vcap->backFrame();                    //返回队列
			vcap->cap2pre();                      //拍照模式切换至预览模式
			flag = 0;
		}
		
		t = (double)cvGetTickCount();
		vcap->getFrame((void **) &frame, &frameSize);
		cvmat = cvMat(IMAGEHEIGHT,IMAGEWIDTH,CV_8UC3,frame);//CV_8UC3
		img = cvDecodeImage(&cvmat,1);           //OpenCV图像解码
		if(!img)
			printf("No img\n");
		cvShowImage("one",img);                  //显示图片
		cvReleaseImage(&img);                    //释放img
		vcap->backFrame();                       //返回队列
		cvWaitKey(1);                            //没有这句话图像无法显示
		t = (double)cvGetTickCount()-t;
		printf("used time is %gms\n",(t/(cvGetTickFrequency()*1000)));
	}

	pthread_exit(0);                            //退出键盘检测线程

	vcap->preEnd();                             //预览模式结束

	return 0;
}

