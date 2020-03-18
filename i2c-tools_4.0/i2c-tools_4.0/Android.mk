#Android.mk
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng
LOCAL_MODULE := i2c-tools

LOCAL_SRC_FILES := \
	tools/i2cbusses.c \
	tools/util.c \
	lib/smbus.c

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/include

#LOCAL_CFLAGS := -g -Wall -Werror -Wno-unused-parameter
include $(BUILD_STATIC_LIBRARY)

########################################
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng
LOCAL_MODULE:=i2cdetect

LOCAL_SRC_FILES:= \
	tools/i2cdetect.c
	
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/include

LOCAL_SHARED_LIBRARIES:= \
	libc
LOCAL_STATIC_LIBRARIES := \
	i2c-tools
	
LOCAL_CPPFLAGS += -DANDROID

include $(BUILD_EXECUTABLE)

########################################
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng
LOCAL_MODULE:=i2cget

LOCAL_SRC_FILES:= \
	tools/i2cget.c
	
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/include

LOCAL_SHARED_LIBRARIES:= \
	libc
LOCAL_STATIC_LIBRARIES := \
	i2c-tools
	
LOCAL_CPPFLAGS += -DANDROID

include $(BUILD_EXECUTABLE)

########################################
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng
LOCAL_MODULE:=i2cset

LOCAL_SRC_FILES:= \
	tools/i2cset.c
	
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/include

LOCAL_SHARED_LIBRARIES:= \
	libc
LOCAL_STATIC_LIBRARIES := \
	i2c-tools

LOCAL_CPPFLAGS += -DANDROID

include $(BUILD_EXECUTABLE)

########################################
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng
LOCAL_MODULE:=i2cdump

LOCAL_SRC_FILES:= \
	tools/i2cdump.c
	
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/include

LOCAL_SHARED_LIBRARIES:= \
	libc
LOCAL_STATIC_LIBRARIES := \
	i2c-tools

LOCAL_CPPFLAGS += -DANDROID

include $(BUILD_EXECUTABLE)

########################################
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng
LOCAL_MODULE:=i2ctransfer

LOCAL_SRC_FILES:= \
	tools/i2ctransfer.c
	
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/include

LOCAL_SHARED_LIBRARIES:= \
	libc
LOCAL_STATIC_LIBRARIES := \
	i2c-tools

LOCAL_CPPFLAGS += -DANDROID

include $(BUILD_EXECUTABLE)
