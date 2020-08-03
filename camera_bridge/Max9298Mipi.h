/*
 * Copyright 2017 NXP
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

#ifndef __MAX9298_H__
#define __MAX9298_H__

#include "Camera.h"
#include "DMAStream.h"

class Max9298Mipi : public Camera
{
public:
    Max9298Mipi(int32_t id, int32_t facing, int32_t orientation, char* path);
    ~Max9298Mipi();

    virtual status_t initSensorStaticData();
    virtual PixelFormat getPreviewPixelFormat();

private:
    class OvStream : public DMAStream
    {
    public:
        OvStream(Camera* device)
            : DMAStream(device, true) { mV4l2BufType = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE; }
        virtual ~OvStream() {}

        virtual int getCaptureMode(int width, int height);
        // configure device.
        virtual int32_t onDeviceConfigureLocked();
    };
};

#endif
