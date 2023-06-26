# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.
cmake_minimum_required(VERSION 3.10.2)
project("anyLive")
enable_language(ASM)
#set(CMAKE_CXX_FLAGS "-std=c++14 -DANDROID -fno-rtti")
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
set(STATIC_LIB_DIR ${CMAKE_SOURCE_DIR}/lib/)
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections -Wl,--exclude-libs,
${STATIC_LIB_DIR}/ffmpeg/${ANDROID_ABI}/libswscale.so,
${STATIC_LIB_DIR}/ffmpeg/${ANDROID_ABI}/libavformat.so,
${STATIC_LIB_DIR}/ffmpeg/${ANDROID_ABI}/libavcodec.so,
${STATIC_LIB_DIR}/ffmpeg/${ANDROID_ABI}/libavfilter.so,
${STATIC_LIB_DIR}/ffmpeg/${ANDROID_ABI}/libavutil.so,
${STATIC_LIB_DIR}/ffmpeg/${ANDROID_ABI}/libswresample.so,
${STATIC_LIB_DIR}/ffmpeg/${ANDROID_ABI}/libpostproc.so,
${STATIC_LIB_DIR}/webrtc/${ANDROID_ABI}/libwebrtc.a")

set(NATIVE_LIB "anyLive")

add_library(avutil SHARED IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION ${STATIC_LIB_DIR}/ffmpeg/${ANDROID_ABI}/libavutil.so)

add_library(avformat SHARED IMPORTED)
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION ${STATIC_LIB_DIR}/ffmpeg/${ANDROID_ABI}/libavformat.so)

add_library(avcodec SHARED IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION ${STATIC_LIB_DIR}/ffmpeg/${ANDROID_ABI}/libavcodec.so)

add_library(avfilter SHARED IMPORTED)
set_target_properties(avfilter PROPERTIES IMPORTED_LOCATION ${STATIC_LIB_DIR}/ffmpeg/${ANDROID_ABI}/libavfilter.so)

add_library(swresample SHARED IMPORTED)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION ${STATIC_LIB_DIR}/ffmpeg/${ANDROID_ABI}/libswresample.so)

add_library(swscale SHARED IMPORTED)
set_target_properties(swscale PROPERTIES IMPORTED_LOCATION ${STATIC_LIB_DIR}/ffmpeg/${ANDROID_ABI}/libswscale.so)

add_library(postproc SHARED IMPORTED)
set_target_properties(postproc PROPERTIES IMPORTED_LOCATION ${STATIC_LIB_DIR}/ffmpeg/${ANDROID_ABI}/libpostproc.so)


add_library(webrtc STATIC IMPORTED)
set_target_properties(webrtc PROPERTIES IMPORTED_LOCATION ${STATIC_LIB_DIR}/webrtc/${ANDROID_ABI}/libwebrtc.a)

