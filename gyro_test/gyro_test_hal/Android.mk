LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := gyro_temp

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

#LOCAL_CFLAGS := "-Wall", "-Werror"

LOCAL_SRC_FILES := gyro_temp.c

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    liblog \
    libstlport \
    libutils \

LOCAL_PRELINK_MODULE := false
LOCAL_STRIP_MODULE := false

LOCAL_C_INCLUDES := \
    external/stlport/stlport \
    bionic \

include $(BUILD_SHARED_LIBRARY)
include $(call all-makefiles-under, $(LOCAL_PATH))
