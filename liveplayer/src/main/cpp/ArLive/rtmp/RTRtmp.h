#ifndef __RT_RTMP_H__
#define __RT_RTMP_H__
#include <string>
#include "libaio/include/aio-connect.h"
#include "include/aio-socket.h"
#include "libaio/include/aio-timeout.h"
#include "librtmp/aio/aio-rtmp-client.h"
#include "../../webrtc/rtc_base/deprecated/recursive_critical_section.h"
#include "../../webrtc/modules/audio_coding/acm2/acm_resampler.h"
#include "RtmpIO.h"

class RTRtmp : public RtmpIOTick
{
public:
	RTRtmp(void);
	virtual ~RTRtmp(void);

	static bool Lock(void*pThis);
	static void UnLock(void*pThis);

	void Destory();
	void DoRtmpTick();
	void SendData(int nType, const char*pData, int nLen, uint32_t timestamp);	// IO线程	
	void CacheData(int nType, const char*pData, int nLen, uint32_t timestamp); // Worker线程
	void DoGetData();

	void* RtmpPtr() { return this; };
	virtual void*ImplPtr() = 0;
	virtual void OnTick() {};
	virtual void OnNetworkConnectRlt(int code, aio_socket_t aio) = 0;
	virtual void OnSendData(int nType, const char*pData, int nLen, uint32_t timestamp) {};
	virtual void OnGetData(int nType, const char*pData, int nLen, uint32_t timestamp) {};

	//* For RtmpIOTick
	virtual void OnRtmpIOTick();
	virtual void OnRtmpIODetach();

protected:
	void Attach(void*pThis);
	void Detach(void*pThis);

protected:
	bool							rtmp_started_;
	bool							rtmp_connected_;
	bool							rtmp_need_reconnect_;
	bool							rtmp_destory_;
	int64_t							rtmp_connect_timeout_;

	aio_rtmp_client_handler_t		rtmp_client_handler_;
	aio_rtmp_client_t				*rtmp_client_;
	std::string						str_rtmp_url_;
	std::string						str_rtmp_tcurl_;
	std::string						str_rtmp_app_;
	std::string						str_rtmp_stream_;

private:
	struct RtmpData
	{
		RtmpData(void): pData(NULL), nType(0), nLen(0), nTimestamp(0) {

		}
		virtual ~RtmpData(void) {
			if (pData != NULL) {
				delete[] pData;
				pData = NULL;
			}
		}
		void SetData(const char*data, int len) {
			if (pData == NULL) {
				nLen = len;
				pData = new char[len];
				memcpy(pData, data, len);
			}
		}
		char* pData;
		int nType;
		int nLen;
		uint32_t nTimestamp;
	};

	rtc::RecursiveCriticalSection cs_send_data_;
	std::list< RtmpData*> lst_send_data_;

	rtc::RecursiveCriticalSection cs_cache_data_;
	std::list< RtmpData*> lst_cache_data_;
};


#endif	// __RT_RTMP_H__
