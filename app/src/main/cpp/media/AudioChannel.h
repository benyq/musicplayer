//
// Created by Administrator on 2021/6/18.
//

#ifndef MUSICPLAYER_AUDIOCHANNEL_H
#define MUSICPLAYER_AUDIOCHANNEL_H

#include <jni.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include "JavaCallHelper.h"
#include "PcmData.h"
#include "MusicState.h"
#include <pthread.h>
#include <queue>

using namespace std;
#define AUDIO_SAMPLE_RATE 44100;

extern "C" {
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/audio_fifo.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
};

class AudioChannel {
    friend void *decode_t(void *args);
    friend void *audioPlay_t(void *args);

public:
    double clock = 0;
    AVRational time_base;

    void seekTo(int sec);

public:
    AudioChannel(JavaCallHelper *helper, AVFormatContext *avFormatContext, AVCodecContext *avCodecContext, int steamIndex, AVRational base);
    ~AudioChannel();

    void decode();

    void play();
    void pause();
    void stop();

    bool isPlaying();

private:
    JavaCallHelper *helper;
    AVCodecContext *avCodecContext;
    AVFormatContext *avFormatContext;
    pthread_t decodeTask;
    pthread_t audioPlayTask;
    int steamIndex = 0;

    // 缓存线程等待锁变量
    pthread_mutex_t m_cache_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t m_cache_cond = PTHREAD_COND_INITIALIZER;

    // 待解码包
    AVPacket *avPacket = NULL;
    // 最终解码数据
    AVFrame *avFrame = NULL;

    // 解码状态
    MusicState mState = STOP;

    const SLuint32 SL_QUEUE_BUFFER_COUNT = 2;
    // 引擎接口
    SLObjectItf mEngineObj = NULL;
    SLEngineItf mEngine = NULL;

    //混音器
    SLObjectItf mOutputMixObj = NULL;
    SLEnvironmentalReverbItf mOutputMixEvnReverb = NULL;
    SLEnvironmentalReverbSettings mReverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;

    //pcm播放器
    SLObjectItf mPcmPlayerObj = NULL;
    SLPlayItf mPcmPlayer = NULL;
    SLVolumeItf mPcmPlayerVolume = NULL;

    //缓冲器队列接口
    SLAndroidSimpleBufferQueueItf mPcmBuffer;

    queue<PcmData *> mDataQueue;

    // 音频转换器
    SwrContext *mSwr = NULL;
    int dataSize = 0;
    // 输出缓冲
    uint8_t *mOutBuffer;

private:
    /**
     * 分配解码过程中需要的缓存
     */
    void prepare();
    AVFrame* decodeOneFrame();
    void doneDecode();
    void loopDecode();
    void render(AVFrame *frame);
    void release();
    void playAudio();

    void createOpenSL();
    void releaseOpenSL();
    void blockEnqueue();

    void initSwr();

    bool checkError(SLresult result, const char *hint);
    void static sReadPcmBufferCbFun(SLAndroidSimpleBufferQueueItf bufferQueueItf, void *context);

    void waitForCache() {
        pthread_mutex_lock(&m_cache_mutex);
        pthread_cond_wait(&m_cache_cond, &m_cache_mutex);
        pthread_mutex_unlock(&m_cache_mutex);
    }

    void sendCacheReadySignal() {
        pthread_mutex_lock(&m_cache_mutex);
        pthread_cond_signal(&m_cache_cond);
        pthread_mutex_unlock(&m_cache_mutex);
    }

};

#endif //MUSICPLAYER_AUDIOCHANNEL_H
