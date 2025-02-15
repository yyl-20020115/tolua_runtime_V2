LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libluajit
LOCAL_SRC_FILES := ../../luajit-2.1/src/libluajit.a
include $(PREBUILT_STATIC_LIBRARY)

#include $(CLEAR_VARS)
#LOCAL_MODULE := libunwindstack
#LOCAL_SRC_FILES := ../../../unwind/build_arm/libunwindstack_static.a
#include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_LDFLAGS += 
LOCAL_LDLIBS += -llog
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE := tolua
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../luajit-2.1/src
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../pbc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../cjson
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../

LOCAL_CPPFLAGS := -O2
LOCAL_CFLAGS :=  -O2 -std=gnu99
LOCAL_SRC_FILES :=	../../tolua.c \
					../../int64.c \
					../../uint64.c \
					../../pb.c \
					../../lpeg.c \
					../../struct.c \
					../../cjson/strbuf.c \
					../../cjson/lua_cjson.c \
					../../cjson/fpconv.c \
					../../luasocket/auxiliar.c \
 					../../luasocket/buffer.c \
 					../../luasocket/except.c \
 					../../luasocket/inet.c \
 					../../luasocket/io.c \
 					../../luasocket/luasocket.c \
 					../../luasocket/mime.c \
 					../../luasocket/options.c \
 					../../luasocket/select.c \
 					../../luasocket/tcp.c \
 					../../luasocket/timeout.c \
 					../../luasocket/udp.c \
 					../../luasocket/usocket.c \
				    ../../sproto.new/sproto.c \
				    ../../sproto.new/lsproto.c \
				    ../../pbc/src/alloc.c \
				    ../../pbc/src/array.c \
				    ../../pbc/src/bootstrap.c \
				    ../../pbc/src/context.c \
				    ../../pbc/src/decode.c \
				    ../../pbc/src/map.c \
				    ../../pbc/src/pattern.c \
				    ../../pbc/src/proto.c \
				    ../../pbc/src/register.c \
				    ../../pbc/src/rmessage.c \
				    ../../pbc/src/stringpool.c \
				    ../../pbc/src/varint.c \
				    ../../pbc/src/wmessage.c \
				    ../../pbc/binding/lua/pbc-lua.c \

LOCAL_WHOLE_STATIC_LIBRARIES += libluajit
# LOCAL_WHOLE_STATIC_LIBRARIES += libunwindstack 

include $(BUILD_SHARED_LIBRARY)

