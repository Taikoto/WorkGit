LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
       uarttest7.c \

#LOCAL_CFLAGS += -DBUILD_FOR_ANDROID

LOCAL_C_INCLUDES += $(LOCAL_PATH) \

LOCAL_LDLIBS :=  -llog
LOCAL_SHARED_LIBRARIES := libutils libc

LOCAL_MODULE := uarttest7
LOCAL_MODULE_TAGS := tests
include $(BUILD_EXECUTABLE)
