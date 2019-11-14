LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := flora-dispatcher
LOCAL_LDLIBS := -llog

MY_FILES_PATH := $(LOCAL_PATH)/src
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
LOCAL_SHARED_LIBRARIES := librlog libcaps libflora-svc
LOCAL_STATIC_LIBRARIES := libmisc
include $(BUILD_EXECUTABLE)
