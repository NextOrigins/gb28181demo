
#include <jni.h>
#include <string>
#include <android/log.h>
#include <cstdlib>
#include <pthread.h>
#include "include/common.h"
#include "include/eXosip2/eXosip.h"
#include <unistd.h>
#include <chrono>

#ifndef OS_GLOBALS
#define OS_GLOBALS

#include "include/gb_lib.h"

#endif
extern "C" {
extern DT_EXOSIP_CALLBACK dt_eXosip_callback;
extern DEVICE_INFO device_info;
extern DEVICE_STATUS device_status;
//extern BOOL SenAliveFlag;
extern int g_call_id;/*INVITE连接ID/用来分辨不同的INVITE连接，每个时刻只允许有一个INVITE连接*/
extern int g_register_id;/*注册ID/用来更新注册或取消注册*/
extern int g_did_realPlay;/*会话ID/用来分辨不同的会话：实时视音频点播*/
extern int g_did_backPlay;/*会话ID/用来分辨不同的会话：历史视音频回放*/
extern int g_did_fileDown;/*会话ID/用来分辨不同的会话：视音频文件下载*/

struct SipHolder {
    eXosip_t *eXosipContext = nullptr;
    ISipCallBack *mSipCallback = nullptr;
    bool wait = false;
};

SipHolder *sipHolder;
unsigned int SN = 1, wait_count = 0;
pthread_t keep_alive_thread = NULL;
char target_id[32];

char *my_call_id_new_random();
char *my_branch_new_unique();
void dt_eXosip_send_message(eXosip_t *context, char *body);
void send_broadcast_invite_to_server(eXosip_t *context);
void send_broadcast_ack_to_server(eXosip_t *context, eXosip_event_t *p_event);

void *startKeepAlive(void *) {
    while (sipHolder->eXosipContext) {
        if (wait_count++ > 8)
        {
            //eXosip_event_free(g_event);
            //g_event = nullptr;

            ISipCallBack *callBack = sipHolder->mSipCallback;
            eXosip_quit(sipHolder->eXosipContext);
            delete sipHolder;
            dt_eXosip_rigister(callBack);
            __android_log_print(ANDROID_LOG_ERROR, "JNI", " ~~~ Keep Alive Failed. Re-register. ~~~ ");
            break;
        }
        __android_log_print(ANDROID_LOG_ERROR, "JNI", " ~~~ Keep Alive Sent. ~~~ ");
        sleep(40);
        dt_eXosip_keepAlive(sipHolder->eXosipContext);
    }
    pthread_exit(nullptr);
}

/*void eXosip_sendLocation(double latitude, double longitude, int altitude, double direction) {
    if (!sipHolder) {
        __android_log_print(ANDROID_LOG_ERROR, "JNI", "Send Location Failed. sipHolder is null.");
        return;
    }

    eXosip_t *context = sipHolder->eXosipContext;
    osip_message_t *rqt_msg = nullptr;
    char to[100];
    char from[100];
    char xml_body[4096];*/
    /*
      <?xml version="1.0"?>
      <Notify>
      <CmdType>UpdateLocation</CmdType>
      <SN>6987234</SN>
      <DeviceID>N9519060</DeviceID>
      <Speed>80km/h</Speed>
      <Status>開機/停機</Status>
      <Longitude>126° 36′ 36″E</Longitude>
      <Latitude>45° 27′ 36″N</Latitude>
      <Altitude>1903m</Altitude>
      <Direction>西面</Direction>
      </Notify>
     */

    /*memset(to, 0, 100);
    memset(from, 0, 100);
    memset(xml_body, 0, 4096);
    snprintf(to, sizeof(to), "sip:%s@%s:%s", device_info.server_id, device_info.server_ip,
             device_info.server_port);
    snprintf(from, sizeof(from), "sip:%s@%s:%s", device_info.ipc_id, device_info.ipc_ip,
             device_info.ipc_sess_port);
    eXosip_message_build_request(context, &rqt_msg, "MESSAGE", to, from, nullptr);
    snprintf(xml_body, 4096, R"(<?xml version="1.0" encoding="utf-8"?>)"
                             "<Notify>\r\n"
                             "<CmdType>MobilePosition</CmdType>\r\n"
                             "<SN>%d</SN>\r\n"
                             "<DeviceID>%s</DeviceID>\r\n"
                             "<Speed>0km/h</Speed>\r\n"
                             "<Status>开机</Status>\r\n"
                             "<Longitude>%f</Longitude>\r\n"
                             "<Latitude>%f</Latitude>\r\n"
                             "<Altitude>%d</Altitude>\r\n"
                             "<Direction>%f</Direction>\r\n"
                             "</Notify>\r\n", SN++, device_info.ipc_id, longitude, latitude,
             altitude, direction);
    osip_message_set_body(rqt_msg, xml_body, strlen(xml_body));
    osip_message_set_content_type(rqt_msg, "Application/MANSCDP+xml");
    eXosip_lock(context);
    eXosip_message_send_request(context, rqt_msg);
    eXosip_unlock(context);
    LOGD("dt_eXosip_sendKeepAlive success!");
}*/

int dt_eXosip_rigister(ISipCallBack *sipCallBack) {
    sipHolder = new SipHolder();
    sipHolder->mSipCallback = sipCallBack;
    sipHolder->eXosipContext = eXosip_malloc();
    if (!sipHolder->eXosipContext) {
        LOGE("exosip malloc failed!");
    }
    //初始化exosip
    if (dt_eXosip_init(sipHolder->eXosipContext, device_info) == -1) {
        LOGE("eXosip创建失败！");
        return -1;
    }
    LOGE("eXosip创建成功！");
    if (dt_eXosip_register(sipHolder->eXosipContext, 3600, device_info) == 0) {
        LOGD("use timer in workthread of console application\n");
        sipHolder->wait = true;
        dt_eXosip_processEvent(sipHolder->eXosipContext, device_info);
    }

    return 0;
}

int dt_eXosip_unregister() {
    if (sipHolder->eXosipContext) {
        eXosip_quit(sipHolder->eXosipContext);
    }
    if (g_register_id) {
    }
    sipHolder->wait = false;
    delete sipHolder->mSipCallback;
    return 0;
}

/*初始化eXosip*/
int dt_eXosip_init(eXosip_t *context, DEVICE_INFO device_info) {
    int ret = 0;
    //初始化osip和eXosip协议栈
    ret = eXosip_init(context);
    if (0 != ret) {
        LOGE("Couldn't initialize eXosip!");
        return -1;
    }
    LOGD("eXosip_init success!");
    ret = eXosip_listen_addr(context,
                             IPPROTO_UDP,
                             nullptr,
                             atoi(device_info.ipc_sess_port),
                             AF_INET,
                             0);
    LOGE("eXosip_listen_addr:ret=%d", ret);
    if (0 != ret)/*传输层初始化失败*/
    {
        eXosip_quit(context);
        LOGE("eXosip_listen_addr error!");
        return -1;
    }
    LOGD("eXosip_listen_addr success!");
    return 0;
}

/**
 * 注册SIP服务
 * @param context 上下文
 * @param expires  注册消息过期时间，单位为秒
 * @return
 */
int dt_eXosip_register(eXosip_t *context, int expires, DEVICE_INFO device_info) {
    eXosip_event_t *je;
    osip_message_t *reg = nullptr;
    char from[100];/*sip:主叫用户名@被叫IP地址*/
    char proxy[100];/*sip:被叫IP地址:被叫IP端口*/
    memset(from, 0, sizeof(from));
    memset(proxy, 0, sizeof(proxy));

    snprintf(from, sizeof(from), "sip:%s@%s:%s", device_info.ipc_id, device_info.ipc_ip,
             device_info.ipc_sess_port);
    LOGD("sip:%s@%s:%s", device_info.ipc_id, device_info.ipc_ip, device_info.ipc_sess_port);
    snprintf(proxy, sizeof(proxy), "sip:%s@%s:%s", device_info.server_id, device_info.server_ip,
             device_info.server_port);
    LOGD("sip:%s@%s:%s", device_info.server_id, device_info.server_ip, device_info.server_port);
    /*发送不带认证信息的注册请求*/
    retry:
    eXosip_masquerade_contact(context, device_info.ipc_ip, atoi(device_info.ipc_sess_port));
    eXosip_set_user_agent(context, UA_STRING);
    //eXosip_lock(context);
    g_register_id = eXosip_register_build_initial_register(context,
                                                           from,
                                                           proxy,
                                                           nullptr,
                                                           expires,
                                                           &reg);
    //eXosip_unlock(context);
    if (0 > g_register_id) {
        //osip_message_free(reg);
        eXosip_register_remove(context, g_register_id);
        LOGE("Initial build failed.");
        sleep(8);
        goto retry;
    }

    eXosip_lock(context);
    int ret = eXosip_register_send_register(context, g_register_id, reg);
    eXosip_unlock(context);
    LOGD("sip:%s@%s:%s\n", device_info.server_id, device_info.server_ip, device_info.server_port);
    //if (0 != ret) {
    if (ret) {
        eXosip_register_remove(context, g_register_id);
        LOGE("eXosip_register_send_register no authorization error!");
        sleep(8);
        goto retry;
        //return -1;
    }
    LOGD("eXosip_register_send_register no authorization success!");
    LOGD("g_register_id=%d", g_register_id);

    for (;;) {
        je = eXosip_event_wait(context, 0, 50);/*侦听消息的到来*/
        if (!je)/*没有接收到消息*/
        {
            eXosip_execute(context);
            eXosip_automatic_action(context);
            osip_usleep(1000);
            LOGD("osip_waiting message... ");
            if (wait_count++ > 8)
            {
                wait_count = 0;
                goto retry;
            }
            continue;
        }

        LOGE("Receiving message... response->type=%s",
             je->type == EXOSIP_REGISTRATION_SUCCESS ? "EXOSIP_REGISTRATION_SUCCESS"
                                                     : "EXOSIP_REGISTRATION_FAILURE");
        if (je->type == EXOSIP_REGISTRATION_SUCCESS) {
            /*收到服务器返回的注册成功*/
            wait_count = 0;
            eXosip_execute(context);
            eXosip_automatic_action(context);
            LOGD("<EXOSIP_REGISTRATION_SUCCESS>");
            g_register_id = je->rid;/*保存注册成功的注册ID*/
            LOGD("g_register_id=%d", g_register_id);
            __android_log_print(ANDROID_LOG_ERROR, "JNI",
                                "Hello. This is register for the service successfully.");
            LOGE("register successfully text_info=%s", je->textinfo);
            //if (sipHolder->mSipCallback!=NULL){
            if (sipHolder->mSipCallback) {
                sipHolder->mSipCallback->registerResult(true);
            }
            if (keep_alive_thread) {
                pthread_kill(keep_alive_thread, 0);
                keep_alive_thread = NULL;
            }
            pthread_create(&keep_alive_thread, nullptr, startKeepAlive, (void *) nullptr);
            //eXosip_set_option(context, EXOSIP_OPT_UDP_KEEP_ALIVE, "1");

            break;
        } else if (je->type == EXOSIP_REGISTRATION_FAILURE)/*注册失败*/
        {
            LOGD("<EXOSIP_REGISTRATION_FAILURE>");
            /*收到服务器返回的注册失败/401未认证状态*/
            if (je->response) {
                __android_log_print(
                        ANDROID_LOG_DEBUG, "JNI",
                        "--- status code = %d ---", je->response->status_code);
                if (je->response->status_code == 401) {
                    /*发送携带认证信息的注册请求*/
                    eXosip_lock(context);
                    /*清除认证信息*/
                    eXosip_clear_authentication_info(context);
                    /* 添加Authorization头部字段 */
                    eXosip_add_authentication_info(context, device_info.ipc_id, device_info.ipc_id,
                                                   device_info.ipc_pwd, "MD5",
                                                   nullptr);/*添加主叫用户的认证信息*/
                    eXosip_register_build_register(context, je->rid, expires, &reg);
                    ret = eXosip_register_send_register(context, je->rid, reg);
                    eXosip_unlock(context);
                    if (ret) {
                        LOGE("eXosip_register_send_register authorization error!");
                        sleep(6);
                        goto retry;
                    }
                    LOGE("eXosip_register_send_register authorization success!");
                } else if (je->response->status_code == 407) // proxy
                {
                } else {
                    LOGE("==== EXOSIP_REGISTRATION_FAILURE error! status code=%d, textinfo=%s",
                         je->request->status_code, je->textinfo);
                    if (sipHolder->mSipCallback) {
                        sipHolder->mSipCallback->registerResult(false);
                    }
                    sleep(3);
                    goto retry;/*重新注册*/
                }
            } else/*真正的注册失败*/
            {
                LOGE("EXOSIP_REGISTRATION_FAILURE error! text info = %s", je->textinfo);
                if (sipHolder->mSipCallback) {
                    sipHolder->mSipCallback->registerResult(false);
                }
                sleep(3);
                goto retry;/*重新注册*/
            }
        }
    }
    if (reg) {
        LOGD("Free reg.");
        //osip_message_free(reg);
    }
    //eXosip_event_free(je);
    return 0;
}

/*消息循环处理*/
/*EXOSIP_MESSAGE_NEW-->EXOSIP_MESSAGE_ANSWERED-->EXOSIP_CALL_INVITE-->EXOSIP_CALL_ACK-->EXOSIP_MESSAGE_ANSWERED*/
void dt_eXosip_processEvent(eXosip_t *context, DEVICE_INFO device_info) {
    eXosip_event_t *g_event;/*消息事件*/
    osip_message_t *g_answer = nullptr;/*请求的确认型应答*/
    while (sipHolder->wait) {

        /*等待新消息的到来*/
        g_event = eXosip_event_wait(context, 0, 200);/*侦听消息的到来*/
        eXosip_lock(context);
        eXosip_default_action(context, g_event);
        //eXosip_automatic_refresh(context);/*Refresh REGISTER and SUBSCRIBE before the expiration delay*/
        eXosip_unlock(context);
        //检查是否发心跳信息
        /*if (SenAliveFlag) {
            dt_eXosip_keepAlive(context);
            SenAliveFlag = FALSE;
        }*/
        if (!g_event) {
            continue;
        }
        auto released = dt_eXosip_printEvent(context, g_event);
        if (released) {
            __android_log_print(ANDROID_LOG_ERROR, "JNI", " --- Server Released. --- ");
            //eXosip_event_free(g_event);
            //g_event = nullptr;

            ISipCallBack *callBack = sipHolder->mSipCallback;
            eXosip_quit(sipHolder->eXosipContext);
            delete sipHolder;
            dt_eXosip_rigister(callBack);
            return;
        }

        /* print */
        /*__android_log_print(ANDROID_LOG_ERROR, "JNI", "After `dt_eXosip_printEvent`.");
        try {
            __android_log_print(ANDROID_LOG_ERROR, "JNI", " --- Before `osip_message_get_body` --- ");
            osip_body_t  *p_rqt_body;
            osip_message_get_body(g_event->request, 0, &p_rqt_body);// 获取接收到请求的XML消息体
            __android_log_print(ANDROID_LOG_DEBUG, "JIN", "%s", p_rqt_body->body);
            __android_log_print(ANDROID_LOG_ERROR, "JNI", " --- After `osip_message_get_body` --- ");
        } catch (...) {
            __android_log_print(ANDROID_LOG_ERROR, "JIN", " --- Caught exception!!!");
            try {
                const std::exception_ptr &ptr = std::current_exception();
                rethrow_exception(ptr);
            } catch (const std::exception &e) {
                __android_log_print(ANDROID_LOG_ERROR, "JIN", " --- Caught exception: %s --- ", e.what());
            }
            //__android_log_print(ANDROID_LOG_ERROR, "JIN", " --- Caught exception: %s --- ", e.what());
        }*/
        /*       */
        /*size_t size = 1 + snprintf(nullptr, 0, "event->type=%d", g_event->type);
        char s[size];
        snprintf(s, size, "event->type=%d", g_event->type);
        __android_log_print(ANDROID_LOG_ERROR, "JNI", " --- After `osip_message_get_body`2 --- ");*/
        __android_log_print(ANDROID_LOG_ERROR, "JNI", "event->type=%d, request->method=%s",
                            g_event->type, g_event->request->sip_method);

        /*处理感兴趣的消息*/
        switch (g_event->type) {
            /*即时消息：通信双方无需事先建立连接*/
            case EXOSIP_MESSAGE_NEW:/*MESSAGE:MESSAGE*/
                LOGD("<EXOSIP_MESSAGE_NEW>");
                if (MSG_IS_MESSAGE(g_event->request))/*使用MESSAGE方法的请求*/
                {
                    /*设备控制*/
                    /*报警事件通知和分发：报警通知响应*/
                    /*网络设备信息查询*/
                    /*设备视音频文件检索*/
                    LOGD("<MSG_IS_MESSAGE>");
                    eXosip_lock(context);
                    eXosip_message_build_answer(context, g_event->tid, 200,
                                                &g_answer);/*Build default Answer for request*/
                    eXosip_message_send_answer(context, g_event->tid, 200, g_answer);/*按照规则回复200OK*/
                    LOGD("eXosip_message_send_answer success!");
                    eXosip_unlock(context);
                    dt_eXosip_paraseMsgBody(context, g_event);/*解析MESSAGE的XML消息体，同时保存全局会话ID*/
                }
                break;

            case EXOSIP_MESSAGE_ANSWERED:/*即时消息回复的200OK*/
                /*设备控制*/
                /*报警事件通知和分发：报警通知*/
                /*网络设备信息查询*/
                /*设备视音频文件检索*/
                LOGD("<EXOSIP_MESSAGE_ANSWERED>");
                char *msg;
                size_t len;
                if (g_event->request)
                {
                    osip_message_to_str(g_event->request, &msg, &len);
                    char *p_keep_alive = strstr(msg, "<CmdType>Keepalive</CmdType>");
                    if (p_keep_alive)
                        wait_count--;

                    free(msg);
                }
                /*if (g_event->response)
                {
                    osip_message_to_str(g_event->response, &msg, &len);
                    LOGD("<EXOSIP_MESSAGE_ANSWERED> Response <EXOSIP_MESSAGE_ANSWERED>");
                    LOGD("len=%ld\n%s", len, msg);
                    LOGD("<EXOSIP_MESSAGE_ANSWERED> Response <EXOSIP_MESSAGE_ANSWERED>");
                    free(msg);
                }*/


                break;
                /*以下类型的消息都必须事先建立连接*/
            case EXOSIP_CALL_INVITE:/*INVITE*/
                LOGD("<EXOSIP_CALL_INVITE>");
                if (MSG_IS_INVITE(g_event->request))/*使用INVITE方法的请求*/
                {
                    /*实时视音频点播*/
                    /*历史视音频回放*/
                    /*视音频文件下载*/
                    osip_message_t *asw_msg;/*请求的确认型应答*/
                    char sdp_body[4096];

                    memset(sdp_body, 0, 4096);
                    LOGD("<MSG_IS_INVITE>");

                    eXosip_lock(context);
                    if (0 != eXosip_call_build_answer(context, g_event->tid, 200,
                                                      &asw_msg))/*Build default Answer for request*/
                    {
                        eXosip_call_send_answer(context, g_event->tid, 603, nullptr);
                        eXosip_unlock(context);
                        LOGD("eXosip_call_build_answer error!");
                        break;
                    }
                    eXosip_unlock(context);
                    snprintf(sdp_body, 4096, "v=0\r\n"/*协议版本*/
                                             "o=%s 0 0 IN IP4 %s\r\n"/*会话源*//*用户名/会话ID/版本/网络类型/地址类型/地址*/
                                             "s=(null)\r\n"/*会话名*/
                                             "c=IN IP4 %s"/*连接信息*//*网络类型/地址信息/多点会议的地址*/
                                             "t=0 0\r\n"/*时间*//*开始时间/结束时间*/
                                             "m=video %s RTP/AVP 96\r\n"/*媒体/端口/传送层协议/格式列表*/
                                             "a=sendonly\r\n"/*收发模式*/
                                             "a=rtpmap:96 PS/90000\r\n"/*净荷类型/编码名/时钟速率*/
                                             "a=username:%s\r\n"
                                             "a=password:%s\r\n"
                                             "y=0999999999\r\n"
                                             "f=\r\n",
                             device_info.ipc_id,
                             device_info.ipc_ip,
                             device_info.ipc_ip,
                             device_info.ipc_media_port,
                             device_info.ipc_id,
                             device_info.ipc_pwd);
                    eXosip_lock(context);
                    osip_message_set_body(asw_msg, sdp_body, strlen(sdp_body));/*设置SDP消息体*/
                    osip_message_set_content_type(asw_msg, "application/sdp");
                    eXosip_call_send_answer(context, g_event->tid, 200,
                                            asw_msg);/*按照规则回复200OK with SDP*/
                    LOGD("eXosip_call_send_answer success!");
                    eXosip_unlock(context);
                    //osip_message_free(asw_msg);
                }
                break;
            case EXOSIP_CALL_ACK:/*ACK*/
                /*实时视音频点播*/
                /*历史视音频回放*/
                /*视音频文件下载*/
                LOGD("<EXOSIP_CALL_ACK>");/*收到ACK才表示成功建立连接*/
                dt_eXosip_paraseInviteBody(context,
                                           g_event);/*解析INVITE的SDP消息体，同时保存全局INVITE连接ID和全局会话ID*/

                break;
            case EXOSIP_CALL_CLOSED:/*BEY*/

                /*实时视音频点播*/
                /*历史视音频回放*/
                /*视音频文件下载*/
                LOGD("<EXOSIP_CALL_CLOSED>");
                if (MSG_IS_BYE(g_event->request)) {
                    LOGD("<MSG_IS_BYE>");
                    if ((0 != g_did_realPlay) && (g_did_realPlay == g_event->did))/*实时视音频点播*/
                    {
                        /*关闭：实时视音频点播*/
                        LOGD("realPlay closed success!");
                        g_did_realPlay = 0;
                    } else if ((0 != g_did_backPlay) && (g_did_backPlay == g_event->did))/*历史视音频回放*/
                    {
                        /*关闭：历史视音频回放*/
                        LOGD("backPlay closed success!");
                        g_did_backPlay = 0;
                    } else if ((0 != g_did_fileDown) && (g_did_fileDown == g_event->did))/*视音频文件下载*/
                    {
                        /*关闭：视音频文件下载*/
                        LOGD("fileDown closed success!");
                        g_did_fileDown = 0;
                    }
                    if ((0 != g_call_id) && (0 == g_did_realPlay) && (0 == g_did_backPlay) &&
                        (0 == g_did_fileDown))/*设置全局INVITE连接ID*/
                    {
                        LOGD("call closed success!");
                        g_call_id = 0;
                    }
                }

                break;
            case EXOSIP_CALL_MESSAGE_NEW:/*MESSAGE:INFO*/
                /*历史视音频回放*/
                LOGD("<EXOSIP_CALL_MESSAGE_NEW>");
                if (MSG_IS_INFO(g_event->request)) {
                    osip_body_t *msg_body = NULL;

                    LOGD("<MSG_IS_INFO>");
                    osip_message_get_body(g_event->request, 0, &msg_body);
                    if (NULL != msg_body) {
                        eXosip_call_build_answer(context, g_event->tid, 200,
                                                 &g_answer);/*Build default Answer for request*/
                        eXosip_call_send_answer(context, g_event->tid, 200,
                                                g_answer);/*按照规则回复200OK*/
                        LOGD("eXosip_call_send_answer success!");
                        dt_eXosip_paraseInfoBody(context, msg_body);/*解析INFO的RTSP消息体*/
                    } else {
                        LOGD("osip_message_get_body null!");
                    }
                }
                break;
            case EXOSIP_CALL_MESSAGE_ANSWERED:/*200OK*/
                /*历史视音频回放*/
                /*文件结束时发送MESSAGE(File to end)的应答*/
                LOGD("<EXOSIP_CALL_MESSAGE_ANSWERED>");
                break;
                /*其它不感兴趣的消息*/
            default:
                LOGD("<OTHER>");
                LOGD("eXosip event type:%d\n", g_event->type);
                break;
        }
        //eXosip_event_free(g_event);
    }


}

/*解析INFO的RTSP消息体*/
void dt_eXosip_paraseInfoBody(eXosip_t *context, osip_body_t *p_msg_body) {
    char *p_rtsp_body;
    char *p_str_begin = NULL;
    char *p_str_end = NULL;
    char *p_strstr = NULL;
    char rtsp_scale[10];
    char rtsp_range_begin[10];
    char rtsp_range_end[10];
    char rtsp_pause_time[10];

    memset(rtsp_scale, 0, 10);
    memset(rtsp_range_begin, 0, 10);
    memset(rtsp_range_end, 0, 10);
    memset(rtsp_pause_time, 0, 10);

    //osip_message_get_body(p_event->request, 0, &p_msg_body);
    p_rtsp_body = p_msg_body->body;
    LOGD("osip_message_get_body success!");

    p_strstr = strstr(p_rtsp_body, "PLAY");
    if (NULL != p_strstr)/*查找到字符串"PLAY"*/
    {
        /*播放速度*/
        p_str_begin = strstr(p_rtsp_body, "Scale:");/*查找字符串"Scale:"*/
        p_str_end = strstr(p_rtsp_body, "Range:");
        memcpy(rtsp_scale, p_str_begin + 6, p_str_end - p_str_begin - 6);/*保存Scale到rtsp_scale*/
        LOGD("PlayScale:%s", rtsp_scale);
        /*播放范围*/
        p_str_begin = strstr(p_rtsp_body, "npt=");/*查找字符串"npt="*/
        p_str_end = strstr(p_rtsp_body, "-");
        memcpy(rtsp_range_begin, p_str_begin + 4,
               p_str_end - p_str_begin - 4);/*保存RangeBegin到rtsp_range_begin*/
        LOGD("PlayRangeBegin:%s", rtsp_range_begin);
        p_str_begin = strstr(p_rtsp_body, "-");/*查找字符串"-"*/
        strncpy(rtsp_range_end, p_str_begin + 1,
                sizeof(rtsp_range_end));/*保存RangeEnd到rtsp_range_end*/
        LOGD("PlayRangeEnd:%s", rtsp_range_end);
        dt_eXosip_callback.dt_eXosip_playControl("PLAY", rtsp_scale, NULL, rtsp_range_begin,
                                                 rtsp_range_end);
        return;
    }

    p_strstr = strstr(p_rtsp_body, "PAUSE");
    if (NULL != p_strstr)/*查找到字符串"PAUSE"*/
    {
        /*暂停时间*/
        p_str_begin = strstr(p_rtsp_body, "PauseTime:");/*查找字符串"PauseTime:"*/
        strncpy(rtsp_pause_time, p_str_begin + 10,
                sizeof(rtsp_pause_time));/*保存PauseTime到rtsp_pause_time*/
        LOGD("PauseTime:%3s", rtsp_pause_time);
        dt_eXosip_callback.dt_eXosip_playControl("PAUSE", NULL, rtsp_pause_time, NULL, NULL);
        return;
    }

    LOGD("can`t find string PLAY or PAUSE!");
}

/*解析MESSAGE的XML消息体*/
void dt_eXosip_paraseMsgBody(eXosip_t *context, eXosip_event_t *p_event) {
    /*与请求相关的变量*/
    osip_body_t *p_rqt_body = NULL;
    char *p_xml_body = NULL;
    char *p_str_begin = NULL;
    char *p_str_end = NULL;
    char xml_cmd_type[20];
    char xml_cmd_sn[10];
    char xml_device_id[30];
    char xml_command[30];
    /*与回复相关的变量*/
    osip_message_t *rsp_msg = NULL;
    char to[100];/*sip:主叫用户名@被叫IP地址*/
    char from[100];/*sip:被叫IP地址:被叫IP端口*/
    //char cut_temp[11];
    char rsp_xml_body[4096];

    memset(xml_cmd_type, 0, 20);
    memset(xml_cmd_sn, 0, 10);
    memset(xml_device_id, 0, 30);
    memset(xml_command, 0, 30);
    memset(to, 0, 100);
    memset(from, 0, 100);
    //memset(cut_temp, 0, 11);
    memset(rsp_xml_body, 0, 4096);

    /*strncpy(cut_temp, device_info.server_id, 10);
    cut_temp[10] = '\0';
    snprintf(to, sizeof to, "sip:%s@%s", device_info.server_id, cut_temp);
    strncpy(cut_temp, device_info.ipc_id, 10);
    snprintf(from, sizeof from, "sip:%s@%s", device_info.ipc_id, cut_temp);*/

    snprintf(to, sizeof(to), "sip:%s@%s:%s", device_info.server_id, device_info.server_ip,
             device_info.server_port);
    snprintf(from, sizeof(from), "sip:%s@%s:%s", device_info.ipc_id, device_info.ipc_ip,
             device_info.ipc_sess_port);
    eXosip_message_build_request(context, &rsp_msg, "MESSAGE", to, from, nullptr);/*构建"MESSAGE"请求*/

    osip_message_get_body(p_event->request, 0, &p_rqt_body);/*获取接收到请求的XML消息体*/
    if (NULL == p_rqt_body) {
        LOGD("osip_message_get_body null!\r\n");
        return;
    }
    p_xml_body = p_rqt_body->body;
    LOGD("osip_message_get_body success!\r\n");

    LOGD("**********CMD START**********\r\n");
    p_str_begin = strstr(p_xml_body, "<CmdType>");/*查找字符串"<CmdType>"*/
    p_str_end = strstr(p_xml_body, "</CmdType>");
    memcpy(xml_cmd_type, p_str_begin + 9, p_str_end - p_str_begin - 9);/*保存<CmdType>到xml_cmd_type*/
    LOGD("<CmdType>:%s\r\n", xml_cmd_type);

    p_str_begin = strstr(p_xml_body, "<SN>");/*查找字符串"<SN>"*/
    p_str_end = strstr(p_xml_body, "</SN>");
    memcpy(xml_cmd_sn, p_str_begin + 4, p_str_end - p_str_begin - 4);/*保存<SN>到xml_cmd_sn*/
    LOGD("<SN>:%s\r\n", xml_cmd_sn);

    LOGE("Finding DeviceID...");
    p_str_begin = strstr(p_xml_body, "<DeviceID>");/*查找字符串"<DeviceID>"*/
    p_str_end = strstr(p_xml_body, "</DeviceID>");
    if (p_str_begin && p_str_end) {
        memcpy(xml_device_id, p_str_begin + 10,
               p_str_end - p_str_begin - 10);/*保存<DeviceID>到xml_device_id*/
    } else {
        strcpy(xml_device_id, "0");
        *(xml_device_id + 1) = '\0';
    }
    LOGD("<DeviceID>:%s\r\n", xml_device_id);
    LOGD("***********CMD END***********\r\n");

    if (0 == strcmp(xml_cmd_type, "DeviceControl"))/*设备控制*/
    {
        LOGD("**********CONTROL START**********\r\n");
        /*向左、向右、向上、向下、放大、缩小、停止遥控*/
        p_str_begin = strstr(p_xml_body, "<PTZCmd>");/*查找字符串"<PTZCmd>"*/
        if (NULL != p_str_begin) {
            p_str_end = strstr(p_xml_body, "</PTZCmd>");
            memcpy(xml_command, p_str_begin + 8,
                   p_str_end - p_str_begin - 8);/*保存<PTZCmd>到xml_command*/
            LOGD("<PTZCmd>:%s\r\n", xml_command);
            goto DeviceControl_Next;
        }
        /*开始手动录像、停止手动录像*/
        p_str_begin = strstr(p_xml_body, "<RecordCmd>");/*查找字符串"<RecordCmd>"*/
        if (NULL != p_str_begin) {
            p_str_end = strstr(p_xml_body, "</RecordCmd>");
            memcpy(xml_command, p_str_begin + 11,
                   p_str_end - p_str_begin - 11);/*保存<RecordCmd>到xml_command*/
            LOGD("<RecordCmd>:%s\r\n", xml_command);
            goto DeviceControl_Next;
        }
        /*布防、撤防*/
        p_str_begin = strstr(p_xml_body, "<GuardCmd>");/*查找字符串"<GuardCmd>"*/
        if (NULL != p_str_begin) {
            p_str_end = strstr(p_xml_body, "</GuardCmd>");
            memcpy(xml_command, p_str_begin + 10,
                   p_str_end - p_str_begin - 10);/*保存<GuardCmd>到xml_command*/
            LOGD("<GuardCmd>:%s\r\n", xml_command);
            goto DeviceControl_Next;
        }
        /*报警复位：30秒内不再触发报警*/
        p_str_begin = strstr(p_xml_body, "<AlarmCmd>");/*查找字符串"<AlarmCmd>"*/
        if (NULL != p_str_begin) {
            p_str_end = strstr(p_xml_body, "</AlarmCmd>");
            memcpy(xml_command, p_str_begin + 10,
                   p_str_end - p_str_begin - 10);/*保存<AlarmCmd>到xml_command*/
            LOGD("<AlarmCmd>:%s\r\n", xml_command);
            goto DeviceControl_Next;
        }
        /*设备远程启动*/
        p_str_begin = strstr(p_xml_body, "<TeleBoot>");/*查找字符串"<TeleBoot>"*/
        if (NULL != p_str_begin) {
            p_str_end = strstr(p_xml_body, "</TeleBoot>");
            memcpy(xml_command, p_str_begin + 10,
                   p_str_end - p_str_begin - 10);/*保存<TeleBoot>到xml_command*/
            LOGD("<TeleBoot>:%s\r\n", xml_command);
            goto DeviceControl_Next;
        }
        DeviceControl_Next:
        LOGD("***********CONTROL END***********\r\n");
        if (0 == strcmp(xml_command, "A50F01021F0000D6"))/*向左*/
        {
            dt_eXosip_callback.dt_eXosip_deviceControl(
                    EXOSIP_CTRL_RMT_LEFT);//调用dt_eXosip_deviceControl函数
        } else if (0 == strcmp(xml_command, "A50F01011F0000D5"))/*向右*/
        {
            dt_eXosip_callback.dt_eXosip_deviceControl(EXOSIP_CTRL_RMT_RIGHT);
        } else if (0 == strcmp(xml_command, "A50F0108001F00DC"))/*向上*/
        {
            dt_eXosip_callback.dt_eXosip_deviceControl(EXOSIP_CTRL_RMT_UP);
        } else if (0 == strcmp(xml_command, "A50F0104001F00D8"))/*向下*/
        {
            dt_eXosip_callback.dt_eXosip_deviceControl(EXOSIP_CTRL_RMT_DOWN);
        } else if (0 == strcmp(xml_command, "A50F0110000010D5"))/*放大*/
        {
            dt_eXosip_callback.dt_eXosip_deviceControl(EXOSIP_CTRL_RMT_LARGE);
        } else if (0 == strcmp(xml_command, "A50F0120000010E5"))/*缩小*/
        {
            dt_eXosip_callback.dt_eXosip_deviceControl(EXOSIP_CTRL_RMT_SMALL);
        } else if (0 == strcmp(xml_command, "A50F0100000000B5"))/*停止遥控*/
        {
            dt_eXosip_callback.dt_eXosip_deviceControl(EXOSIP_CTRL_RMT_STOP);
        } else if (0 == strcmp(xml_command, "Record"))/*开始手动录像*/
        {
            dt_eXosip_callback.dt_eXosip_deviceControl(EXOSIP_CTRL_REC_START);
        } else if (0 == strcmp(xml_command, "StopRecord"))/*停止手动录像*/
        {
            dt_eXosip_callback.dt_eXosip_deviceControl(EXOSIP_CTRL_REC_STOP);
        } else if (0 == strcmp(xml_command, "SetGuard"))/*布防*/
        {
            dt_eXosip_callback.dt_eXosip_deviceControl(EXOSIP_CTRL_GUD_START);
            //strncpy(device_status.status_guard,strlen("ONDUTY"), "ONDUTY");/*设置布防状态为"ONDUTY"*/
        } else if (0 == strcmp(xml_command, "ResetGuard"))/*撤防*/
        {
            dt_eXosip_callback.dt_eXosip_deviceControl(EXOSIP_CTRL_GUD_STOP);
            strncpy(device_status.status_guard, "OFFDUTY", strlen("OFFDUTY"));/*设置布防状态为"OFFDUTY"*/
        } else if (0 == strcmp(xml_command, "ResetAlarm"))/*报警复位*/
        {
            dt_eXosip_callback.dt_eXosip_deviceControl(EXOSIP_CTRL_ALM_RESET);
        } else if (0 == strcmp(xml_command, "Boot"))/*设备远程启动*/
        {
            dt_eXosip_callback.dt_eXosip_deviceControl(EXOSIP_CTRL_TEL_BOOT);
        } else {
            LOGD("unknown device control command!\r\n");
        }
        snprintf(rsp_xml_body, 4096, "<?xml version=\"1.0\"?>\r\n"
                                     "<Response>\r\n"
                                     "<CmdType>DeviceControl</CmdType>\r\n"/*命令类型*/
                                     "<SN>%s</SN>\r\n"/*命令序列号*/
                                     "<DeviceID>%s</DeviceID>\r\n"/*目标设备/区域/系统编码*/
                                     "<Result>OK</Result>\r\n"/*执行结果标志*/
                                     "</Response>\r\n",
                 xml_cmd_sn,
                 xml_device_id);
    } else if (0 == strcmp(xml_cmd_type, "Alarm"))/*报警事件通知和分发：报警通知响应*/
    {
        LOGD("**********ALARM START**********\r\n");
        /*报警通知响应*/
        LOGD("local eventAlarm response success!\n");
        LOGD("***********ALARM END***********\r\n");
        return;
    } else if (0 == strcmp(xml_cmd_type, "Catalog"))/*网络设备信息查询：设备目录查询*/
    {
        LOGD("**********CATALOG START**********\r\n");
        /*设备目录查询*/
        LOGD("***********CATALOG END***********\r\n");
        snprintf(rsp_xml_body, 4096, "<?xml version=\"1.0\"?>\r\n"
                                     "<Response>\r\n"
                                     "<CmdType>Catalog</CmdType>\r\n"/*命令类型*/
                                     "<SN>%s</SN>\r\n"/*命令序列号*/
                                     "<DeviceID>%s</DeviceID>\r\n"/*目标设备/区域/系统的编码*/
                                     "<SumNum>1</SumNum>\r\n"/*查询结果总数*/
                                     "<DeviceList Num=\"1\">\r\n"/*设备目录项列表*/
                                     "<Item>\r\n"
                                     "<DeviceID>%s</DeviceID>\r\n"/*目标设备/区域/系统的编码*/
                                     "<Name>%s</Name>\r\n"/*设备/区域/系统名称*/
                                     "<Manufacturer>%s</Manufacturer>\r\n"/*设备厂商*/
                                     "<Model>%s</Model>\r\n"/*设备型号*/
                                     "<Owner>Owner</Owner>\r\n"/*设备归属*/
                                     "<CivilCode>435200</CivilCode>\r\n"/*行政区域*/
                                     "<Block>435200</Block>\r\n"/*??????*/
                                     "<Address>Address</Address>\r\n"/*安装地址*/
                                     "<Parental>0</Parental>\r\n"/*是否有子设备*/
                                     "<ParentID>%s</ParentID>\r\n"
                                     "<IPAddress></IPAddress>\r\n"
                                     "<Port></Port>\r\n"
                                     "<Password></Password>\r\n"
                                     //                                     "<SafetyWay>0</SafetyWay>\r\n"/*信令安全模式/0为不采用/2为S/MIME签名方式/3为S/MIME加密签名同时采用方式/4为数字摘要方式*/
                                     //                                     "<RegisterWay>1</RegisterWay>\r\n"/*注册方式/1为符合sip3261标准的认证注册模式/2为基于口令的双向认证注册模式/3为基于数字证书的双向认证注册模式*/
                                     //                                     "<Secrecy>0</Secrecy>\r\n"
                                     "<Status>ON</Status>\r\n"
                                     "<Longitude>%f</Longitude>\r\n"
                                     "<Latitude>%f</Latitude>\r\n"
                                     "</Item>\r\n"
                                     "</DeviceList>\r\n"
                                     "</Response>\r\n",
                 xml_cmd_sn,
                 xml_device_id,
                 device_info.ipc_id,
                 device_info.device_name,
                 device_info.device_manufacturer,
                 device_info.device_model,
                 xml_device_id,
                 device_info.longitude ? *device_info.longitude : 0.0,
                 device_info.latitude ? *device_info.latitude : 0.0);
    } else if (0 == strcmp(xml_cmd_type, "DeviceInfo"))/*网络设备信息查询：设备信息查询*/
    {
        LOGD("**********DEVICE INFO START**********\r\n");
        /*设备信息查询*/
        LOGD("***********DEVICE INFO END***********\r\n");
        snprintf(rsp_xml_body, 4096, "<?xml version=\"1.0\"?>\r\n"
                                     "<Response>\r\n"
                                     "<CmdType>DeviceInfo</CmdType>\r\n"/*命令类型*/
                                     "<SN>%s</SN>\r\n"/*命令序列号*/
                                     "<DeviceID>%s</DeviceID>\r\n"/*目标设备/区域/系统的编码*/
                                     "<Result>OK</Result>\r\n"/*查询结果*/
                                     "<DeviceType>IPC</DeviceType>\r\n"
                                     "<Manufacturer>%s</Manufacturer>\r\n"/*设备生产商*/
                                     "<Model>%s</Model>\r\n"/*设备型号*/
                                     "<Firmware>%s</Firmware>\r\n"/*设备固件版本*/
                                     "<MaxCamera>1</MaxCamera>\r\n"
                                     "<MaxAlarm>1</MaxAlarm>\r\n"
                                     "</Response>\r\n",
                 xml_cmd_sn,
                 xml_device_id,
                 device_info.device_manufacturer,
                 device_info.device_model,
                 device_info.device_firmware);
    } else if (0 == strcmp(xml_cmd_type, "DeviceStatus"))/*网络设备信息查询：设备状态查询*/
    {
        LOGD("**********DEVICE STATUS START**********\r\n");
        /*设备状态查询*/
        LOGD("***********DEVICE STATUS END***********\r\n");
        char xml_status_guard[10];
        strncpy(xml_status_guard, device_status.status_guard, sizeof(xml_status_guard));/*保存当前布防状态*/
        dt_eXosip_callback.dt_eXosip_getDeviceStatus(&device_status);/*获取设备当前状态*/
        strncpy(device_status.status_guard, xml_status_guard, sizeof(xml_status_guard));/*恢复当前布防状态*/
        snprintf(rsp_xml_body, 4096, "<?xml version=\"1.0\"?>\r\n"
                                     "<Response>\r\n"
                                     "<CmdType>DeviceStatus</CmdType>\r\n"/*命令类型*/
                                     "<SN>%s</SN>\r\n"/*命令序列号*/
                                     "<DeviceID>%s</DeviceID>\r\n"/*目标设备/区域/系统的编码*/
                                     "<Result>OK</Result>\r\n"/*查询结果标志*/
                                     "<Online>%s</Online>\r\n"/*是否在线/ONLINE/OFFLINE*/
                                     "<Status>%s</Status>\r\n"/*是否正常工作*/
                                     "<Encode>%s</Encode>\r\n"/*是否编码*/
                                     "<Record>%s</Record>\r\n"/*是否录像*/
                                     "<DeviceTime>%s</DeviceTime>\r\n"/*设备时间和日期*/
                                     "<Alarmstatus Num=\"1\">\r\n"/*报警设备状态列表*/
                                     "<Item>\r\n"
                                     "<DeviceID>%s</DeviceID>\r\n"/*报警设备编码*/
                                     "<DutyStatus>%s</DutyStatus>\r\n"/*报警设备状态*/
                                     "</Item>\r\n"
                                     "</Alarmstatus>\r\n"
                                     "</Response>\r\n",
                 xml_cmd_sn,
                 xml_device_id,
                 device_status.status_online,
                 device_status.status_ok,
                 device_info.device_encode,
                 device_info.device_record,
                 device_status.status_time,
                 xml_device_id,
                 device_status.status_guard);
    } else if (0 == strcmp(xml_cmd_type, "RecordInfo"))/*设备视音频文件检索*/
    {
        /*录像文件检索*/
        char xml_file_path[30];
        char xml_start_time[30];
        char xml_end_time[30];
        char xml_recorder_id[30];
        char item_start_time[30];
        char item_end_time[30];
        char rsp_item_body[4096];
        int record_list_num = 0;
        int record_list_ret = 0;

        memset(xml_file_path, 0, 30);
        memset(xml_start_time, 0, 30);
        memset(xml_end_time, 0, 30);
        memset(xml_recorder_id, 0, 30);
        memset(item_start_time, 0, 30);
        memset(item_end_time, 0, 30);
        memset(rsp_item_body, 0, 4096);
        LOGD("**********RECORD INFO START**********\r\n");
        p_str_begin = strstr(p_xml_body, "<FilePath>");/*查找字符串"<FilePath>"*/
        p_str_end = strstr(p_xml_body, "</FilePath>");
        memcpy(xml_file_path, p_str_begin + 10,
               p_str_end - p_str_begin - 10);/*保存<FilePath>到xml_file_path*/
        LOGD("<FilePath>:%s\r\n", xml_file_path);

        p_str_begin = strstr(p_xml_body, "<StartTime>");/*查找字符串"<StartTime>"*/
        p_str_end = strstr(p_xml_body, "</StartTime>");
        memcpy(xml_start_time, p_str_begin + 11,
               p_str_end - p_str_begin - 11);/*保存<StartTime>到xml_start_time*/
        LOGD("<StartTime>:%s\r\n", xml_start_time);

        p_str_begin = strstr(p_xml_body, "<EndTime>");/*查找字符串"<EndTime>"*/
        p_str_end = strstr(p_xml_body, "</EndTime>");
        memcpy(xml_end_time, p_str_begin + 9,
               p_str_end - p_str_begin - 9);/*保存<EndTime>到xml_end_time*/
        LOGD("<EndTime>:%s\r\n", xml_end_time);

        p_str_begin = strstr(p_xml_body, "<RecorderID>");/*查找字符串"<RecorderID>"*/
        p_str_end = strstr(p_xml_body, "</RecorderID>");
        memcpy(xml_recorder_id, p_str_begin + 12,
               p_str_end - p_str_begin - 12);/*保存<RecorderID>到xml_recorder_id*/
        LOGD("<RecorderID>:%s\r\n", xml_recorder_id);
        LOGD("***********RECORD INFO END***********\r\n");
        for (;;) {
            record_list_ret = dt_eXosip_callback.dt_eXosip_getRecordTime(xml_start_time,
                                                                         xml_end_time,
                                                                         item_start_time,
                                                                         item_end_time);
            if (0 > record_list_ret) {
                break;
            } else {
                char temp_body[1024];
                memset(temp_body, 0, 1024);
                snprintf(temp_body, 1024, "<Item>\r\n"
                                          "<DeviceID>%s</DeviceID>\r\n"/*设备/区域编码*/
                                          "<Name>%s</Name>\r\n"/*设备/区域名称*/
                                          "<FilePath>%s</FilePath>\r\n"/*文件路径名*/
                                          "<Address>Address1</Address>\r\n"/*录像地址*/
                                          "<StartTime>%s</StartTime>\r\n"/*录像开始时间*/
                                          "<EndTime>%s</EndTime>\r\n"/*录像结束时间*/
                                          "<Secrecy>0</Secrecy>\r\n"/*保密属性/0为不涉密/1为涉密*/
                                          "<Type>time</Type>\r\n"/*录像产生类型*/
                                          "<RecorderID>%s</RecorderID>\r\n"/*录像触发者ID*/
                                          "</Item>\r\n",
                         xml_device_id,
                         device_info.device_name,
                         xml_file_path,
                         item_start_time,
                         item_end_time,
                         xml_recorder_id);
                strncat(rsp_item_body, temp_body, sizeof(rsp_item_body));
                record_list_num++;
                if (0 == record_list_ret) {
                    break;
                }
            }
        }
        if (0 >= record_list_num)/*未检索到任何设备视音频文件*/
        {
            return;
        }
        snprintf(rsp_xml_body, 4096, "<?xml version=\"1.0\"?>\r\n"
                                     "<Response>\r\n"
                                     "<CmdType>RecordInfo</CmdType>\r\n"/*命令类型*/
                                     "<SN>%s</SN>\r\n"/*命令序列号*/
                                     "<DeviceID>%s</DeviceID>\r\n"/*设备/区域编码*/
                                     "<Name>%s</Name>\r\n"/*设备/区域名称*/
                                     "<SumNum>%d</SumNum>\r\n"/*查询结果总数*/
                                     "<RecordList Num=\"%d\">\r\n"/*文件目录项列表*/
                                     "%s\r\n"
                                     "</RecordList>\r\n"
                                     "</Response>\r\n",
                 xml_cmd_sn,
                 xml_device_id,
                 device_info.device_name,
                 record_list_num,
                 record_list_num,
                 rsp_item_body);
    } else if (0 == strcmp(xml_cmd_type, "Broadcast")) {
        osip_message_t *answer = nullptr;
        int i = eXosip_message_build_answer(context, p_event->tid, 200, &answer);
        //int i = osip_message_init(&answer);
        if (i != 0)
            LOGE("build a answer occurred error. No sent");
        else
        {
            char *msg_str;
            /*char *call_id, *msg_str, *unique;
            char cseq[32], via[64];
            memset(cseq, 0, 32);
            //memset(via, 0, 64);

            call_id = my_call_id_new_random();
            unique = my_branch_new_unique();

            //auto now = std::chrono::system_clock::now();
            //auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
            //long long ms = millis.count();
            //LOGD("=== MS: %lld ===", ms);

            snprintf(cseq, 32, "%d MESSAGE", SN++);
            //snprintf(via, 64, "SIP/2.0/UDP %s:%s;rport;branch=%s", device_info.ipc_ip, device_info.ipc_sess_port, unique);

            // response
            osip_message_set_status_code(answer, 200);
            osip_message_set_reason_phrase(answer, osip_strdup("OK"));
            osip_message_set_max_forwards(answer, "70");

            // headers
            // From, To, Via
            osip_message_set_from(answer, from);
            osip_message_set_to(answer, to);
            //osip_message_set_via(answer, via);
            int pos = 0;
            while (!osip_list_eol(&p_event->request->vias, pos)) {
                osip_via_t *via;
                osip_via_t *via2 = nullptr;
                via = (osip_via_t *) osip_list_get(&p_event->request->vias, pos);
                osip_via_clone(via, &via2);
                osip_list_add(&answer->vias, via2, -1);
                pos++;
            }

            //osip_message_set_header(answer, "To", to);
            //osip_message_set_header(answer, "From", from);

            // set Call-ID, CSeq
            osip_message_set_call_id(answer, call_id);
            osip_message_set_cseq(answer, cseq);
            osip_message_set_content_type(answer, "application/sdp");*/

            osip_message_to_str(answer, &msg_str, &answer->message_length);
            i = osip_message_get_status_code(answer);
            LOGE("osip_message_to_str, status_code=%d; Content:\n%s", i, msg_str);

            eXosip_lock(context);
            i = eXosip_message_send_answer(context, p_event->tid, 200, answer);
            //i = eXosip_call_send_answer(context, p_event->tid, 200, answer);
            //i = eXosip_message_send_request(context, answer);
            eXosip_unlock(context);
            if (i != 0)
                LOGE("send a answer failed. i=%d", i);
            else
                LOGE("send a answer success.");

            //free(call_id);
            free(msg_str);
            //free(unique);
        }

        char xml_body[4096];
        snprintf(xml_body, 4096, R"(<?xml version="1.0"?>)"
                                 "\r\n<Response>\r\n"
                                 "<CmdType>Broadcast</CmdType>\r\n"
                                 "<SN>%d</SN>\r\n"
                                 "<DeviceID>%s</DeviceID>\r\n"
                                 "<Result>OK</Result>\r\n"
                                 "</Response>\r\n", SN++, device_info.ipc_id);
        dt_eXosip_send_message(context, xml_body);

        return;
    }
        // 	else if (0 == strcmp(xml_cmd_type, "RecordInfo"))/*设备视音频文件检索*/
        // 	{
        //
        //     }
    else/*CmdType为不支持的类型*/
    {
        //dt_eXosip_keepAlive(context);
        LOGD("**********OTHER TYPE START**********\r\n");
        LOGD("***********OTHER TYPE END***********\r\n");
        return;
    }
    osip_message_set_body(rsp_msg, rsp_xml_body, strlen(rsp_xml_body));
    osip_message_set_content_type(rsp_msg, "Application/MANSCDP+xml");
    eXosip_message_send_request(context, rsp_msg);/*回复"MESSAGE"请求*/
    LOGD("eXosip_message_send_request success!\r\n");
}


void dt_eXosip_keepAlive(eXosip_t *context) {
    char xml_body[4096];
    memset(xml_body, 0, 4096);
    snprintf(xml_body, 4096, R"(<?xml version="1.0" encoding="utf-8"?>)"
                             "<Notify>\r\n"
                             "<CmdType>Keepalive</CmdType>\r\n" // 命令类型
                             "<SN>%d</SN>\r\n"                   // 命令序列号
                             "<DeviceID>%s</DeviceID>\r\n"      // 设备编码
                             "<Status>OK</Status>\r\n"          // 是否正常工作
                             "</Notify>\r\n", SN++, device_info.ipc_id);

    dt_eXosip_send_message(context, xml_body);
}

void dt_eXosip_send_message(eXosip_t *context, char *body)
{
    osip_message_t *rqt_msg = NULL;
    char to[100];/*sip:主叫用户名@被叫IP地址*/
    char from[100];/*sip:被叫IP地址:被叫IP端口*/
    char *call_id;

    memset(to, 0, 100);
    memset(from, 0, 100);

    snprintf(to, sizeof to, "sip:%s@%s:%s", device_info.server_id, device_info.server_ip,
             device_info.server_port);
    snprintf(from, sizeof from, "sip:%s@%s:%s", device_info.ipc_id, device_info.ipc_ip,
             device_info.ipc_sess_port);

    int i = eXosip_message_build_request(context, &rqt_msg, "MESSAGE", to, from, nullptr);/*构建"MESSAGE"请求*/
    LOGE("KeepAlive build_request Success? %d", i);

    char cseq[32];
    char c_body_len[32];
    //char via[64];
    memset(cseq, 0, 32);
    //memset(via, 0, 64);
    call_id = my_call_id_new_random();
    //unique = my_branch_new_unique();
    auto now = std::chrono::system_clock::now();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    long long ms = millis.count();

    snprintf(cseq, 32, "%lld MESSAGE", ms);
    //snprintf(via, 64, "SIP/2.0/UDP %s:%s;rport;branch=%s", device_info.ipc_ip, device_info.ipc_sess_port, unique);
    //LOGD("<MSG_VIA_IS>:\n%s", via);

    osip_message_set_call_id(rqt_msg, call_id);
    osip_message_set_cseq(rqt_msg, cseq);
    //osip_message_set_via(rqt_msg, via);
    snprintf(c_body_len, 32, "%ld", strlen(body));
    LOGE("Sending... body length='%s'", c_body_len);
    osip_message_set_content_length(rqt_msg, c_body_len);

    osip_message_set_body(rqt_msg, body, strlen(body));
    osip_message_set_content_type(rqt_msg, "application/MANSCDP+xml");

    eXosip_lock(context);
    i = eXosip_message_send_request(context, rqt_msg);/*回复"MESSAGE"请求*/
    eXosip_unlock(context);
    free(call_id);
    //free(unique);
    LOGD("dt_eXosip_send_message Success? %d", i);
}

/*检测并打印事件*/
bool dt_eXosip_printEvent(eXosip_t *context, eXosip_event_t *p_event) {
    osip_message_t *clone_event = NULL;
    char *message;
    if (p_event->type == EXOSIP_CALL_RELEASED) {
        return true;
    }
    switch (p_event->type) {
        case EXOSIP_REGISTRATION_SUCCESS:
            LOGD("EXOSIP_REGISTRATION_SUCCESS");
            break;
        case EXOSIP_REGISTRATION_FAILURE:
            LOGD("EXOSIP_REGISTRATION_FAILURE");
            break;
        case EXOSIP_CALL_INVITE:
            LOGD("EXOSIP_CALL_INVITE");
            break;
        case EXOSIP_CALL_REINVITE:
            LOGD("EXOSIP_CALL_REINVITE");
            break;
        case EXOSIP_CALL_NOANSWER:
            LOGD("EXOSIP_CALL_NOANSWER");
            break;
        case EXOSIP_CALL_PROCEEDING:
            LOGD("EXOSIP_CALL_PROCEEDING");
            break;
        case EXOSIP_CALL_RINGING:
            LOGD("EXOSIP_CALL_RINGING");
            break;
        case EXOSIP_CALL_ANSWERED:
            LOGD("EXOSIP_CALL_ANSWERED");
            break;
        case EXOSIP_CALL_REDIRECTED:
            LOGD("EXOSIP_CALL_REDIRECTED");
            break;
        case EXOSIP_CALL_REQUESTFAILURE:
            LOGD("EXOSIP_CALL_REQUESTFAILURE");
            break;
        case EXOSIP_CALL_SERVERFAILURE:
            LOGD("EXOSIP_CALL_SERVERFAILURE");
            break;
        case EXOSIP_CALL_GLOBALFAILURE:
            LOGD("EXOSIP_CALL_GLOBALFAILURE");
            break;
        case EXOSIP_CALL_ACK:
            LOGD("EXOSIP_CALL_ACK");
            break;
        case EXOSIP_CALL_CANCELLED:
            LOGD("EXOSIP_CALL_CANCELLED");
            break;
        case EXOSIP_CALL_MESSAGE_NEW:
            LOGD("EXOSIP_CALL_MESSAGE_NEW");
            break;
        case EXOSIP_CALL_MESSAGE_PROCEEDING:
            LOGD("EXOSIP_CALL_MESSAGE_PROCEEDING");
            break;
        case EXOSIP_CALL_MESSAGE_ANSWERED:
            LOGD("EXOSIP_CALL_MESSAGE_ANSWERED");
            break;
        case EXOSIP_CALL_MESSAGE_REDIRECTED:
            LOGD("EXOSIP_CALL_MESSAGE_REDIRECTED");
            break;
        case EXOSIP_CALL_MESSAGE_REQUESTFAILURE:
            LOGD("EXOSIP_CALL_MESSAGE_REQUESTFAILURE");
            break;
        case EXOSIP_CALL_MESSAGE_SERVERFAILURE:
            LOGD("EXOSIP_CALL_MESSAGE_SERVERFAILURE");
            break;
        case EXOSIP_CALL_MESSAGE_GLOBALFAILURE:
            LOGD("EXOSIP_CALL_MESSAGE_GLOBALFAILURE");
            break;
        case EXOSIP_CALL_CLOSED:
            LOGD("EXOSIP_CALL_CLOSED");
            break;
        case EXOSIP_CALL_RELEASED:
            LOGD("EXOSIP_CALL_RELEASED");
            break;
        case EXOSIP_MESSAGE_NEW:
            LOGD("EXOSIP_MESSAGE_NEW");
            break;
        case EXOSIP_MESSAGE_PROCEEDING:
            LOGD("EXOSIP_MESSAGE_PROCEEDING");
            break;
        case EXOSIP_MESSAGE_ANSWERED:
            LOGD("EXOSIP_MESSAGE_ANSWERED");
            if (p_event->response)
            {
                LOGE("status_code: %d", p_event->response->status_code);
            }
            else
            {
                LOGE("Response is NULL.");
            }
            break;
        case EXOSIP_MESSAGE_REDIRECTED:
            LOGD("EXOSIP_MESSAGE_REDIRECTED");
            break;
        case EXOSIP_MESSAGE_REQUESTFAILURE:
            LOGE("EXOSIP_MESSAGE_REQUESTFAILURE");
            if (p_event->response)
            {
                LOGE("Error: %d", p_event->response->status_code);
            }
            else
            {
                LOGE("Response is NULL.");
            }
            break;
        case EXOSIP_MESSAGE_SERVERFAILURE:
            LOGD("EXOSIP_MESSAGE_SERVERFAILURE");
            break;
        case EXOSIP_MESSAGE_GLOBALFAILURE:
            LOGD("EXOSIP_MESSAGE_GLOBALFAILURE");
            break;
        case EXOSIP_SUBSCRIPTION_NOANSWER:
            LOGD("EXOSIP_SUBSCRIPTION_NOANSWER");
            break;
        case EXOSIP_SUBSCRIPTION_PROCEEDING:
            LOGD("EXOSIP_SUBSCRIPTION_PROCEEDING");
            break;
        case EXOSIP_SUBSCRIPTION_ANSWERED:
            LOGD("EXOSIP_SUBSCRIPTION_ANSWERED");
            break;
        case EXOSIP_SUBSCRIPTION_REDIRECTED:
            LOGD("EXOSIP_SUBSCRIPTION_REDIRECTED");
            break;
        case EXOSIP_SUBSCRIPTION_REQUESTFAILURE:
            LOGD("EXOSIP_SUBSCRIPTION_REQUESTFAILURE");
            break;
        case EXOSIP_SUBSCRIPTION_SERVERFAILURE:
            LOGD("EXOSIP_SUBSCRIPTION_SERVERFAILURE");
            break;
        case EXOSIP_SUBSCRIPTION_GLOBALFAILURE:
            LOGD("EXOSIP_SUBSCRIPTION_GLOBALFAILURE");
            break;
        case EXOSIP_SUBSCRIPTION_NOTIFY:
            LOGD("EXOSIP_SUBSCRIPTION_NOTIFY");
            break;
        case EXOSIP_IN_SUBSCRIPTION_NEW:
            LOGD("EXOSIP_IN_SUBSCRIPTION_NEW");
            break;
        case EXOSIP_NOTIFICATION_NOANSWER:
            LOGD("EXOSIP_NOTIFICATION_NOANSWER");
            break;
        case EXOSIP_NOTIFICATION_PROCEEDING:
            LOGD("EXOSIP_NOTIFICATION_PROCEEDING");
            break;
        case EXOSIP_NOTIFICATION_ANSWERED:
            LOGD("EXOSIP_NOTIFICATION_ANSWERED");
            break;
        case EXOSIP_NOTIFICATION_REDIRECTED:
            LOGD("EXOSIP_NOTIFICATION_REDIRECTED");
            break;
        case EXOSIP_NOTIFICATION_REQUESTFAILURE:
            LOGD("EXOSIP_NOTIFICATION_REQUESTFAILURE");
            break;
        case EXOSIP_NOTIFICATION_SERVERFAILURE:
            LOGD("EXOSIP_NOTIFICATION_SERVERFAILURE");
            break;
        case EXOSIP_NOTIFICATION_GLOBALFAILURE:
            LOGD("EXOSIP_NOTIFICATION_GLOBALFAILURE");
            break;
        case EXOSIP_EVENT_COUNT:
            LOGD("EXOSIP_EVENT_COUNT");
            break;
        default:
            LOGD("..................");
            break;
    }

    char *start;
    char *end;
    char *p_find_broadcast = nullptr;
    size_t len = 0;
    if (p_event->request)
    {
        osip_message_clone(p_event->request, &clone_event);
        osip_message_to_str(clone_event, &message, &len);
        p_find_broadcast = strstr(message, "Broadcast");

        start = strstr(message, "<TargetID>");
        end = strstr(message, "</TargetID>");
        if (p_find_broadcast && start && end)
        {
            memset(target_id, 0, 32);
            strncpy(target_id, start + 10, end - start - 10);
        }
        //LOGD("收到消息,长度：%lu：\n%s", len, message);

        __android_log_print(ANDROID_LOG_ERROR, "JNI", "====== Log request messages ======");
        __android_log_print(ANDROID_LOG_ERROR, "JNI", "Received message, length=%zu", len);
        if (len > 500) {
            int p = 0;
            size_t arr_len = sizeof(char) * 501;
            char temp[arr_len];
            __android_log_print(ANDROID_LOG_ERROR, "JNI", "message content:");
            do {
                size_t diff = len - p;
                memset(temp, 0, arr_len);
                strncpy(temp, &message[p], diff > 500 ? 500 : diff);
                temp[500] = '\0';
                __android_log_print(ANDROID_LOG_DEBUG, "JNI", "%s", temp);
                p += 500;
            } while (p < len);
        } else {
            __android_log_print(ANDROID_LOG_DEBUG, "JNI", "message content:\n%s", message);
        }
        free(message);
        //osip_message_free(clone_event);
        __android_log_print(ANDROID_LOG_ERROR, "JNI", "------ Request messages ------");
    }

    if (p_event->response)
    {
        int status_code = osip_message_get_status_code(p_event->response);
        osip_message_clone(p_event->response, &clone_event);
        osip_message_to_str(clone_event, &message, &len);
        char *is_ok = strstr(message, "SIP/2.0 200 OK");
        char *is_self = strstr(message, UA_STRING);
        osip_content_length_t *osip_content_length = osip_message_get_content_length(clone_event);
        //LOGE("===> osip_message_get_status_code=%d,  <===")
        //LOGD("收到消息,长度：%lu：\n%s", len, message);

        __android_log_print(ANDROID_LOG_ERROR, "JNI", "=-=-=-= Log response messages =-=-=-=");
        if (len > 500) {
            int p = 0;
            size_t arr_len = sizeof(char) * 501;
            char temp[arr_len];
            __android_log_print(ANDROID_LOG_ERROR, "JNI", "message content:");
            do {
                size_t diff = len - p;
                memset(temp, 0, arr_len);
                strncpy(temp, &message[p], diff > 500 ? 500 : diff);
                temp[500] = '\0';
                __android_log_print(ANDROID_LOG_WARN, "JNI", "%s", temp);
                p += 500;
            } while (p < len);
        } else {
            __android_log_print(ANDROID_LOG_WARN, "JNI", "message content:\n%s", message);
        }

        if (osip_content_length)
            LOGD("osip_content_length->value=%s, ", osip_content_length->value);
        else
            LOGD("osip_content_length is NULL");
        if (p_find_broadcast != nullptr && status_code == 200)
        {
            // 发送INVITE
            send_broadcast_invite_to_server(context);
        }
        else if (osip_content_length && is_ok && is_self == nullptr && strcmp(osip_content_length->value, "0") != 0)
        {
            // 发送ACK
            char ip[16];
            char rtmp_url[128];
            start = strstr(message, "c=IN IP4 ");
            end = strstr(start, "\n");
            if (start != nullptr && end != nullptr)
            {
                memset(ip, 0, 16);
                memset(rtmp_url, 0, 128);
                strncpy(ip, start + 9, end - start - 9);
                snprintf(rtmp_url, 128, "rtmp://%s:1935/broadcast/%s_%s",
                         ip, target_id, device_info.ipc_id);
                LOGD("---\nip = %s;\tTargetID=%s;\trtmp_url=%s\n---",
                     ip, target_id, rtmp_url);
                // TODO callback.
                // rtmp://139.224.133.209:1935/broadcast/deviceid_channelid
                // rtmp://ip:1935/broadcast/ipc_id+TargetID
                sipHolder->mSipCallback->audioRtmpUrl(rtmp_url);
            }
            send_broadcast_ack_to_server(context, p_event);
        }

        free(message);
        if (osip_content_length)
            osip_content_length_free(osip_content_length);
        //osip_message_free(clone_event);
        __android_log_print(ANDROID_LOG_ERROR, "JNI", "======= Response messages =======");
    }
    else
    {
        __android_log_print(ANDROID_LOG_ERROR, "JNI", "=-=-=-= Response is NULL. =-=-=-=");
    }

    /*LOGD("收到消息,长度：%lu：\n%s", len, message);
    if (len > 500) {
        LOGD("收到消息：\n%s", &message[500]);
    }*/
    return false;
}
/*解析INVITE的SDP消息体，同时保存全局INVITE连接ID和全局会话ID*/
void dt_eXosip_paraseInviteBody(eXosip_t *context, eXosip_event_t *p_event) {
    sdp_message_t *sdp_msg = NULL;
    char *media_sever_ip = NULL;
    char *media_sever_port = NULL;

    sdp_msg = eXosip_get_remote_sdp(context, p_event->did);
    if (sdp_msg == NULL) {
        LOGD("eXosip_get_remote_sdp NULL!");
        return;
    }
    LOGD("eXosip_get_remote_sdp success!");

    g_call_id = p_event->cid;/*保存全局INVITE连接ID*/
    /*实时点播*/
    if (0 == strcmp(sdp_msg->s_name, "Play")) {
        g_did_realPlay = p_event->did;/*保存全局会话ID：实时视音频点播*/
    }
        /*回放*/
    else if (0 == strcmp(sdp_msg->s_name, "Playback")) {
        g_did_backPlay = p_event->did;/*保存全局会话ID：历史视音频回放*/
    }
        /*下载*/
    else if (0 == strcmp(sdp_msg->s_name, "Download")) {
        g_did_fileDown = p_event->did;/*保存全局会话ID：视音频文件下载*/
    }
    /*从SIP服务器发过来的INVITE请求的o字段或c字段中获取媒体服务器的IP地址与端口*/
    media_sever_ip = sdp_message_o_addr_get(sdp_msg);/*媒体服务器IP地址*/
    media_sever_port = sdp_message_m_port_get(sdp_msg, 0);/*媒体服务器IP端口*/
    LOGD("dt_eXosip_paraseInviteBody：%s->%s:%s", sdp_msg->s_name, media_sever_ip, media_sever_port);
    dt_eXosip_callback.dt_eXosip_mediaControl(sdp_msg->s_name, media_sever_ip, media_sever_port);

    if (sipHolder->mSipCallback != NULL) {
        sipHolder->mSipCallback->mediaControl(sdp_msg->s_name, media_sever_ip, media_sever_port);
    }
}

char *my_call_id_new_random()
{
    // allocate memory for the Call-ID value
    char *call_id = (char *)malloc(33);

    // check if memory allocation succeeded
    if (call_id == NULL)
    {
        return NULL;
    }

    // initialize the random number generator
    srand(time(NULL));

    // generate a random hexadecimal string of length 32
    for (int i = 0; i < 32; i++)
    {
        int r = rand() % 16;
        if (r < 10)
        {
            call_id[i] = '0' + r;
        }
        else
        {
            call_id[i] = 'a' + r - 10;
        }
    }

    // add a null terminator at the end
    call_id[32] = '\0';

    // return the Call-ID value
    return call_id;
}

char *my_branch_new_unique()
{
  // allocate memory for the branch value
  char *branch = (char *)malloc(33);

  // check if memory allocation succeeded
  if (branch == NULL)
  {
    return NULL;
  }

  // initialize the random number generator
  srand(time(NULL));

  // generate a random hexadecimal string of length 32
  for (int i = 0; i < 32; i++)
  {
    int r = rand() % 16;
    if (r < 10)
    {
      branch[i] = '0' + r;
    }
    else
    {
      branch[i] = 'a' + r - 10;
    }
  }

  // add a null terminator at the end
  branch[32] = '\0';

  // return the branch value
  return branch;
}

void send_broadcast_invite_to_server(eXosip_t *context)
{
    osip_message_t *invite;
    char to[64], from[64], subject[64], body[512], len[10];
    char *msg_str;

    memset(to, 0, 64);
    memset(from, 0, 64);
    memset(subject, 0, 64);
    memset(body, 0, 512);
    memset(len, 0, 10);

    snprintf(to, sizeof(to), "sip:%s@%s:%s", device_info.server_id, device_info.server_ip,
             device_info.server_port);
    snprintf(from, sizeof(from), "sip:%s@%s:%s", device_info.ipc_id, device_info.ipc_ip,
             device_info.ipc_sess_port);
    //snprintf(subject, sizeof subject, "Subject: %s:1, %s:2", device_info.ipc_id, source_id);
    /*
     *
v=0
o=34020000001110000001 2418 2418 IN IP4 192.168.1.64
s=Play
c=IN IP4 192.168.1.64
t=0 0
<!-- 音频 端口  RTP-over-UDP 负载类型( 8-PCMA, 96-PS) -->
m=audio 15062 RTP/AVP 8 96
a=recvonly
<!-- RTP + 音频流: 负载类型 <encodingname>/<clockrate> -->
a=rtpmap:8 PCMA/8000
a=rtpmap:96 PS/90000
<!-- SSRC(同步信源标识符): SSRC值由媒体流发送设备所在的SIP监控域产生,作为媒体流的标识使用 -->
y=0200000017
<!--// v/编码格式/分辨率/帧率/码率类型/码率大小  a/编码格式/码率大小/采样率
    //										  G.711 / 64kbps / 8kHz -->
f=v/a/1/8/1
     */
    /*snprintf(body, 512, "v=0\r\n"/*协议版本*/
                             //"o=%s 0 0 IN IP4 %s\r\n"/*会话源*//*用户名/会话ID/版本/网络类型/地址类型/地址*/
                             //"s=Play\r\n"/*会话名*/
                             //"c=IN IP4 %s\r\n"/*连接信息*//*网络类型/地址信息/多点会议的地址*/
                             //"t=0 0\r\n"/*时间*//*开始时间/结束时间*/
                             //"m=audio %s RTP/AVP 96\r\n"/*媒体/端口/传送层协议/格式列表*/
                             //"a=recvonly\r\n"/*收发模式*/
                             //"a=rtpmap:96 PS/90000\r\n"/*净荷类型/编码名/时钟速率*/
                             /*"a=username:%s\r\n"
                             "a=password:%s\r\n"
                             "y=0999999999\r\n"
                             "f=v/////a/1/8/1\r\n",*/
             /*device_info.ipc_id,
             device_info.ipc_ip,
             device_info.ipc_ip,
             device_info.ipc_media_port,
             device_info.ipc_id,
             device_info.ipc_pwd);*/
    snprintf(body, 256, "v=0\n"
                        "o=%s 0 0 IN IP4 %s\n"//用户ID, ip
                        "s=Play\n"
                        "c=IN IP4 %s\n"// ip
                        "t=0 0\n"
                        //"m=audio 15065 RTP/AVP 8 96\n"
                        "m=audio 8000 RTP/AVP 8\n"
                        "a=recvonly\n"
                        "a=rtpmap:8 PCMA/8000\n"
                        //"a=rtpmap:96 PS/90000\n"
                        "y=0200000017\n"
                        "f=v/////a/1/8/1",
                        //"f=v/a/1/8/1",
                        device_info.ipc_id, device_info.ipc_ip, device_info.ipc_ip
    );
    size_t body_len = strlen(body);
    snprintf(len, 10, "%ld", body_len);

    int i = eXosip_call_build_initial_invite(context, &invite, to, from, nullptr, subject);
    osip_message_set_content_length(invite, len);
    osip_message_set_body(invite, body, body_len);
    osip_message_set_content_type(invite, "application/sdp");

    osip_message_to_str(invite, &msg_str, &body_len);
    LOGW("eXosip_call_build_initial_invite: i = %d, body:\n%s", i, msg_str);
    eXosip_lock(context);
    i = eXosip_call_send_initial_invite(context, invite);
    eXosip_unlock(context);
    LOGD("Sent a invite: i = %d", i);
    free(msg_str);
    //osip_message_free(invite);
}

void send_broadcast_ack_to_server(eXosip_t *context, eXosip_event_t *p_event)
{
    osip_message_t *ack;
    char via[64], *unique;
    unique = my_branch_new_unique();

    int i = eXosip_call_build_ack(context, p_event->did, &ack);
    if (i)
    {
        LOGE("build ack failed. i = %d", i);
        return;
    }
    LOGD("Build ack success.");
    snprintf(via, 64, "SIP/2.0/UDP %s:%s;rport;branch=%s", device_info.ipc_ip, device_info.ipc_sess_port, unique);

    eXosip_lock(context);
    i = eXosip_call_send_ack(context, p_event->did, ack);
    eXosip_unlock(context);
    LOGD("Send ack: i = %d", i);

    free(unique);
}

}
