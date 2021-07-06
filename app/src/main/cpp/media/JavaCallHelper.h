//
// Created by Administrator on 2021/6/4.
//

#ifndef FFPLAYER_JAVACALLHELPER_H
#define FFPLAYER_JAVACALLHELPER_H

#include <jni.h>

#define THREAD_MAIN 1
#define THREAD_CHILD 2

//错误代码
//打不开视频
#define FFMPEG_CAN_NOT_OPEN_URL 1
//找不到流媒体
#define FFMPEG_CAN_NOT_FIND_STREAMS 2
//找不到解码器
#define FFMPEG_FIND_PECODER_FAIL 3
//无法根据解码器创建上下文
#define FFMPEG_ALLOC_CODEC_CONTEXT_FAIL 4
//根据流信点面路上下业参数失女
#define FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL 6
//打开部码器失威
#define FFMPEG_OPEN_DECODER_FAIL 7
//没育音视频
#define FFMPEG_NOMEDIA 8


class JavaCallHelper{

public:
    JavaCallHelper(JavaVM * _javaVM, JNIEnv *_env, jobject &_jobj);
    ~JavaCallHelper();
    void onError(int code, int thread = THREAD_MAIN);
    void onPrepare(int thread = THREAD_MAIN);
    void onProgress(int progress, int thread = THREAD_MAIN);


public:

    JavaVM *javaVm;
    JNIEnv *env;
    jobject jobj;
    jmethodID jmid_error;
    jmethodID jmid_prepare;
    jmethodID jmid_progress;

};

#endif //FFPLAYER_JAVACALLHELPER_H
