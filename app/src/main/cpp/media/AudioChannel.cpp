//
// Created by Administrator on 2021/6/18.
//

#include <unistd.h>
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
    while (mDataQueue.empty() && mState != STOP) {
        waitForCache();
    }
    (*mPcmPlayer)->SetPlayState(mPcmPlayer, SL_PLAYSTATE_PLAYING);
    sReadPcmBufferCbFun(mPcmBuffer, this);
}

void AudioChannel::play() {

    if (mState == PAUSE) {
        mState = DECODING;
        if (mPcmPlayer != nullptr) {
            (*mPcmPlayer)->SetPlayState(mPcmPlayer, SL_PLAYSTATE_PLAYING);
        }
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
    if (mPcmPlayer != nullptr) {
        (*mPcmPlayer)->SetPlayState(mPcmPlayer, SL_PLAYSTATE_PAUSED);
    }
}

void AudioChannel::stop() {
    mState = STOP;
    if (mPcmPlayer != nullptr) {
        (*mPcmPlayer)->SetPlayState(mPcmPlayer, SL_PLAYSTATE_STOPPED);
    }
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
                    return NULL; //????????????
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
            //TODO ????????????????????????packet?????????????????????frame?????????
            int result = avcodec_receive_frame(avCodecContext, avFrame);
            if (result == 0) {
                av_packet_unref(avPacket);
                return avFrame;
            } else {
                LOGE("Receive frame error result: %s", av_err2str(AVERROR(result)));
            }
        }
        // ??????packet
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
    if (mOutBuffer) {
        delete mOutBuffer;
        mOutBuffer = nullptr;
    }
}

void AudioChannel::loopDecode() {
    while (1) {
        if (mState == STOP) {
            break;
        }
        if (decodeOneFrame()) {
            render(avFrame);
        }
    }
}

void AudioChannel::render(AVFrame *frame) {

    // ???????????????????????????????????????
    int nb = swr_convert(mSwr, &mOutBuffer, dataSize / 2,
                          (const uint8_t **) frame->data, frame->nb_samples);

    LOGE("old nb_samples: %d, new nb_samples %d", frame->nb_samples, nb);
    clock = frame->pts * av_q2d(time_base);
    helper->onProgress((int)clock ,THREAD_CHILD);
    dataSize = av_samples_get_buffer_size(NULL, 2, nb,AV_SAMPLE_FMT_S16, 1);

    if (mPcmPlayer) {
        while (mDataQueue.size() >= 2 && mState != STOP) {
            sendCacheReadySignal();
            usleep(20000);
        }

        // ???????????????????????????????????????
        uint8_t *data = (uint8_t *) malloc(dataSize);
        memcpy(data, mOutBuffer, dataSize);

        PcmData *pcmData = new PcmData(data, dataSize);
        mDataQueue.push(pcmData);

        // ?????????????????????????????????????????????
        sendCacheReadySignal();
    } else {
        LOGE("mPcmPlayer is nullptr");
    }
}

void AudioChannel::release() {

}

void AudioChannel::createOpenSL() {
    //1.??????????????????
    SLresult result = slCreateEngine(&mEngineObj, 0, NULL, 0, NULL, NULL);
    if (checkError(result, "Engine")) return;

    result = (*mEngineObj)->Realize(mEngineObj, SL_BOOLEAN_FALSE);
    if (checkError(result, "Engine Realize")) return;

    result = (*mEngineObj)->GetInterface(mEngineObj, SL_IID_ENGINE, &mEngine);

    //2.???????????????
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

    //3.???????????????

    //??????PCM????????????
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            SL_QUEUE_BUFFER_COUNT};
    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,//??????pcm???????????????
            (SLuint32) 2,//2????????????????????????
            SL_SAMPLINGRATE_44_1,//44100hz?????????
            SL_PCMSAMPLEFORMAT_FIXED_16,//?????? 16???
            SL_PCMSAMPLEFORMAT_FIXED_16,//?????????????????????
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,//???????????????????????????
            SL_BYTEORDER_LITTLEENDIAN//????????????
    };
    SLDataSource slDataSource = {&android_queue, &pcm};

    //???????????????
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, mOutputMixObj};
    SLDataSink slDataSink = {&outputMix, NULL};

    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    result = (*mEngine)->CreateAudioPlayer(mEngine, &mPcmPlayerObj, &slDataSource, &slDataSink, 3,
                                           ids, req);
    if (checkError(result, "Player")) return;

    //??????????????????
    result = (*mPcmPlayerObj)->Realize(mPcmPlayerObj, SL_BOOLEAN_FALSE);
    if (checkError(result, "Player Realize")) return;

    //??????????????????????????????Player??????
    result = (*mPcmPlayerObj)->GetInterface(mPcmPlayerObj, SL_IID_PLAY, &mPcmPlayer);
    if (checkError(result, "Player Interface")) return;

    //????????????????????????????????????????????????
    result = (*mPcmPlayerObj)->GetInterface(mPcmPlayerObj, SL_IID_BUFFERQUEUE, &mPcmBuffer);
    if (checkError(result, "Player Queue Buffer")) return;

    //??????????????????
    result = (*mPcmBuffer)->RegisterCallback(mPcmBuffer, sReadPcmBufferCbFun, this);
    if (checkError(result, "Register Callback Interface")) return;

    //??????????????????
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

    // ????????????????????????????????????
    while (!mDataQueue.empty()) {
        PcmData *pcm = mDataQueue.front();
        if (pcm && pcm->used) {
            mDataQueue.pop();
            delete pcm;
        } else {
            break;
        }
    }

    // ??????????????????
    while (mDataQueue.empty() && mPcmPlayer != nullptr) {// if m_pcm_player is NULL, stop render
        waitForCache();
    }

    PcmData *pcmData = mDataQueue.front();
    if (nullptr != pcmData && mPcmPlayer) {
        SLresult result = (*mPcmBuffer)->Enqueue(mPcmBuffer, pcmData->pcm,
                                                 (SLuint32) pcmData->size);
        if (result == SL_RESULT_SUCCESS) {
            // ????????????????????????????????????????????????????????????
            // ?????????????????????????????????????????????????????????
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
        //?????????????????????
        avcodec_flush_buffers(avCodecContext);
    }
}
