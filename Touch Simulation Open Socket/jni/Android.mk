LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
# give module name
LOCAL_MODULE    := view32.xml
# list your C files to compile
LOCAL_SRC_FILES := pubg/main32.cpp
# this option will build executables instead of building library for android application.
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
# give module name
LOCAL_MODULE    := view64.xml
# list your C files to compile
LOCAL_CFLAGS += -DNDEBUG
LOCAL_SRC_FILES := pubg/main64.cpp
# this option will build executables instead of building library for android application.
LOCAL_LDFLAGS += -Wl,--gc-sections
LOCAL_LDFLAGS += -L$(SYSROOT)/usr/lib -lz -llog -landroid
include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)
# give module name
LOCAL_MODULE    := viewn32.xml
# list your C files to compile
LOCAL_SRC_FILES := pubg/mainc32.cpp
# this option will build executables instead of building library for android application.
include $(BUILD_EXECUTABLE)