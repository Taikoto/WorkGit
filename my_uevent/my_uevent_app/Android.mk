LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := my_uevent_app.c

#LOCAL_CFLAGS += -DBUILD_FOR_ANDROID

LOCAL_C_INCLUDES += $(LOCAL_PATH)


LOCAL_SHARED_LIBRARIES := libutils libc

LOCAL_MODULE := my_uevent_app
LOCAL_MODULE_TAGS := tests
include $(BUILD_EXECUTABLE)
