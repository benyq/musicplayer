//
// Created by Administrator on 2021/6/3.
//

#ifndef FFPLAYER_ANDROID_LOG_H
#define FFPLAYER_ANDROID_LOG_H
#include <android/log.h>

#define TAG "JNI_TAG"
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)


#endif //FFPLAYER_ANDROID_LOG_H
