#include <jni.h>
#include <jni_helpers.h>
#include "ArHttpClient.h"
#include "rtc_base/synchronization/mutex.h"
#include "util/ClassreferenceHolder.h"

class ArHttpClient
{
public:
	ArHttpClient(void);
	virtual ~ArHttpClient(void);
	static ArHttpClient*The();

	int DoRequest(void*ptr, ArHttpClientResponse resp, ArHttpVerb eHttpVerb, int nTimeout, const std::string&strReqUrl, ArHttpHeaders&httpHeahders, const std::string&strContent);
	int UnRequest(void*ptr);

	void GetResponse(void*ptr, int nCode, const std::string&strResponse);

protected:
	struct HttpReq
	{
		HttpReq():m_jJavaObj(NULL) {

		}

		void StartReq(int nTimeout)
        {

			JNIEnv *env = webrtc_jni::AttachCurrentThreadIfNeeded();
			jclass httpClass = webrtc_loader::FindClass(env,"io/anyrtc/live/util/AsyncHttpURLConnection");
			jmethodID initMethod =env->GetMethodID(httpClass,"<init>", "(J)V");
			m_jJavaObj = env->NewGlobalRef(env->NewObject(httpClass,initMethod, reinterpret_cast<jlong>(ptr)));
			jmethodID sendRequest = env->GetMethodID(httpClass,"Call", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
			jstring strMethod = env->NewStringUTF("POST");
			jstring strUrl = env->NewStringUTF(strReqUrl.c_str());
			jstring strParams = env->NewStringUTF(strContent.c_str());
			jstring strContentType = env->NewStringUTF("application/x-www-form-urlencoded");
            env->CallVoidMethod(m_jJavaObj,sendRequest,strMethod,strUrl,strParams,strContentType);
        }
		void* ptr;
		ArHttpVerb eHttpVerb;
		ArHttpClientResponse cbResp;

		std::string strReqUrl;
		ArHttpHeaders httpHeahders;
		std::string strContent;

		jobject m_jJavaObj;
	};
private:

    typedef std::map<void*, HttpReq> MapHttpReq;
    webrtc::Mutex cs_map_http_req_;
    MapHttpReq	map_http_req_;

};




int ArHttpClientDoRequest(void*ptr, ArHttpClientResponse resp, ArHttpVerb eHttpVerb, int nTimeout, const std::string&strReqUrl, ArHttpHeaders&httpHeahders, const std::string&strContent)
{
	return ArHttpClient::The()->DoRequest(ptr, resp, eHttpVerb, nTimeout, strReqUrl, httpHeahders, strContent);
}
int ArHttpClientUnRequest(void*ptr)
{
	return ArHttpClient::The()->UnRequest(ptr);
}


ArHttpClient*ArHttpClient::The()
{
	static ArHttpClient gShareThread;
	return &gShareThread;
}

ArHttpClient::ArHttpClient(void)
{

};
ArHttpClient::~ArHttpClient(void)
{

};

int ArHttpClient::DoRequest(void*ptr, ArHttpClientResponse cbResp, ArHttpVerb eHttpVerb, int nTimeout, const std::string&strReqUrl, ArHttpHeaders&httpHeahders, const std::string&strContent)
{
	if (nTimeout < 0) {
		nTimeout = 1000;
	}
	if (nTimeout > 90000) {
		nTimeout = 90000;
	}
	webrtc::MutexLock l(&cs_map_http_req_);
	if (map_http_req_.find(ptr) != map_http_req_.end()) {
		return -1;
	}

	HttpReq&httpReq = map_http_req_[ptr];
	httpReq.ptr = ptr;
	httpReq.eHttpVerb = eHttpVerb;
	httpReq.cbResp = cbResp;
	httpReq.strReqUrl = strReqUrl;
	httpReq.httpHeahders = httpHeahders;
	httpReq.strContent = strContent;

    httpReq.StartReq(nTimeout);

	return 0;
}
int ArHttpClient::UnRequest(void*ptr)
{
	webrtc::MutexLock l(&cs_map_http_req_);
	if (map_http_req_.find(ptr) != map_http_req_.end()) {
		HttpReq&httpReq = map_http_req_[ptr];
		map_http_req_.erase(ptr);
	}
	return 0;
}


void ArHttpClient::GetResponse(void*ptr, int nCode, const std::string&strResponse)
{
	webrtc::MutexLock l(&cs_map_http_req_);


	if (map_http_req_.find(ptr) != map_http_req_.end()) {
		HttpReq&httpReq = map_http_req_[ptr];
		if(httpReq.cbResp != NULL) {
			ArHttpHeaders httpHdr;
			httpReq.cbResp(httpReq.ptr, AHC_OK, httpHdr, strResponse);
		}
	}
}


extern "C"
JNIEXPORT void JNICALL
Java_io_anyrtc_live_util_AsyncHttpEventsImpl_setHttpError(JNIEnv *env, jobject thiz,long ptr,
														  jstring error) {



}
extern "C"
JNIEXPORT void JNICALL
Java_io_anyrtc_live_util_AsyncHttpEventsImpl_setHttpComplete(JNIEnv *env, jobject thiz,long ptr,
															 jstring response) {
	std::string result = webrtc::JavaToStdString(env, response);
	ArHttpClient::The()->GetResponse((void*)ptr, 200, result);
}