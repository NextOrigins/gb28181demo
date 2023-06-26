//
// Created by liu on 2023/4/24.
//
#include "SipClientListener.h"
#include "dev_util/JniHelper.h"

SipClientListener::SipClientListener(jobject event):
        m_jJavaObj(NULL),
        m_jClass(NULL)
{
    if (event){
        JNIEnv* jniEnv = AppCoder::JniHelper::getEnv();
        m_jJavaObj = jniEnv->NewGlobalRef(event);
        m_jClass = reinterpret_cast<jclass> (jniEnv->NewGlobalRef(
                jniEnv->GetObjectClass(m_jJavaObj)));
    }
}

SipClientListener::~SipClientListener(){
    if (m_jJavaObj) {
        JNIEnv* jniEnv = AppCoder::JniHelper::getEnv();
        m_jClass = NULL;
        jniEnv->DeleteGlobalRef(m_jJavaObj);
        m_jJavaObj = NULL;
    }
}

void SipClientListener::registerResult(bool success){
    JNIEnv* jniEnv = AppCoder::JniHelper::getEnv();
    {
        // Get *** callback interface method id
        jmethodID j_callJavaMId = jniEnv->GetMethodID(m_jClass, "onRegisterResult", "(Z)V");
        // Callback with param
        jniEnv->CallVoidMethod(m_jJavaObj, j_callJavaMId,success);
    }
}

void SipClientListener::mediaControl(const char*cmd, const char*media_sever_ip, const char*media_sever_port){
    JNIEnv* jniEnv = AppCoder::JniHelper::getEnv();

    jstring jstrCmd=  AppCoder::JniHelper::charToJstring(jniEnv,cmd);
    jstring jStrMediaServer=  AppCoder::JniHelper::charToJstring(jniEnv,media_sever_ip);
    jstring jStrMediaPort=  AppCoder::JniHelper::charToJstring(jniEnv,media_sever_port);
    jmethodID methodId = jniEnv->GetMethodID(m_jClass, "onMediaControl", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");

    jniEnv->CallVoidMethod(m_jJavaObj, methodId, jstrCmd, jStrMediaServer,jStrMediaPort);
// 释放Java字符串
    jniEnv->DeleteLocalRef(jstrCmd);
    jniEnv->DeleteLocalRef(jStrMediaServer);
    jniEnv->DeleteLocalRef(jStrMediaPort);
}

void SipClientListener::audioRtmpUrl(const char *url) {
    JNIEnv *pEnv = AppCoder::JniHelper::getEnv();

    jstring jstrUrl=  AppCoder::JniHelper::charToJstring(pEnv,url);
    jmethodID methodId = pEnv->GetMethodID(m_jClass, "onRtmpUrl", "(Ljava/lang/String;)V");

    pEnv->CallVoidMethod(m_jJavaObj, methodId, jstrUrl);
// 释放Java字符串
    pEnv->DeleteLocalRef(jstrUrl);
}