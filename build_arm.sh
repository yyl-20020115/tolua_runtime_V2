cd luajit-2.1/src

# Android/ARM, armeabi-v7a (ARMv7 VFP), Android 4.0+ (ICS)
NDK=~/opt/android-ndk-r25c/
NDKABI=19
NDKVER=$NDK/toolchains/llvm
NDKP=$NDKVER/prebuilt/linux-x86_64/bin/armv7a-linux-androideabi$NDKABI-
NDKF=""  
NDKARCH="-march=armv7-a -mfloat-abi=softfp -fno-omit-frame-pointer -fPIC"
make USE_64=0 CROSS=$NDKP TARGET_SYS=Linux TARGET_FLAGS="$NDKARCH --sysroot $NDK/toolchains/llvm/prebuilt/linux-x86_64/sysroot/ " 

cd ../../android
$NDK/ndk-build APP_ABI="armeabi-v7a"
# cp libs/armeabi-v7a/libtolua.so ../../DouluoDalu/libmain/
