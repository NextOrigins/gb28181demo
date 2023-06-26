//
// Created by ljb on 7/6/20.
//
#ifndef LOG_TAG
#define LOG_TAG "HELLO_JNI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG ,__VA_ARGS__) // 定义LOGD类型
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG ,__VA_ARGS__) // 定义LOGI类型
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,LOG_TAG ,__VA_ARGS__) // 定义LOGW类型
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG ,__VA_ARGS__) // 定义LOGE类型
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL,LOG_TAG ,__VA_ARGS__) // 定义LOGF类型
#endif


#ifndef NDK_2_3_2_COMMON_H
#define NDK_2_3_2_COMMON_H

typedef enum _device_control/*设备控制动作*/
{
    EXOSIP_CTRL_RMT_LEFT = 1,/*向左*/
    EXOSIP_CTRL_RMT_RIGHT,   /*向右*/
    EXOSIP_CTRL_RMT_UP,      /*向上*/
    EXOSIP_CTRL_RMT_DOWN,    /*向下*/
    EXOSIP_CTRL_RMT_LARGE,   /*放大*/
    EXOSIP_CTRL_RMT_SMALL,   /*缩小*/
    EXOSIP_CTRL_RMT_STOP,    /*停止遥控*/
    EXOSIP_CTRL_REC_START,   /*开始手动录像*/
    EXOSIP_CTRL_REC_STOP,    /*停止手动录像*/
    EXOSIP_CTRL_GUD_START,   /*布防*/
    EXOSIP_CTRL_GUD_STOP,    /*撤防*/
    EXOSIP_CTRL_ALM_RESET,   /*报警复位*/
    EXOSIP_CTRL_TEL_BOOT,    /*设备远程启动*/
} DEVICE_CONTROL;

typedef struct _device_info/*设备信息结构体*/
{
    char *server_id;/*SIP服务器ID*//*默认值：34020000002000000001*/
    char *server_ip;/*SIP服务器IP地址*//*默认值：192.168.1.178*/
    char *server_port;/*SIP服务器IP端口*//*默认值：5060*/

    char *ipc_id;/*媒体流发送者ID*//*默认值：34020000001180000002*/
    char *ipc_pwd;/*媒体流发送者密码*//*默认值：12345678*/
    char *ipc_ip;/*媒体流发送者IP地址*//*默认值：192.168.1.144*/
    char *ipc_media_port;/*媒体流发送者IP端口*//*默认值：20000*/
    char *ipc_sess_port; /*会话端口，即SIP端口*/

    char *alarm_id; /*报警器ID*/

    char *media_ip;
    char *media_port;

    char *device_name;/*设备/区域/系统名称*//*默认值：IPC*/
    char *device_manufacturer;/*设备厂商*//*默认值：CSENN*/
    char *device_model;/*设备型号*//*默认值：GB28181*/
    char *device_firmware;/*设备固件版本*//*默认值：V1.0*/
    char *device_encode;/*是否编码*//*取值范围：ON/OFF*//*默认值：ON*/
    char *device_record;/*是否录像*//*取值范围：ON/OFF*//*默认值：OFF*/

    double *latitude;
    double *longitude;
} DEVICE_INFO;
typedef struct _device_status/*设备状态结构体*/
{
    char *status_on;/*设备打开状态*//*取值范围：ON/OFF*//*默认值：ON*/
    char *status_ok;/*是否正常工作*//*取值范围：OK/ERROR*//*默认值：OFF*/
    char *status_online;/*是否在线*//*取值范围：ONLINE/OFFLINE*//*默认值：ONLINE*/
    char *status_guard;/*布防状态*//*取值范围：ONDUTY/OFFDUTY/ALARM*//*默认值：OFFDUTY*/
    char *status_time;/*设备日期和时间*//*格式：xxxx-xx-xxTxx:xx:xx*//*默认值：2012-12-20T12:12:20*/
} DEVICE_STATUS;
/*回调函数*/
typedef struct _dt_eXosip_callback {
    /*获取设备信息*/
    /*device_info：设备信息结构体指针*/
    /*返回值：成功时返回0，失败时返回负值*/
    int (*dt_eXosip_getDeviceInfo)(DEVICE_INFO *device_info);

    /*获取设备状态*/
    /*device_info：设备状态结构体指针*/
    /*返回值：成功时返回0，失败时返回负值*/
    int (*dt_eXosip_getDeviceStatus)(DEVICE_STATUS *device_status);

    /*获取录像文件的起始时间与结束时间*/
    /*时间格式：xxxx-xx-xxTxx:xx:xx*/
    /*period_start：录像时间段起始值*/
    /*period_end：录像时间段结束值*/
    /*start_time：当前返回录像文件的起始时间*/
    /*end_time：当前返回录像文件的结束时间*/
    /*返回值：成功时返回符合时间段条件的剩余录像文件数量，失败时返回负值*/
    int (*dt_eXosip_getRecordTime)(char *period_start, char *period_end, char *start_time,
                                   char *end_time);

    /*设备控制：向左、向右、向上、向下、放大、缩小、停止遥控/开始手动录像、停止手动录像/布防、撤防/报警复位/设备远程启动*/
    /*ctrl_cmd：设备控制命令，_device_control类型的枚举变量*/
    /*返回值：成功时返回0，失败时返回负值*/
    int (*dt_eXosip_deviceControl)(enum _device_control ctrl_cmd);

    /*媒体控制：实时点播/回放/下载*/
    /*control_type：媒体控制类型，实时点播/Play，回放/Playback，下载/Download*/
    /*media_ip：媒体服务器IP地址*/
    /*media_port：媒体服务器IP端口*/
    /*返回值：成功时返回0，失败时返回负值*/
    int (*dt_eXosip_mediaControl)(char *control_type, char *media_ip, char *media_port);

    /*播放控制：播放/快放/慢放/暂停*/
    /*control_type：播放控制，播放/快放/慢放/PLAY，暂停/PAUSE*/
    /*play_speed：播放速度，1为播放，大于1为快放，小于1为慢放*/
    /*pause_time：暂停时间，单位为秒*/
    /*range_start：播放范围的起始值*/
    /*range_end：播放范围的结束值*/
    /*返回值：成功时返回0，失败时返回负值*/
    int (*dt_eXosip_playControl)(char *control_type, char *play_speed, char *pause_time,
                                 char *range_start, char *range_end);
} DT_EXOSIP_CALLBACK;


#endif //NDK_2_3_2_COMMON_H
