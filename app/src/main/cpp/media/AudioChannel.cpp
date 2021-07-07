//
// Created by Administrator on 2021/6/18.
//

#include "AudioChannel.h"
#include "../utils/android_log.h"

AudioChannel::AudioChannel(JavaCallHelper *helper, AVFormatContext *avFormatContext,
                           AVCodecContext *avCodecContext, int streamIndex, AVRational base)
        : avCodecContext(avCodecContext), helper(helper), time_base(base) , steamIndex(streamIndex), avFormatContext(avFormatContext){

}

AudioChannel::~AudioChannel() {

}

void *decode_t(void *args) {
    auto audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->decode();
    return nullptr;
}

void *audioPlay_t(void *args) {
    auto audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->playAudio();
    return nullptr;
}

void AudioChannel::playAudio() {
    while (mDataQueue.empty()) {
        waitForCache();
    }
    (*mPcmPlayer)->SetPlayState(mPcmPlayer, SL_PLAYSTATE_PLAYING);
    sReadPcmBufferCbFun(mPcmBuffer, this);
}

void AudioChannel::play() {

    if (mState == PAUSE) {
        mState = DECODING;
        sendCacheReadySignal();
    } else {
        pthread_create(&decodeTask, nullptr, decode_t, this);
    }
}

void AudioChannel::decode() {
    mState = DECODING;
    prepare();
    loopDecode();
    doneDecode();
}

void AudioChannel::pause() {
    mState = PAUSE;
}

void AudioChannel::stop() {
    mState = STOP;
    pthread_join(decodeTask, nullptr);
    pthread_join(audioPlayTask, nullptr);
}

void AudioChannel::prepare() {
    avPacket = av_packet_alloc();
    avFrame = av_frame_alloc();
    initSwr();
    createOpenSL();
    pthread_create(&audioPlayTask, 0, audioPlay_t, this);
}

AVFrame *AudioChannel::decodeOneFrame() {
    int ret = av_read_frame(avFormatContext, avPacket);
    while (ret == 0) {
        if (avPacket->stream_index == steamIndex) {
            switch (avcodec_send_packet(avCodecContext, avPacket)) {
                case AVERROR_EOF: {
                    av_packet_unref(avPacket);
                    LOGE("Decode error: %s", av_err2str(AVERROR_EOF));
                    return NULL; //解码结束
                }
                case AVERROR(EAGAIN):
                    LOGE("Decode error: %s", av_err2str(AVERROR(EAGAIN)));
                    break;
                case AVERROR(EINVAL):
                    LOGE("Decode error: %s", av_err2str(AVERROR(EINVAL)));
                    break;
                case AVERROR(ENOMEM):
                    LOGE("Decode error: %s", av_err2str(AVERROR(ENOMEM)));
                    break;
                default:
                    break;
            }
            //TODO 这里需要考虑一个packet有可能包含多个frame的情况
            int result = avcodec_receive_frame(avCodecContext, avFrame);
            if (result == 0) {
                av_packet_unref(avPacket);
                return avFrame;
            } else {
                LOGE("Receive frame error result: %s", av_err2str(AVERROR(result)));
            }
        }
        // 释放packet
        av_packet_unref(avPacket);
        ret = av_read_frame(avFormatContext, avPacket);
    }
    av_packet_unref(avPacket);
    LOGI("ret = %s", av_err2str(AVERROR(ret)));
    return nullptr;
}

void AudioChannel::doneDecode() {
    releaseOpenSL();
    if (this->mSwr) {
        swr_free(&mSwr);
        this->mSwr = nullptr;
    }
    while (!mDataQueue.empty()) {
        PcmData *pcm = mDataQueue.front();
        if (pcm) {
            mDataQueue.pop();
            delete pcm;
        } else {
            break;
        }
    }
}

void AudioChannel::loopDecode() {
    while (1) {

        if (mState == STOP) {
            break;
        }

        if (mState == PAUSE) {
            waitForCache();
        }

        if (decodeOneFrame()) {
            render(avFrame);
        }
    }
}

