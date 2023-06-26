//
// Created by liu on 2023/4/24.
//

#ifndef GB28181AND_ISIPEVENTHANDLER_H
#define GB28181AND_ISIPEVENTHANDLER_H

class ISipCallBack{
public:
    virtual ~ISipCallBack(){}
    virtual void registerResult(bool success){}
    virtual void mediaControl(const char*cmd, const char*media_sever_ip, const char*media_sever_port){}
    virtual void audioRtmpUrl(const char *url){}
};

#endif //GB28181AND_ISIPEVENTHANDLER_H
