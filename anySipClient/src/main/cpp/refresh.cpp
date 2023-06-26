# include "include/eXosip2/eXosip.h"
#include "include/common.h"
#include "include/refresh.h"
extern "C" {
/*获取设备信息*/
/*device_info：设备信息结构体指针*/
/*返回值：成功时返回0，失败时返回负值*/
int dt_eXosip_getDeviceInfo(DEVICE_INFO *device_info) {
    return 0;
}

/*获取设备状态*/
/*device_info：设备状态结构体指针*/
/*返回值：成功时返回0，失败时返回负值*/
int dt_eXosip_getDeviceStatus(DEVICE_STATUS *device_status) {
    return 0;
}

/*获取录像文件的起始时间与结束时间*/
/*时间格式：xxxx-xx-xxTxx:xx:xx*/
/*period_start：录像时间段起始值*/
/*period_end：录像时间段结束值*/
/*start_time：当前返回录像文件的起始时间*/
/*end_time：当前返回录像文件的结束时间*/
/*返回值：成功时返回符合时间段条件的剩余录像文件数量，失败时返回负值*/
int
dt_eXosip_getRecordTime(char *period_start, char *period_end, char *start_time, char *end_time) {
    return 0;
}
}
