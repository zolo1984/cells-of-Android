# celld Makefile
#
# Copyright (C) 2011-2013 Columbia University
# Author: Jeremy C. Andrus <jeremya@cs.columbia.edu>
#

LOCAL_PATH := $(call my-dir)


#-------------------------------------------------------------------
# copy switch APK
#-------------------------------------------------------------------
# include $(CLEAR_VARS)
# LOCAL_MODULE	:= SwitchSystem
# LOCAL_SRC_FILES	:= $(LOCAL_MODULE).apk
# LOCAL_MODULE_TAGS	:= optional eng
# LOCAL_MODULE_CLASS	:= APPS
# LOCAL_MODULE_SUFFIX	:= $(COMMON_ANDROID_PACKAGE_SUFFIX)
# #LOCAL_MODULE_PATH	:= $(TARGET_OUT_DATA_APPS)
# LOCAL_MODULE_PATH	:= $(TARGET_OUT_APPS)
# LOCAL_CERTIFICATE	:= platform
#
# include $(BUILD_PREBUILT)


#
# celld (container control daemon)
#
include $(CLEAR_VARS)

LOCAL_CFLAGS :=

LOCAL_SRC_FILES:= \
	ext/glibc_openpty.c \
	cell_console.c \
	nsexec.c \
	shared_ops.c \
	util.c \
	celld.c \
	cell_config.c \
	nl.c \
	network.c

LOCAL_MODULE := celld
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := huawei
LOCAL_C_INCLUDES := \
	$(call include-path-for, libhardware_legacy)/hardware_legacy
LOCAL_SHARED_LIBRARIES := libm libcutils libc libhardware_legacy libselinux
#LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_SBIN)
include $(BUILD_EXECUTABLE)


#
# cell (container control front-end)
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	ext/glibc_openpty.c \
	cell_console.c \
	shared_ops.c \
	util.c \
	cell.c

LOCAL_MODULE:= cellc
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := huawei
LOCAL_SHARED_LIBRARIES := libm libcutils libc
include $(BUILD_EXECUTABLE)


# vmcmd
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	util.c \
	vmcmd.cpp \
	cellsocket.cpp 

LOCAL_MODULE:= vmcmd
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := huawei
#LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_SBIN)
LOCAL_SHARED_LIBRARIES := libm libcutils libc libgui libbinder libutils libpowermanager
include $(BUILD_EXECUTABLE)


#hostcmd
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	util.c \
	hostcmd.cpp \
	cellsocket.cpp

LOCAL_MODULE:= hostcmd
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := huawei
#LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_SBIN)
LOCAL_SHARED_LIBRARIES := libm libcutils libc libgui libbinder libutils libpowermanager
include $(BUILD_EXECUTABLE)


