#include <android/log.h>
#include <jni.h>
#include <string>
#include "log.h"
#include "SipClientListener.h"
#include "include/eXosip2/eXosip.h"
#include "include/common.h"
#include <cstdlib>
#include <pthread.h>
#include <unistd.h>
#include <iostream>
#include  <sys/socket.h>
#include "include/ctrl.h"
#include "include/refresh.h"
#include "include/gb_lib.h"
#include "include/common.h"
#include "dev_util/JniHelper.h"
#include <jni.h>

extern "C" {



DEVICE_INFO device_info;
DEVICE_STATUS device_status;
DEVICE_CONTROL device_control;
DT_EXOSIP_CALLBACK dt_eXosip_callback;
BOOL SenAliveFlag = FALSE;
int g_call_id = 0;/*INVITE连接ID/用来分辨不同的INVITE连接，每个时刻只允许有一个INVITE连接*/
int g_register_id = 0;/*注册ID/用来更新注册或取消注册*/
int g_did_realPlay = 0;/*会话ID/用来分辨不同的会话：实时视音频点播*/
int g_did_backPlay = 0;/*会话ID/用来分辨不同的会话：历史视音频回放*/
int g_did_fileDown = 0;/*会话ID/用来分辨不同的会话：视音频文件下载*/
int g_reister_listning = 1;/*会话ID/用来分辨不同的会话：实时视音频点播*/
pthread_t pthread;

JavaVM *jvm;
SipClientListener *sipClientListener;


char eXosip_server_id[30] = "34020000002000000001";
char eXosip_server_ip[20] = "10.39.62.3";
char eXosip_server_port[10] = "5060";
char eXosip_ipc_id[30] = "34020000001310000010";
char eXosip_ipc_pwd[20] = "123456";
char eXosip_ipc_ip[20];
char eXosip_ipc_media_port[10] = "20000";
char eXosip_ipc_sess_port[10] = "5080";
char eXosip_alarm_id[30] = "34020000001340000010";

char eXosip_media_ip[30];
char eXosip_media_port[10] = "6000";

char eXosip_device_name[30] = "DJIClient";
char eXosip_device_manufacturer[30] = "datang";
char eXosip_device_model[30] = "amodel";
char eXosip_device_firmware[30] = "V1.0";
char eXosip_device_encode[10] = "ON";
char eXosip_device_record[10] = "OFF";

char eXosip_status_on[10] = "ON";
char eXosip_status_ok[10] = "OK";
char eXosip_status_online[10] = "ONLINE";
char eXosip_status_guard[10] = "OFFDUTY";
char eXosip_status_time[30] = "2014-01-17T16:30:20";



JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *javaVm, void *pVoid) {
    // 获取 javaVM
    jvm = javaVm;
    AppCoder::JniHelper::setJavaVM(jvm);
    return JNI_VERSION_1_4;
}


void *normalCallBack(void *data) {
    LOGD("normalCallBack：device_info.server_id=%s", device_info.server_id);
    dt_eXosip_callback.dt_eXosip_deviceControl = dt_eXosip_deviceControl;
    dt_eXosip_callback.dt_eXosip_getDeviceInfo = dt_eXosip_getDeviceInfo;
    dt_eXosip_callback.dt_eXosip_getDeviceStatus = dt_eXosip_getDeviceStatus;
    dt_eXosip_callback.dt_eXosip_getRecordTime = dt_eXosip_getRecordTime;
    dt_eXosip_callback.dt_eXosip_mediaControl = dt_eXosip_mediaControl;
    dt_eXosip_callback.dt_eXosip_playControl = dt_eXosip_playControl;
    dt_eXosip_rigister(sipClientListener);
    pthread_exit(NULL);
}
}




