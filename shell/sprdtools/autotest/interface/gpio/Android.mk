LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_32_BIT_ONLY := true

LOCAL_SRC_FILES := gpio.cpp
LOCAL_MODULE := autotestgpio
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/engpc

ifneq (,$(findstring 9863, $(TARGET_PRODUCT)))
PLATFORM := sharkl3
else ifneq (,$(findstring 9832, $(TARGET_PRODUCT)))
PLATFORM := sharkle
else ifneq (,$(findstring 9820, $(TARGET_PRODUCT)))
PLATFORM := sharkle
else
PLATFORM := default
endif
$(warning TOP:$(TOP) LOCAL_PATH:$(LOCAL_PATH) TARGET_PRODUCT:$(TARGET_PRODUCT)  platform:$(PLATFORM))

LOCAL_C_INCLUDES:= \
    $(TOP)/vendor/sprd/proprietories-source/engmode \
    $(LOCAL_PATH)/$(PLATFORM)

LOCAL_SHARED_LIBRARIES:= \
    libcutils \
    liblog

include $(BUILD_SHARED_LIBRARY)
