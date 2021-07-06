cmake_minimum_required(VERSION 3.10.2)

project("music-player")

include_directories(${CMAKE_SOURCE_DIR}/include)

#指定共享库的库目录
link_directories(${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI})
file(GLOB_RECURSE MEDIA ${CMAKE_SOURCE_DIR}/media/*.*)
file(GLOB_RECURSE UTILS ${CMAKE_SOURCE_DIR}/utils/*.*)

add_library(
        music-player
        SHARED
        ${UTILS}
        ${MEDIA}
        ${CMAKE_SOURCE_DIR}/media/AudioChannel.cpp
        music-player.cpp)

find_library(
        log-lib
        log)

target_link_libraries(
        music-player
        avutil
        swresample
        avcodec
        avfilter
        swscale
        avformat
        avdevice
        android
        OpenSLES
        ${log-lib})