extern "C"
JNIEXPORT void JNICALL
Java_io_anyrtc_sipClient_AnySipClient_unRegister(JNIEnv *env, jobject thiz) {
    g_reister_listning = 0;
    if (pthread != NULL) {
        dt_eXosip_unregister();
        pthread_kill(pthread, 0);
        pthread = NULL;
    }
}
extern "C"
JNIEXPORT void JNICALL
Java_io_anyrtc_sipClient_AnySipClient_registerSip(JNIEnv *env, jobject thiz, jstring server_ip,
                                                  jstring server_port, jstring server_id,
                                                  jstring user_id, jstring user_password,
                                                  jstring ipc_ip, jobject listener) {
    //设置监听flag
    g_reister_listning = 1;

    const char *ipc_ip_char = env->GetStringUTFChars(ipc_ip, 0);
    strcpy(eXosip_ipc_ip, ipc_ip_char);
    strcpy(eXosip_media_ip, ipc_ip_char);

    const char *server_ip_char = env->GetStringUTFChars(server_ip, 0);
    strcpy(eXosip_server_ip, server_ip_char);

    const char *server_port_char = env->GetStringUTFChars(server_port, 0);
    strcpy(eXosip_server_port, server_port_char);

    const char *server_id_char = env->GetStringUTFChars(server_id, 0);
    strcpy(eXosip_server_id, server_id_char);

    const char *user_id_char = env->GetStringUTFChars(user_id, 0);
    strcpy(eXosip_ipc_id, user_id_char);

    const char *user_password_char = env->GetStringUTFChars(user_password, 0);
    strcpy(eXosip_ipc_pwd, user_password_char);


    device_info.server_id = eXosip_server_id;
    device_info.server_ip = eXosip_server_ip;
    device_info.server_port = eXosip_server_port;
    device_info.ipc_id = eXosip_ipc_id;
    device_info.ipc_pwd = eXosip_ipc_pwd;
    device_info.ipc_ip = eXosip_ipc_ip;
    device_info.ipc_media_port = eXosip_ipc_media_port;
    device_info.ipc_sess_port = eXosip_ipc_sess_port;

    device_info.alarm_id = eXosip_alarm_id;

    device_info.media_ip = eXosip_media_ip;
    device_info.media_port = eXosip_media_port;

    device_info.device_name = eXosip_device_name;
    device_info.device_manufacturer = eXosip_device_manufacturer;
    device_info.device_model = eXosip_device_model;
    device_info.device_firmware = eXosip_device_firmware;
    device_info.device_encode = eXosip_device_encode;
    device_info.device_record = eXosip_device_record;

    device_status.status_on = eXosip_status_on;
    device_status.status_ok = eXosip_status_ok;
    device_status.status_online = eXosip_status_online;
    device_status.status_guard = eXosip_status_guard;
    device_status.status_time = eXosip_status_time;


    sipClientListener = new SipClientListener(listener);

    if (pthread) {
        pthread_kill(pthread, 0);
        pthread = NULL;
    }
    pthread_create(&pthread, nullptr, normalCallBack, (void *) nullptr);

}
extern "C"
JNIEXPORT void JNICALL
Java_io_anyrtc_sipClient_AnySipClient_registerSipWithLocation(
        JNIEnv *env, jobject thiz,
        jstring server_ip, jstring server_port, jstring server_id,
        jstring user_id, jstring user_password,
        jstring ipc_ip, jobject listener,
        jdouble latitude, jdouble longitude
)
{
    LOGD("=== set location info. ===\nlongitude=%f, latitude=%f", longitude, latitude);
    if (!device_info.longitude)
        device_info.longitude = (double *) malloc(sizeof(double));
    if (!device_info.latitude)
        device_info.latitude = (double *) malloc(sizeof(double));

    *device_info.longitude = longitude;
    *device_info.latitude = latitude;

    Java_io_anyrtc_sipClient_AnySipClient_registerSip(env, thiz, server_ip,
                                                      server_port, server_id,
                                                      user_id, user_password,
                                                      ipc_ip, listener);
}