void AudioChannel::render(AVFrame *frame) {

    // 转换，返回每个通道的样本数
    int nb = swr_convert(mSwr, &mOutBuffer, dataSize / 2,
                          (const uint8_t **) frame->data, frame->nb_samples);

    LOGE("old nb_samples: %d, new nb_samples %d", frame->nb_samples, nb);
    clock = frame->pts * av_q2d(time_base);
    helper->onProgress((int)clock ,THREAD_CHILD);
    dataSize = av_samples_get_buffer_size(NULL, 2, nb,AV_SAMPLE_FMT_S16, 1);

    if (mPcmPlayer) {
        while (mDataQueue.size() >= 2) {
            sendCacheReadySignal();
        }

        // 将数据复制一份，并压入队列
        uint8_t *data = (uint8_t *) malloc(dataSize);
        memcpy(data, mOutBuffer, dataSize);

        PcmData *pcmData = new PcmData(data, dataSize);
        mDataQueue.push(pcmData);

        // 通知播放线程推出等待，恢复播放
        sendCacheReadySignal();
    } else {
        LOGE("mPcmPlayer is nullptr");
    }
}

void AudioChannel::release() {

}

void AudioChannel::createOpenSL() {
    //1.创建引擎对象
    SLresult result = slCreateEngine(&mEngineObj, 0, NULL, 0, NULL, NULL);
    if (checkError(result, "Engine")) return;

    result = (*mEngineObj)->Realize(mEngineObj, SL_BOOLEAN_FALSE);
    if (checkError(result, "Engine Realize")) return;

    result = (*mEngineObj)->GetInterface(mEngineObj, SL_IID_ENGINE, &mEngine);

    //2.创建混音器
    const SLInterfaceID mids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean mreq[1] = {SL_BOOLEAN_FALSE};
    result = (*mEngine)->CreateOutputMix(mEngine, &mOutputMixObj, 1, mids, mreq);
    if (checkError(result, "Output Mix")) return;

    result = (*mOutputMixObj)->Realize(mOutputMixObj, SL_BOOLEAN_FALSE);
    if (checkError(result, "Output Mix Realize")) return;

    result = (*mOutputMixObj)->GetInterface(mOutputMixObj, SL_IID_ENVIRONMENTALREVERB,
                                            &mOutputMixEvnReverb);
    if (checkError(result, "Output Mix Env Reverb")) return;

    (*mOutputMixEvnReverb)->SetEnvironmentalReverbProperties(mOutputMixEvnReverb, &mReverbSettings);

    //3.创建播放器

    //配置PCM格式信息
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            SL_QUEUE_BUFFER_COUNT};
    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,//播放pcm格式的数据
            (SLuint32) 2,//2个声道（立体声）
            SL_SAMPLINGRATE_44_1,//44100hz的频率
            SL_PCMSAMPLEFORMAT_FIXED_16,//位数 16位
            SL_PCMSAMPLEFORMAT_FIXED_16,//和位数一致就行
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,//立体声（前左前右）
            SL_BYTEORDER_LITTLEENDIAN//结束标志
    };
    SLDataSource slDataSource = {&android_queue, &pcm};

    //配置音频池
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, mOutputMixObj};
    SLDataSink slDataSink = {&outputMix, NULL};

    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    result = (*mEngine)->CreateAudioPlayer(mEngine, &mPcmPlayerObj, &slDataSource, &slDataSink, 3,
                                           ids, req);
    if (checkError(result, "Player")) return;

    //初始化播放器
    result = (*mPcmPlayerObj)->Realize(mPcmPlayerObj, SL_BOOLEAN_FALSE);
    if (checkError(result, "Player Realize")) return;

    //得到接口后调用，获取Player接口
    result = (*mPcmPlayerObj)->GetInterface(mPcmPlayerObj, SL_IID_PLAY, &mPcmPlayer);
    if (checkError(result, "Player Interface")) return;

    //注册回调缓冲区，获取缓冲队列接口
    result = (*mPcmPlayerObj)->GetInterface(mPcmPlayerObj, SL_IID_BUFFERQUEUE, &mPcmBuffer);
    if (checkError(result, "Player Queue Buffer")) return;

    //缓冲接口回调
    result = (*mPcmBuffer)->RegisterCallback(mPcmBuffer, sReadPcmBufferCbFun, this);
    if (checkError(result, "Register Callback Interface")) return;

    //获取音量接口
