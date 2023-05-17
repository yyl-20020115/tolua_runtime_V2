cd luajit-2.1/src

# Android/ARM, armeabi-v7a (ARMv7 VFP), Android 4.0+ (ICS)
mkdir -p Plugins/Android/libs/armeabi-v7a
#NDK=/c/android-ndk-r13b
NDK=~/opt/ndk-r25b/android-ndk-r25b
NDKABI=19
NDKVER=$NDK/toolchains/llvm
NDKP=$NDKVER/prebuilt/linux-x86_64/bin/armv7a-linux-androideabi19-
NDKF="--sysroot $NDK/platforms/android-$NDKABI/arch-arm"  
NDKARCH="-march=armv7-a -mfloat-abi=softfp -Wl,--fix-cortex-a8 -fno-omit-frame-pointer"

make HOST_CC="gcc -m32" CROSS=$NDKP TARGET_SYS=Linux TARGET_FLAGS="$NDKF $NDKARCH" SYSROOT="$NDK/platforms/android-$NDKABI/arch-arm"

cd ../../android
$NDK/ndk-build APP_ABI="armeabi-v7a"
# cp libs/armeabi-v7a/libtolua.so ../../DouluoDalu/libmain/
