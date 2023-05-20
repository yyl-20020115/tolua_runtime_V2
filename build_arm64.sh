cd luajit-2.1/src

# Android/ARM, armeabi-v8a (ARMv7 VFP), Android 4.0+ (ICS)
NDK=~/opt/android-ndk-r25c/
NDKABI=21
NDKTRIPLE=aarch64-linux-android
NDKVER=$NDK/toolchains/llvm
NDKP=$NDKVER/prebuilt/linux-x86_64/bin/$NDKTRIPLE$NDKABI-
NDKS="--sysroot $NDK/toolchains/llvm/prebuilt/linux-x86_64/sysroot/"
# NDKF="--sysroot $NDK/platforms/android-$NDKABI/arch-arm64" 
NDKARCH="-march=armv8a -fno-omit-frame-pointer -fPIC -DLJ_ABI_SOFTFP=0 -DLJ_ARCH_HASFPU=1 -DLUAJIT_ENABLE_GC64=1"

make USE_64=1 CROSS=$NDKP TARGET_SYS=Linux TARGET_FLAGS="$NDKARCH $NDKS"
#cp ./libluajit.a ../../android/jni/libluajit.a
#make clean

cd ../../android
#$NDK/ndk-build clean APP_ABI="arm64-v8a"
$NDK/ndk-build APP_ABI="arm64-v8a"
#cp libs/arm64-v8a/libtolua.so ../Plugins/Android/libs/arm64-v8a
#$NDK/ndk-build clean APP_ABI="arm64-v8a"
