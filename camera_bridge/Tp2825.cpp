/*
 * Copyright (C) 2012-2016 Freescale Semiconductor, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Tp2825.h"

#include "tp2825_config.h"


Tp2825::Tp2825(int32_t id, int32_t facing, int32_t orientation, char *path)
    : Camera(id, facing, orientation, path)
{
    mVideoStream = new Tp2825Stream(this);
}

Tp2825::~Tp2825()
{
}

status_t Tp2825::initSensorStaticData()
{
    int32_t fd = open(mDevPath, O_RDWR);
    if (fd < 0) {
        ALOGE("Tp2825Device: initParameters sensor has not been opened");
        return BAD_VALUE;
    }

    // first read sensor format.
    int ret = 0, index = 0;
    int sensorFormats[MAX_SENSOR_FORMAT];
    int availFormats[MAX_SENSOR_FORMAT];
    memset(sensorFormats, 0, sizeof(sensorFormats));
    memset(availFormats, 0, sizeof(availFormats));

    // Don't support enum format, now hard code here.
    sensorFormats[index] = v4l2_fourcc('Y', 'U', 'Y', 'V');
    availFormats[index++] = v4l2_fourcc('Y', 'U', 'Y', 'V');
    mSensorFormatCount =
        changeSensorFormats(sensorFormats, mSensorFormats, index);
    if (mSensorFormatCount == 0) {
        ALOGE("%s no sensor format enum", __func__);
        close(fd);
        return BAD_VALUE;
    }

    availFormats[index++] = v4l2_fourcc('N', 'V', '2', '1');
    mAvailableFormatCount =
        changeSensorFormats(availFormats, mAvailableFormats, index);

    v4l2_std_id id;
    if (ioctl(fd, VIDIOC_G_STD, &id) < 0) {
         ALOGE("get std failed");
    }
	if (id == V4L2_STD_PAL)
		ALOGI("std is PAL");
	else if (id == V4L2_STD_NTSC)
		ALOGI("std is NTSC");
	else
		ALOGI("std is unknown");

    index = 0;
    char TmpStr[20];
    int previewCnt = 0, pictureCnt = 0;
    struct v4l2_frmsizeenum vid_frmsize;
    struct v4l2_frmivalenum vid_frmval;
    while (ret == 0) 
	{
        memset(TmpStr, 0, 20);
        memset(&vid_frmsize, 0, sizeof(struct v4l2_frmsizeenum));
        vid_frmsize.index = index++;
        vid_frmsize.pixel_format =
            convertPixelFormatToV4L2Format(mSensorFormats[0]);
        ret = ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &vid_frmsize);
        if (ret != 0) {
            continue;
        }
        ALOGI("enum frame size w:%d, h:%d", vid_frmsize.discrete.width, vid_frmsize.discrete.height);

        // v4l2 does not support, now hard code here.
        if ((vid_frmsize.discrete.width > 1280) ||(vid_frmsize.discrete.height > 800)) 
		{
            vid_frmval.discrete.denominator = 15;
            vid_frmval.discrete.numerator = 1;
        }
		else 
		{
            vid_frmval.discrete.denominator = 30;
            vid_frmval.discrete.numerator = 1;
        }
#if TP2825_50HZ			
            vid_frmval.discrete.denominator = 50;//org:15;
#endif

#if TP2825_25HZ			
			vid_frmval.discrete.denominator = 25;//org:15;
#endif

#if TP2825_30HZ			
			vid_frmval.discrete.denominator = 30;//org:15;
#endif

#if TP2825_60HZ			
			vid_frmval.discrete.denominator = 60;//org:15;
#endif

        // If w/h ratio is not same with senserW/sensorH, framework assume that
        // first crop little width or little height, then scale.
        // But 1920x1080, 176x144 not work in this mode.
        // For 1M pixel, 720p sometimes may take green picture(5%), so not report
        // it,
        // use 1024x768 for 1M pixel
        // 1920x1080 1280x720 is required by CTS.
        if (!(vid_frmsize.discrete.width == 176 &&
              vid_frmsize.discrete.height == 144)) {
            mPictureResolutions[pictureCnt++] = vid_frmsize.discrete.width;
            mPictureResolutions[pictureCnt++] = vid_frmsize.discrete.height;
        }

        if (vid_frmval.discrete.denominator / vid_frmval.discrete.numerator > 15) {
            mPreviewResolutions[previewCnt++] = vid_frmsize.discrete.width;
            mPreviewResolutions[previewCnt++] = vid_frmsize.discrete.height;
        }
    }  // end while

    mPreviewResolutionCount = previewCnt;
    mPictureResolutionCount = pictureCnt;

    mMinFrameDuration = 33331760L;
    mMaxFrameDuration = 30000000000L;
    int i;
    for (i = 0; i < MAX_RESOLUTION_SIZE && i < pictureCnt; i += 2) {
        ALOGI("SupportedPictureSizes: %d x %d", mPictureResolutions[i], mPictureResolutions[i + 1]);
    }

    adjustPreviewResolutions();
    for (i = 0; i < MAX_RESOLUTION_SIZE && i < previewCnt; i += 2) {
        ALOGI("SupportedPreviewSizes: %ld x %ld", mPreviewResolutions[i], mPreviewResolutions[i + 1]);
    }
    ALOGI("FrameDuration is %lld, %lld", mMinFrameDuration, mMaxFrameDuration);

    i = 0;
#if TP2825_50HZ			
	mTargetFpsRange[i++] = 50;//org:10;
    mTargetFpsRange[i++] = 50;//org:30;
    mTargetFpsRange[i++] = 50;//org:30;
    mTargetFpsRange[i++] = 50;//org:30;
    mTargetFpsRange[i++] = 50;//org:30;
    mTargetFpsRange[i++] = 50;//org:30;
#endif
	
#if TP2825_25HZ			
	mTargetFpsRange[i++] = 25;//org:10;
    mTargetFpsRange[i++] = 25;//org:30;
    mTargetFpsRange[i++] = 25;//org:30;
    mTargetFpsRange[i++] = 25;//org:30;
    mTargetFpsRange[i++] = 25;//org:30;
    mTargetFpsRange[i++] = 25;//org:30;
#endif
	
#if TP2825_30HZ			
	mTargetFpsRange[i++] = 30;//org:10;
    mTargetFpsRange[i++] = 30;//org:30;
    mTargetFpsRange[i++] = 30;//org:30;
    mTargetFpsRange[i++] = 30;//org:30;
    mTargetFpsRange[i++] = 30;//org:30;
    mTargetFpsRange[i++] = 30;//org:30;
#endif

#if TP2825_60HZ			
	mTargetFpsRange[i++] = 60;//org:10;
    mTargetFpsRange[i++] = 60;//org:30;
    mTargetFpsRange[i++] = 60;//org:30;
    mTargetFpsRange[i++] = 60;//org:30;
    mTargetFpsRange[i++] = 60;//org:30;
    mTargetFpsRange[i++] = 60;//org:30;
#endif	
    setMaxPictureResolutions();
    ALOGI("mMaxWidth:%d, mMaxHeight:%d", mMaxWidth, mMaxHeight);

    mFocalLength = 3.37f;
    mPhysicalWidth = 3.6288f;   // 2592 x 1.4u
    mPhysicalHeight = 2.7216f;  // 1944 x 1.4u
    mActiveArrayWidth = 1280;
    mActiveArrayHeight = 800;
    mPixelArrayWidth = 1280;
    mPixelArrayHeight = 800;
#if TP2825_960X480    
    mActiveArrayWidth = 960;
    mActiveArrayHeight = 480;
    mPixelArrayWidth = 960;
    mPixelArrayHeight = 480;
#endif

#if TP2825_1280X720    
	mActiveArrayWidth = 1280;
	mActiveArrayHeight = 720;
	mPixelArrayWidth = 1280;
	mPixelArrayHeight = 720;
#endif

#if TP2825_720X480    
		mActiveArrayWidth = 720;
		mActiveArrayHeight = 480;
		mPixelArrayWidth = 720;
		mPixelArrayHeight = 480;
#endif

    ALOGI("ImxdpuCsi, mFocalLength:%f, mPhysicalWidth:%f, mPhysicalHeight %f",
          mFocalLength,
          mPhysicalWidth,
          mPhysicalHeight);

    close(fd);
    return NO_ERROR;
}

PixelFormat Tp2825::getPreviewPixelFormat()
{
    ALOGI("%s", __func__);
    return HAL_PIXEL_FORMAT_YCbCr_422_I;
}

// configure device.
int32_t Tp2825::Tp2825Stream::onDeviceConfigureLocked()
{
    ALOGI("%s", __func__);
    int32_t ret = 0;
    if (mDev <= 0) {
        ALOGE("%s invalid fd handle", __func__);
        return BAD_VALUE;
    }

    int32_t fps = mFps;
    int32_t vformat;
    vformat = convertPixelFormatToV4L2Format(mFormat);

    if ((mWidth > 1280) || (mHeight > 720)) 
	{
        fps = 15;
    }
	else if ((mWidth > 1280) || (mHeight > 720)) 
	{
		fps = 25;
		ALOGI("lqc set tp2825 fps = %d", fps);
	}
	else 
    {    	
        fps = 30;
    }
#if TP2825_50HZ			
        fps = 50;
#endif

#if TP2825_25HZ			
		fps = 25;
#endif

#if TP2825_30HZ			
		fps = 30;
#endif

#if TP2825_60HZ			
		fps = 60;
#endif

    ALOGI("Width * Height %d x %d format %c%c%c%c, fps: %d", mWidth, mHeight, vformat & 0xFF, (vformat >> 8) & 0xFF, (vformat >> 16) & 0xFF, (vformat >> 24) & 0xFF, fps);

    struct v4l2_streamparm param;
    memset(&param, 0, sizeof(param));
    param.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    param.parm.capture.timeperframe.numerator   = 1;
    param.parm.capture.timeperframe.denominator = fps;
//    param.parm.capture.capturemode = 0;


    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix_mp.pixelformat = vformat;
    fmt.fmt.pix_mp.width = mWidth & 0xFFFFFFF8;
    fmt.fmt.pix_mp.height = mHeight & 0xFFFFFFF8;
    fmt.fmt.pix_mp.num_planes = 1; /* adv7180 use YUYV format, is packed storage mode, set num_planes 1*/

    ret = ioctl(mDev, VIDIOC_S_FMT, &fmt);
    if (ret < 0) {
        ALOGE("%s: VIDIOC_S_FMT Failed: %s", __func__, strerror(errno));
        return ret;
    }

    return 0;
}
/*
int32_t Tp2825::Tp2825Stream::onDeviceStopLocked()
{
    ALOGI("%s", __func__);
    int32_t ret = 0;
    enum v4l2_buf_type bufType;
    struct v4l2_requestbuffers req;

    if (mDev <= 0) {
        ALOGE("%s invalid fd handle", __func__);
        return BAD_VALUE;
    }

    for (uint32_t i = 0; i < MAX_STREAM_BUFFERS; i++) {
        if (mBuffers[i] != NULL && mBuffers[i]->mVirtAddr != NULL
                                && mBuffers[i]->mSize > 0) {
            munmap(mBuffers[i]->mVirtAddr, mBuffers[i]->mSize);
            delete mBuffers[i];
            mBuffers[i] = NULL;
        }
    }

    bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    ret = ioctl(mDev, VIDIOC_STREAMOFF, &bufType);
    if (ret < 0) {
        ALOGE("%s VIDIOC_STREAMOFF failed: %s", __func__, strerror(errno));
        return ret;
    }

    memset(&req, 0, sizeof (req));
    req.count = 0;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(mDev, VIDIOC_REQBUFS, &req) < 0) {
        ALOGI("%s VIDIOC_REQBUFS failed: %s", __func__, strerror(errno));
        return BAD_VALUE;
    }
	
    return 0;
}*/