include(${CMAKE_HOME_DIRECTORY}/third_party/faac-1.28/CMakeLists.txt)
include(${CMAKE_HOME_DIRECTORY}/ArLive/rtmp/CMakeLists.txt)
add_library(
        ${NATIVE_LIB}
        SHARED
        ${CMAKE_SOURCE_DIR}/jni/LiveEngine.cpp
        ${CMAKE_SOURCE_DIR}/jni/StaticThreads.cpp
        ${CMAKE_SOURCE_DIR}/jni/android/AndroidContext.cpp
        ${CMAKE_SOURCE_DIR}/jni/android/VideoCameraCapturer.cpp
        ${CMAKE_SOURCE_DIR}/jni/liveEngine/ArLiveEngineEvent.cpp
        ${CMAKE_SOURCE_DIR}/jni/liveEngine/ArLivePlayEvent.cpp
        ${CMAKE_SOURCE_DIR}/jni/liveEngine/ArLivePushEvent.cpp
        ${CMAKE_SOURCE_DIR}/jni/liveEngine/AndroidDeviceManager.cpp

        ${CMAKE_SOURCE_DIR}/ArLive/AndroidRenderer.cpp
        ${CMAKE_SOURCE_DIR}/ArLive/ArLive2Engine.cpp
        ${CMAKE_SOURCE_DIR}/ArLive/PlatformImpl.cpp
        ${CMAKE_SOURCE_DIR}/ArLive/VcmCapturer.cpp
        ${CMAKE_SOURCE_DIR}/ArLive/ArLive2Player.cpp
        ${CMAKE_SOURCE_DIR}/ArLive/ArLive2Pusher.cpp
        ${CMAKE_SOURCE_DIR}/ArLive/codec/aacencode.cc
        ${CMAKE_SOURCE_DIR}/ArLive/codec/AvCodec.cc
#        ${CMAKE_SOURCE_DIR}/ArLive/codec/g711.c
#        ${CMAKE_SOURCE_DIR}/ArLive/codec/g711_interface.c
        ${CMAKE_SOURCE_DIR}/ArLive/MgrRender.cpp
        ${CMAKE_SOURCE_DIR}/ArLive/PlayBuffer.cpp
        ${CMAKE_SOURCE_DIR}/ArLive/player/ARFFPlayer.cpp
        ${CMAKE_SOURCE_DIR}/ArLive/player/FFBuffer.cpp
        ${CMAKE_SOURCE_DIR}/ArLive/player/ARRtcPlayer.cpp
        ${CMAKE_SOURCE_DIR}/ArLive/player/sonic.c
        ${CMAKE_SOURCE_DIR}/ArLive/pusher/ARRtcPusher.cpp
        ${CMAKE_SOURCE_DIR}/ArLive/pusher/ARFFPusher.cpp
        ${CMAKE_SOURCE_DIR}/ArLive/pusher/ArFFWriter.cpp
#        ${CMAKE_SOURCE_DIR}/ArLive/ARRtspPusher.cpp
        ${CMAKE_SOURCE_DIR}/ArLive/ArNetClient.cpp
        ${CMAKE_SOURCE_DIR}/ArLive/ArNetTcpClient.cpp
        ${CMAKE_SOURCE_DIR}/ArLive/RtcTick.cpp
        ${CMAKE_SOURCE_DIR}/ArLive/ArHttpClientAndroid.cpp
        ${CMAKE_SOURCE_DIR}/ArLive/H264SeiPack.cpp

        ${CMAKE_SOURCE_DIR}/jni/util/ClassreferenceHolder.cc

)

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

target_link_libraries(
        ${NATIVE_LIB}
        faac
        rtmp
        -Wl,--start-group
        swscale
        avformat
        avcodec
        avfilter
        swresample
        postproc
        avutil
        -Wl,--end-group
        -Wl,--whole-archive
        webrtc
        -Wl,--no-whole-archive
        OpenSLES
        ${log-lib}
)



include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/ArLive/)
include_directories(${CMAKE_SOURCE_DIR}/ArLive/include/)
include_directories(${CMAKE_SOURCE_DIR}/jni/)
include_directories(${CMAKE_SOURCE_DIR}/jni/util/)
include_directories(${CMAKE_SOURCE_DIR}/jni/android/)
include_directories(${CMAKE_SOURCE_DIR}/ArLive/codec/)
include_directories(${CMAKE_SOURCE_DIR}/ArLive/player/)
include_directories(${CMAKE_SOURCE_DIR}/ArLive/pusher/)
include_directories(${CMAKE_SOURCE_DIR}/webrtc)
include_directories(${CMAKE_SOURCE_DIR}/webrtc/sdk/)
include_directories(${CMAKE_SOURCE_DIR}/webrtc/sdk/android/src/jni/)
include_directories(${CMAKE_SOURCE_DIR}/third_party/)
include_directories(${CMAKE_SOURCE_DIR}/webrtc/third_party/abseil-cpp/)
include_directories(${CMAKE_SOURCE_DIR}/third_party/libyuv/include/)
include_directories(${CMAKE_SOURCE_DIR}/third_party/rapidjson/)
include_directories(${CMAKE_SOURCE_DIR}/VideoRender/)

include_directories(${CMAKE_SOURCE_DIR}/lib/ffmpeg/include/)



add_definitions("-DWEBRTC_POSIX")
add_definitions("-DWEBRTC_ANDROID")
add_definitions("-DWEBRTC_USE_H264")
add_definitions("-DNDEBUG")
add_definitions("-DBUILDFLAG_INTERNAL_EXPENSIVE_DCHECKS_ARE_ON")
add_definitions("-DRW_LOCK")
add_definitions("-DWEBRTC_80")