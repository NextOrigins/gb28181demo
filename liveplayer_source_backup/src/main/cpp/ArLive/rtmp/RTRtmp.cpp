#include "RTRtmp.h"
#include <map>
#include "RtmpIO.h"
#include "../../webrtc/rtc_base/checks.h"
#include "../../webrtc/rtc_base/time_utils.h"

typedef std::map<void*, int>MapRTRtmp;
static rtc::RecursiveCriticalSection gCsRtmp;
static MapRTRtmp	gMapRtmp;

void rtmp_discovery_tc_url(std::string tcUrl, std::string& schema, std::string& host, std::string& app, std::string& port, std::string& stream)
{
	size_t pos = std::string::npos;
	std::string url = tcUrl;

	if ((pos = url.find("://")) != std::string::npos) {
		schema = url.substr(0, pos);
		url = url.substr(schema.length() + 3);
	}

	if ((pos = url.find("/")) != std::string::npos) {
		host = url.substr(0, pos);
		url = url.substr(host.length() + 1);
	}

	port = "1935";
	if ((pos = host.find(":")) != std::string::npos) {
		port = host.substr(pos + 1);
		host = host.substr(0, pos);
	}

	if ((pos = url.find("/")) != std::string::npos) {
		app = url.substr(0, pos);
		url = url.substr(app.length() + 1);
	}
	else {
		app = url;
	}
	stream = url;
}

static void rtmp_onconnect(void* param, int code, aio_socket_t aio)
{
	if (!RTRtmp::Lock(param)) {
		aio_socket_destroy(aio, NULL, NULL);
		return;
	}
	RTRtmp* rtmpClient = (RTRtmp*)param;
	if (rtmpClient != NULL) {
		rtmpClient->OnNetworkConnectRlt(code, aio);
	}

	RTRtmp::UnLock(param);
}

bool RTRtmp::Lock(void*pThis)
{
	rtc::CritScope l(&gCsRtmp);
	MapRTRtmp::iterator iter = gMapRtmp.find(pThis);
	if (iter != gMapRtmp.end()) {
		iter->second++;

		return true;
	}
	return false;
}
void RTRtmp::UnLock(void*pThis)
{
	rtc::CritScope l(&gCsRtmp);
	MapRTRtmp::iterator iter = gMapRtmp.find(pThis);
	if (iter != gMapRtmp.end()) {
		iter->second--;
	}
}

//===============================================================
//
RTRtmp::RTRtmp(void)
	: rtmp_client_(NULL)
	, rtmp_started_(false)
	, rtmp_connected_(false)
	, rtmp_need_reconnect_(false)
	, rtmp_destory_(false)
{
	memset(&rtmp_client_handler_, 0, sizeof(rtmp_client_handler_));

	RtmpIO::Inst().Attach(this);
}

RTRtmp::~RTRtmp(void) 
{
	RTC_CHECK(rtmp_destory_);
	RtmpIO::Inst().Detach(this);
}

void RTRtmp::Attach(void*pThis)
{
	rtc::CritScope l(&gCsRtmp);
	if (gMapRtmp.find(pThis) == gMapRtmp.end())
	{
		gMapRtmp[pThis] = 0;
	}
}
void RTRtmp::Detach(void*pThis)
{
	rtc::CritScope l(&gCsRtmp);
	MapRTRtmp::iterator iter = gMapRtmp.find(pThis);
	if (iter != gMapRtmp.end()) {
		if (iter->second != 0) {
			gCsRtmp.Leave();
			rtc::Thread::SleepMs(1);
			gCsRtmp.Enter();
		}

		gMapRtmp.erase(iter);
	}
}
void RTRtmp::Destory()
{
	rtmp_destory_ = true;	// ���ٺ�Ͳ����ٷ�������

	{
		rtc::CritScope l(&cs_send_data_);
		while (lst_send_data_.size() > 0) {
			delete lst_send_data_.front();
			lst_send_data_.pop_front();
		}
	}
	{
		rtc::CritScope l(&cs_cache_data_);
		while (lst_cache_data_.size() > 0) {
			delete lst_cache_data_.front();
			lst_cache_data_.pop_front();
		}
	}
}
void RTRtmp::DoRtmpTick()
{
	if (rtmp_need_reconnect_) {
		rtmp_need_reconnect_ = false;
		rtmp_connect_timeout_ = rtc::TimeUTCMillis() + 3500;
		if (rtmp_client_ != NULL) {
			aio_rtmp_client_destroy(rtmp_client_);
			rtmp_client_ = NULL;
		}

		if (str_rtmp_url_.length() > 0) {
			std::string schema, host, app, port, stream;
			rtmp_discovery_tc_url(str_rtmp_url_, schema, host, app, port, stream);

			char tcurl[1024] = { 0 };
			snprintf(tcurl, sizeof(tcurl), "rtmp://%s/%s", host.c_str(), app.c_str()); // tcurl
			str_rtmp_tcurl_ = tcurl;
			str_rtmp_app_ = app;
			str_rtmp_stream_ = stream;
			aio_connect(host.c_str(), atoi(port.c_str()), 3000, rtmp_onconnect, RtmpPtr());
		}
	}

	if (rtmp_connect_timeout_ != 0 && rtmp_connect_timeout_ <= rtc::TimeUTCMillis()) {
		rtmp_need_reconnect_ = true;
		rtmp_connect_timeout_ = 0;
	}
}

void RTRtmp::SendData(int nType, const char*pData, int nLen, uint32_t timestamp)
{
	if (rtmp_destory_ || pData == NULL || nLen == 0)
		return;
	rtc::CritScope l(&cs_send_data_);
	RtmpData* rtmpData = new RtmpData();
	rtmpData->SetData(pData, nLen);
	rtmpData->nType = nType;
	rtmpData->nTimestamp = timestamp;
	lst_send_data_.push_back(rtmpData);
}
void RTRtmp::CacheData(int nType, const char*pData, int nLen, uint32_t timestamp)
{
	if (rtmp_destory_ || pData == NULL || nLen == 0)
		return;
	rtc::CritScope l(&cs_cache_data_);
	RtmpData* rtmpData = new RtmpData();
	rtmpData->SetData(pData, nLen);
	rtmpData->nType = nType;
	rtmpData->nTimestamp = timestamp;
	lst_cache_data_.push_back(rtmpData);
}
void RTRtmp::DoGetData()
{
	RtmpData* rtmpData = NULL;
	{
		rtc::CritScope l(&cs_cache_data_);
		if (lst_cache_data_.size() > 0) {
			rtmpData = lst_cache_data_.front();
			lst_cache_data_.pop_front();
		}
	}

	if (rtmpData != NULL) {
		OnGetData(rtmpData->nType, rtmpData->pData, rtmpData->nLen, rtmpData->nTimestamp);
		delete rtmpData;
		rtmpData = NULL;
	}
}

//* For RtmpIOTick
void RTRtmp::OnRtmpIOTick()
{
	OnTick();

	DoRtmpTick();

	RtmpData* rtmpData = NULL;
	{
		rtc::CritScope l(&cs_send_data_);
		if (lst_send_data_.size() > 0) {
			rtmpData = lst_send_data_.front();
			lst_send_data_.pop_front();
		}
	}

	if (rtmpData != NULL) {
		OnSendData(rtmpData->nType, rtmpData->pData, rtmpData->nLen, rtmpData->nTimestamp);
		delete rtmpData;
		rtmpData = NULL;
	}
}

void RTRtmp::OnRtmpIODetach()
{
	if (rtmp_client_ != NULL) {
		aio_rtmp_client_destroy(rtmp_client_);
		rtmp_client_ = NULL;
	}
}