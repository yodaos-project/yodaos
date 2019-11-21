LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := librlog

MY_FILES_PATH := $(LOCAL_PATH)/src/log
MY_FILES_SUFFIX := %.cpp %.c %.cc
My_All_Files := \
	$(patsubst \
		$(LOCAL_PATH)/%, \
		%, \
		$(filter \
			$(MY_FILES_SUFFIX), \
			$(shell \
				find $(MY_FILES_PATH) -type f \
			) \
		) \
	)
LOCAL_SRC_FILES := $(My_All_Files)
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include/log
LOCAL_STATIC_LIBRARIES := libmisc
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include/log
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libcaps
LOCAL_LDLIBS := -llog

MY_FILES_PATH := $(LOCAL_PATH)/src/caps
MY_FILES_SUFFIX := %.cpp %.c %.cc
My_All_Files := \
	$(patsubst \
		$(LOCAL_PATH)/%, \
		%, \
		$(filter \
			$(MY_FILES_SUFFIX), \
			$(shell \
				find $(MY_FILES_PATH) -type f \
			) \
		) \
	)
LOCAL_SRC_FILES := $(My_All_Files)
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include/caps
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include/caps
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libmisc

MY_FILES_PATH := $(LOCAL_PATH)/src/misc
MY_FILES_SUFFIX := %.cpp %.c %.cc
My_All_Files := \
	$(patsubst \
		$(LOCAL_PATH)/%, \
		%, \
		$(filter \
			$(MY_FILES_SUFFIX), \
			$(shell \
				find $(MY_FILES_PATH) -type f \
			) \
		) \
	)
LOCAL_SRC_FILES := $(My_All_Files)
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include/misc
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include/misc
include $(BUILD_STATIC_LIBRARY)
