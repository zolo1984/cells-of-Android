LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(call all-subdir-java-files)
LOCAL_RESOURCE_DIR += $(LOCAL_PATH)/res
LOCAL_PACKAGE_NAME := stopsystem
LOCAL_MODULE_OWNER := cells
LOCAL_CERTIFICATE := platform

include $(BUILD_PACKAGE)