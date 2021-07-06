//
// Created by Administrator on 2021/6/4.
//

#include "JavaCallHelper.h"

JavaCallHelper::JavaCallHelper(JavaVM *_javaVM, JNIEnv *_env, jobject &_jobj) : javaVm(_javaVM), env(_env){
    jobj = env->NewGlobalRef(_jobj);
    jclass jclazz = env->GetObjectClass(jobj);

    jmid_error = env->GetMethodID(jclazz, "onError", "(I)V");
    jmid_prepare = env->GetMethodID(jclazz, "onPrepare", "()V");
    jmid_progress = env->GetMethodID(jclazz, "onProgress", "(I)V");
}

JavaCallHelper::~JavaCallHelper() {
    env->DeleteGlobalRef(jobj);
    jobj = nullptr;
}

void JavaCallHelper::onError(int code, int thread) {
    if (thread == THREAD_CHILD) {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK) {
            return;
        }
        env->CallVoidMethod(jobj, jmid_error, code);
        javaVm->DetachCurrentThread();
    } else {
        env->CallVoidMethod(jobj, jmid_error, code);
    }
}

void JavaCallHelper::onPrepare(int thread) {
    if (thread == THREAD_CHILD) {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK) {
            return;
        }
        env->CallVoidMethod(jobj, jmid_prepare);
        javaVm->DetachCurrentThread();
    } else {
        env->CallVoidMethod(jobj, jmid_prepare);
    }
}

void JavaCallHelper::onProgress(int progress, int thread) {
    if (thread == THREAD_CHILD) {
        JNIEnv *env;
        if (javaVm->AttachCurrentThread(&env, 0) != JNI_OK) {
            return;
        }
        env->CallVoidMethod(jobj, jmid_progress, progress);
        javaVm->DetachCurrentThread();
    } else {
        env->CallVoidMethod(jobj, jmid_progress, progress);
    }
}