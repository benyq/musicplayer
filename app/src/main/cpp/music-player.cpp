#include <jni.h>
#include <string>
#include <jni.h>
#include "media/MusicPlayer.h"

JavaVM *javaVM = nullptr;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    javaVM = vm;
    return JNI_VERSION_1_4;
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_benyq_musicplayer_MusicPlayer_nativeInit(JNIEnv *env, jobject thiz) {
    auto *player = new MusicPlayer(new JavaCallHelper(javaVM, env, thiz));
    return reinterpret_cast<jlong>(player);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_benyq_musicplayer_MusicPlayer_nativeSetDataSource(JNIEnv *env, jobject thiz,
                                                           jlong native_handle, jstring uri) {
    const char *path = env->GetStringUTFChars(uri, 0);
    auto *player = reinterpret_cast<MusicPlayer*>(native_handle);
    player->setDataSource(path);
    env->ReleaseStringUTFChars(uri, path);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_benyq_musicplayer_MusicPlayer_nativePrepare(JNIEnv *env, jobject thiz,
                                                     jlong native_handle) {
    auto *player = reinterpret_cast<MusicPlayer*>(native_handle);
    player->prepare();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_benyq_musicplayer_MusicPlayer_nativePlay(JNIEnv *env, jobject thiz, jlong native_handle) {
    auto *player = reinterpret_cast<MusicPlayer*>(native_handle);
    player->play();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_benyq_musicplayer_MusicPlayer_nativePause(JNIEnv *env, jobject thiz, jlong native_handle) {
    auto *player = reinterpret_cast<MusicPlayer*>(native_handle);
    player->pause();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_benyq_musicplayer_MusicPlayer_nativeStop(JNIEnv *env, jobject thiz, jlong native_handle) {
    auto *player = reinterpret_cast<MusicPlayer*>(native_handle);
    player->stop();
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_benyq_musicplayer_MusicPlayer_nativeGetDuration(JNIEnv *env, jobject thiz,
                                                         jlong native_handle) {
    auto *player = reinterpret_cast<MusicPlayer*>(native_handle);
    return player->getDuration();
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_benyq_musicplayer_MusicPlayer_nativeGetCurrentPosition(JNIEnv *env, jobject thiz,
                                                            jlong native_handle) {
    auto *player = reinterpret_cast<MusicPlayer*>(native_handle);
    return player->getCurrentPosition();
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_benyq_musicplayer_MusicPlayer_nativeIsPlaying(JNIEnv *env, jobject thiz,
                                                       jlong native_handle) {
    auto *player = reinterpret_cast<MusicPlayer*>(native_handle);
    bool playing = player->isPlaying();
    return playing;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_benyq_musicplayer_MusicPlayer_nativeIsPrepared(JNIEnv *env, jobject thiz,
                                                        jlong native_handle) {
    auto *player = reinterpret_cast<MusicPlayer*>(native_handle);
    bool prepared = player->isPrepared();
    return prepared;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_benyq_musicplayer_MusicPlayer_nativeRelease(JNIEnv *env, jobject thiz, jlong native_handle) {
    auto *player = reinterpret_cast<MusicPlayer*>(native_handle);
    delete player;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_benyq_musicplayer_MusicPlayer_nativeSeekTo(JNIEnv *env, jobject thiz, jlong native_handle, jint sec) {
    auto *player = reinterpret_cast<MusicPlayer*>(native_handle);
    player->seekTo(sec);
}