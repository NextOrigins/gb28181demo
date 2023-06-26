//
// Created by ljb on 7/5/20.
//
#ifndef NDK_2_3_2_GB_LIB_H
#define NDK_2_3_2_GB_LIB_H
extern "C" {
#define FALSE 0
#define TRUE 1
#define IPPROTO_UDP 17
#define AF_INET 2
#define UA_STRING "DJIAgent"
#include "../ISipEventHandler.h"
typedef int BOOL;

inline void LOGD_LONG(const char* tag, const char *msg);
int dt_eXosip_rigister(ISipCallBack *sipCallBack);
/*初始化eXosip*/
int dt_eXosip_init(eXosip_t *context, DEVICE_INFO info);
void dt_eXosip_keepAlive(eXosip_t *context);
/*检测并打印事件*/
bool dt_eXosip_printEvent(eXosip_t *context, eXosip_event_t *p_event);
void dt_eXosip_paraseMsgBody(eXosip_t *context, eXosip_event_t *p_event);/*解析MESSAGE的XML消息体*/
void dt_eXosip_paraseInviteBody(eXosip_t *context,eXosip_event_t *p_event);/*解析INVITE的SDP消息体，同时保存全局INVITE连接ID和全局会话ID*/
int dt_eXosip_register(eXosip_t *context, int expires,DEVICE_INFO device_info);/*注册eXosip，手动处理服务器返回的401状态*/
int dt_eXosip_unregister();/*注销eXosip*/
void dt_eXosip_sendEventAlarm(eXosip_t *context, char *alarm_time);/*报警事件通知和分发：发送报警通知*/
void dt_eXosip_sendFileEnd(eXosip_t *context);/*历史视音频回放：发送文件结束*/
void dt_eXosip_paraseInfoBody(eXosip_t *context, osip_body_t *p_msg_body);/*解析INFO的RTSP消息体*/
void dt_eXosip_processEvent(eXosip_t *context, DEVICE_INFO device_info);/*消息循环处理*/
/* 发送位置 */
//void eXosip_sendLocation(double latitude, double longitude, int altitude, double direction);

}

#endif //NDK_2_3_2_GB_LIB_H
