/*
 * Copyright (C) 2012-2015 Freescale Semiconductor, Inc.
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

#ifndef _Tp2825_H
#define _Tp2825_H

#include "Camera.h"
#include "DMAStream.h"

class Tp2825 : public Camera
{
public:
    Tp2825(int32_t id, int32_t facing, int32_t orientation, char *path);
    ~Tp2825();

    virtual status_t initSensorStaticData();
    virtual PixelFormat getPreviewPixelFormat();

private:
    class Tp2825Stream : public DMAStream
    {
    public:
        Tp2825Stream(Camera *device) : DMAStream(device, true) { mV4l2BufType = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE; }
        virtual ~Tp2825Stream() {}

        // configure device.
        virtual int32_t onDeviceConfigureLocked();
        /*virtual int32_t onDeviceStopLocked();*/
    };
};

#endif