//    result = (*mPcmPlayerObj)->GetInterface(mPcmPlayerObj, SL_IID_VOLUME, &mPcmPlayerVolume);
    if (checkError(result, "Player Volume Interface")) return;

    LOGI("OpenSL ES init success");

}

void
AudioChannel::sReadPcmBufferCbFun(SLAndroidSimpleBufferQueueItf bufferQueueItf, void *context) {
    auto audioChannel = static_cast<AudioChannel *>(context);
    audioChannel->blockEnqueue();
}

void AudioChannel::releaseOpenSL() {

}

bool AudioChannel::checkError(SLresult result, const char *hint) {
    if (SL_RESULT_SUCCESS != result) {
        LOGE("OpenSL ES [%s] init fail", hint);
        return true;
    }
    return false;
}


void AudioChannel::blockEnqueue() {
    if (mPcmPlayer == nullptr) return;

    // 先将已经使用过的数据移除
    while (!mDataQueue.empty()) {
        PcmData *pcm = mDataQueue.front();
        if (pcm && pcm->used) {
            mDataQueue.pop();
            delete pcm;
        } else {
            break;
        }
    }

    // 等待数据缓冲
    while (mDataQueue.empty() && mPcmPlayer != nullptr) {// if m_pcm_player is NULL, stop render
        waitForCache();
    }

    PcmData *pcmData = mDataQueue.front();
    if (nullptr != pcmData && mPcmPlayer) {
        SLresult result = (*mPcmBuffer)->Enqueue(mPcmBuffer, pcmData->pcm,
                                                 (SLuint32) pcmData->size);
        if (result == SL_RESULT_SUCCESS) {
            // 只做已经使用标记，在下一帧数据压入前移除
            // 保证数据能正常使用，否则可能会出现破音
            pcmData->used = true;
        }
    }
}

void AudioChannel::initSwr() {
    enum AVSampleFormat outSampleFormat = AVSampleFormat::AV_SAMPLE_FMT_S16;
    int64_t outChLayout = AV_CH_LAYOUT_STEREO;
    int outSampleRate = AUDIO_SAMPLE_RATE;

    mSwr = swr_alloc_set_opts(nullptr, outChLayout, outSampleFormat, outSampleRate,
                              avCodecContext->channel_layout, avCodecContext->sample_fmt,
                              avCodecContext->sample_rate, 0,
                              nullptr);

    swr_init(mSwr);
    dataSize = av_samples_get_buffer_size(nullptr, 2,
                                          outSampleRate, outSampleFormat,0);

    mOutBuffer = (uint8_t *) malloc(sizeof(uint8_t) * dataSize);
}

bool AudioChannel::isPlaying() {
    return mState == DECODING;
}

void AudioChannel::seekTo(int sec) {

    int64_t startTime = sec * AV_TIME_BASE;
    LOGD("onProgressChanged startTime: %hd", startTime);
    int64_t target_time = av_rescale_q(startTime,AV_TIME_BASE_Q,time_base);
    LOGD("onProgressChanged target_time: %hd", target_time);
    int ret = av_seek_frame(avFormatContext, steamIndex, target_time, AVSEEK_FLAG_BACKWARD);
    if (ret >= 0) {
        //清空解码器缓存
        avcodec_flush_buffers(avCodecContext);
    }
}
