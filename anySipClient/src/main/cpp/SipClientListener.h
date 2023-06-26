//
// Created by liu on 2023/4/24.
//

#ifndef GB28181AND_SIPCLIENTLISTENER_H
#define GB28181AND_SIPCLIENTLISTENER_H
#include "ISipEventHandler.h"
#include "jni.h"
class SipClientListener : public ISipCallBack{
public:
    SipClientListener(jobject listener);
    ~SipClientListener();

public:
    void registerResult(bool success);
    void mediaControl(const char*cmd, const char*media_sever_ip, const char*media_sever_port);
protected:
    jobject m_jJavaObj;
    jclass m_jClass;

    void audioRtmpUrl(const char *url);
};

#endif //GB28181AND_SIPCLIENTLISTENER_H
