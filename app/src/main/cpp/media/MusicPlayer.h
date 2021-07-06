//
// Created by Administrator on 2021/6/18.
//

#ifndef MUSICPLAYER_MUSICPLAYER_H
#define MUSICPLAYER_MUSICPLAYER_H

#include "JavaCallHelper.h"
#include "AudioChannel.h"

extern "C" {
#include <libavformat/avformat.h>
};

class MusicPlayer {

    friend void * prepare_t(void *args);


public:
    MusicPlayer(JavaCallHelper *helper);
    ~MusicPlayer();

    void setDataSource(const char *path);

    void prepare();
    void play();
    void stop();
    void pause();

    bool isPlaying();
    bool isPrepared();

    int getDuration();
    int getCurrentPosition();

    void seekTo(int sec);

private:
    JavaCallHelper *helper;
    char * path;
    pthread_t prepareTask;
    AVFormatContext *avFormatContext;
    int64_t duration;
    AudioChannel *audioChannel;

private:
    void _prepare();

};


#endif //MUSICPLAYER_MUSICPLAYER_H
