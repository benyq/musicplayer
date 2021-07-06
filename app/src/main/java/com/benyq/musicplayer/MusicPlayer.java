package com.benyq.musicplayer;

import android.text.TextUtils;

public class MusicPlayer {
    static {
        System.loadLibrary("music-player");
    }

    private long nativeHandle;
    private OnErrorListener onErrorListener;
    private OnPreparedListener onPreparedListener;
    private OnProgressListener onProgressListener;


    public MusicPlayer() {
        nativeHandle = nativeInit();
    }

    public void setDataSource(String uri) {
        if (TextUtils.isEmpty(uri)) {
            throw new NullPointerException("url is null, please call method setDataSource first");
        }
        nativeSetDataSource(nativeHandle, uri);
    }

    public void prepare() {
        nativePrepare(nativeHandle);
    }

    public void play() {
        nativePlay(nativeHandle);
    }

    public void pause() {
        nativePause(nativeHandle);
    }

    public void stop() {
        nativeStop(nativeHandle);
    }

    public void seekTo(int sec){
        nativeSeekTo(nativeHandle, sec);
    }

    public void release() {
        onErrorListener = null;
        onPreparedListener = null;
        onProgressListener = null;

        nativeRelease(nativeHandle);
        nativeHandle = -1;
    }

    public int getDuration(){
        return nativeGetDuration(nativeHandle);
    }

    public int getCurrentPosition(){
        return nativeGetCurrentPosition(nativeHandle);
    }

    public boolean isPlaying() {
        return nativeIsPlaying(nativeHandle);
    }

    public boolean isPrepared() {
        return nativeIsPrepared(nativeHandle);
    }



    public void setOnErrorListener(OnErrorListener listener) {
        this.onErrorListener = listener;
    }

    public void setOnPreparedListener(OnPreparedListener listener) {
        this.onPreparedListener = listener;
    }

    public void setOnProgressListener(OnProgressListener listener) {
        this.onProgressListener = listener;
    }

    private void onError(int code){
        if (onErrorListener != null) {
            onErrorListener.onError(code);
        }
    }

    private void onPrepare() {
        if (onPreparedListener != null) {
            onPreparedListener.onPrepared();
        }
    }

    private void onProgress(int progress) {
        if (onProgressListener != null) {
            onProgressListener.onProgress(progress);
        }
    }


    public interface OnErrorListener{
        void onError(int code);
    }

    public interface OnPreparedListener{
        void onPrepared();
    }

    public interface OnProgressListener{
        void onProgress(int progress);
    }

    private native long nativeInit();
    private native void nativeSetDataSource(long nativeHandle, String uri);
    private native void nativePrepare(long nativeHandle);
    private native void nativePlay(long nativeHandle);
    private native void nativePause(long nativeHandle);
    private native void nativeStop(long nativeHandle);
    private native void nativeRelease(long nativeHandle);
    private native int nativeGetDuration(long nativeHandle);
    private native int nativeGetCurrentPosition(long nativeHandle);
    private native void nativeSeekTo(long nativeHandle, int sec);
    private native boolean nativeIsPlaying(long nativeHandle);
    private native boolean nativeIsPrepared(long nativeHandle);
}
