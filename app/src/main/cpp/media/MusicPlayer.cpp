//
// Created by Administrator on 2021/6/18.
//

#include "MusicPlayer.h"
#include <cstring>
#include <malloc.h>
#include <pthread.h>
#include "../utils/android_log.h"


MusicPlayer::MusicPlayer(JavaCallHelper *helper): helper(helper), prepareTask(0) {
    avformat_network_init();
    audioChannel = nullptr;
    avFormatContext = nullptr;
}

MusicPlayer::~MusicPlayer() {
    avformat_network_deinit();
    if (audioChannel) {
        delete audioChannel;
        this->audioChannel = nullptr;
    }
    delete helper;
    helper = nullptr;
    if (path) {
        delete [] path;
        path = nullptr;
    }
}

void MusicPlayer::setDataSource(const char *path) {
    this->path = static_cast<char *>(malloc(strlen(path) + 1));
    memset(this->path, 0, strlen(path) + 1);
    memcpy(this->path, path, strlen(path));
}

void *prepare_t(void *args) {
    auto *player = static_cast<MusicPlayer*>(args);
    player->_prepare();
    return nullptr;
}


void MusicPlayer::prepare() {
    pthread_create(&prepareTask, 0, prepare_t, this);
}

void MusicPlayer::play() {
    if (audioChannel) {
        audioChannel->play();
    }
}

void MusicPlayer::stop() {
    if (prepareTask != 0) {
        pthread_join(prepareTask, nullptr);
    }
    if (avFormatContext) {
        avformat_free_context(avFormatContext);
        avFormatContext = nullptr;
    }
    if (audioChannel) {
        audioChannel->stop();
        audioChannel = nullptr;
    }
}

void MusicPlayer::pause() {
    if (audioChannel) {
        audioChannel->pause();
    }
}

void MusicPlayer::_prepare() {
    avFormatContext = avformat_alloc_context();
    int ret = avformat_open_input(&avFormatContext, path, nullptr, nullptr);
    if (ret != 0) {
        LOGE("打开 %s 失败，返回 %d 错误描述 %s", this->path, ret, av_err2str(ret));
        helper->onError(FFMPEG_CAN_NOT_OPEN_URL, THREAD_CHILD);
        return;
    }

    ret = avformat_find_stream_info(avFormatContext, nullptr);
    if (ret < 0) {
        LOGE("查找媒体流 %s 失败，返回 %d 错误描述 %s", this->path, ret, av_err2str(ret));
        helper->onError(FFMPEG_CAN_NOT_FIND_STREAMS, THREAD_CHILD);
        return;
    }

    //等到视频时长，单位是秒
    this->duration = avFormatContext->duration / AV_TIME_BASE;
    for (int i = 0; i < avFormatContext->nb_streams; i++) {
        AVStream *avStream = avFormatContext->streams[i];
        //解码信息
        AVCodecParameters *parameters = avStream->codecpar;
        //查找解码器
        AVCodec *avCodec = avcodec_find_decoder(parameters->codec_id);
        if (avCodec == nullptr) {
            LOGE("找不到解码器 %s 失败，返回 %d 错误描述 %s", this->path, ret, av_err2str(ret));
            helper->onError(FFMPEG_FIND_PECODER_FAIL, THREAD_CHILD);
            return;
        }
        AVCodecContext *avCodecContext = avcodec_alloc_context3(avCodec);
        //解码的信息赋值给解码上下文
        ret = avcodec_parameters_to_context(avCodecContext, parameters);
        if (ret < 0) {
            LOGE("解码器赋值上下文 %s 失败，返回 %d 错误描述 %s", this->path, ret, av_err2str(ret));
            helper->onError(FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL, THREAD_CHILD);
            return;
        }
        ret = avcodec_open2(avCodecContext, avCodec, nullptr);
        if (ret != 0) {
            LOGE("打开解码器失败 %s，返回 %d 错误描述 %s", this->path, ret, av_err2str(ret));
            helper->onError(FFMPEG_OPEN_DECODER_FAIL, THREAD_CHILD);
            return;
        }
        if (parameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            this->audioChannel = new AudioChannel(helper, avFormatContext, avCodecContext, i, avStream->time_base);
        }
    }

    if (!audioChannel) {
        LOGE("没有音频流");
        helper->onError(FFMPEG_NOMEDIA, THREAD_CHILD);
        return;
    }

    //告诉java，准备好了，可以播放
    helper->onPrepare(THREAD_CHILD);
}

bool MusicPlayer::isPlaying() {
    if (audioChannel) {
        return audioChannel->isPlaying();
    }
    return false;
}

bool MusicPlayer::isPrepared() {
    return audioChannel != nullptr;
}

int MusicPlayer::getDuration() {
    if (!this->isPrepared()) {
        LOGD("音频还没有解码完成");
        return -1;
    }
    return (int)this->avFormatContext->duration / AV_TIME_BASE;
}

int MusicPlayer::getCurrentPosition() {
    if (audioChannel) {
        return (int)audioChannel->clock;
    }
    return -1;
}

void MusicPlayer::seekTo(int sec) {
    if (audioChannel) {
        audioChannel->seekTo(sec);
    }
}

