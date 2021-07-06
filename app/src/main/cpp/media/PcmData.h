//
// Created by Administrator on 2021/6/18.
//

#ifndef MUSICPLAYER_PCMDATA_H
#define MUSICPLAYER_PCMDATA_H

class PcmData {
public:
    PcmData(uint8_t *pcm, int size) {
        this->pcm = pcm;
        this->size = size;
    }
    ~PcmData() {
        if (pcm != nullptr) {
            //释放已使用的内存
            delete pcm;
            pcm = nullptr;
            used = false;
        }
    }
    uint8_t *pcm = NULL;
    int size = 0;
    bool used = false;
};

#endif //MUSICPLAYER_PCMDATA_H
