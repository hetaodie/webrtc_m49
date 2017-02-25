LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

#
# Module Settings
#
LOCAL_MODULE := videotool

#
# Source Files
#
CODEC_PATH := ../../../../
LOCAL_SRC_FILES := \
            myjni.cpp \
            videotool.cpp \

#
# Header Includes
#
LOCAL_C_INCLUDES := \
	      	$(LOCAL_PATH)/../../x264/include \
	      	$(LOCAL_PATH)/../../ffmpeg-2.8.1/android/arm/include \

#
# Compile Flags and Link Libraries
#
LOCAL_CFLAGS := -DANDROID_NDK -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS

LOCAL_LDLIBS := -llog
LOCAL_LDLIBS += -lz

LOCAL_DISABLE_FATAL_LINKER_WARNINGS := true
LOCAL_LDFLAGS += $(LOCAL_PATH)/../../x264/lib/libx264.a \
				$(LOCAL_PATH)/../../ffmpeg-2.8.1/android/arm/lib/libavcodec.a \
				$(LOCAL_PATH)/../../ffmpeg-2.8.1/android/arm/lib/libavutil.a \
				$(LOCAL_PATH)/../../ffmpeg-2.8.1/android/arm/lib/libswscale.a \

include $(BUILD_SHARED_LIBRARY)

