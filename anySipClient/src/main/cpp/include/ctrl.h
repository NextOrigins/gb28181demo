#ifndef NDK_2_3_2_CTRL_H
#define NDK_2_3_2_CTRL_H
extern "C" {
typedef struct _media_para {
    char control_type[20];
    char media_ip[30];
    char media_port[10];
} MEDIA_PARA;

/*设备控制：向左、向右、向上、向下、放大、缩小、停止遥控/开始手动录像、停止手动录像/布防、撤防/报警复位/设备远程启动*/
/*ctrl_cmd：设备控制命令，_device_control类型的枚举变量*/
/*返回值：成功时返回0，失败时返回负值*/
extern int dt_eXosip_deviceControl(enum _device_control ctrl_cmd);


/*媒体控制：实时点播/回放/下载*/
/*control_type：媒体控制类型，实时点播/Play，回放/Playback，下载/Download*/
/*media_ip：媒体服务器IP地址*/
/*media_port：媒体服务器IP端口*/
/*返回值：成功时返回0，失败时返回负值*/
extern int dt_eXosip_mediaControl(char *control_type, char *media_ip, char *media_port);


/*播放控制：播放/快放/慢放/暂停*/
/*control_type：播放控制，播放/快放/慢放/PLAY，暂停/PAUSE*/
/*play_speed：播放速度，1为播放，大于1为快放，小于1为慢放*/
/*pause_time：暂停时间，单位为秒*/
/*range_start：播放范围的起始值*/
/*range_end：播放范围的结束值*/
/*返回值：成功时返回0，失败时返回负值*/
extern int
dt_eXosip_playControl(char *control_type, char *play_speed, char *pause_time, char *range_start,
                      char *range_end);
}


#endif //NDK_2_3_2_CTRL_H
