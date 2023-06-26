#include "ARRtcPusher.h"
#include "rtc_base/bind.h"
#include "rapidjson/json_value.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"	
#include "rapidjson/stringbuffer.h"
#include "ArHttpClient.h"

//#define TECENT_WEBRTC_PUSH_GB  "https://webrtcpush.myqcloud.com/webrtc/v1/pushstream"
#define TECENT_WEBRTC_PUSH_GB  "http://192.168.1.130:9098/anyrtc/v1/pushstream"
const char kAudioLabel[] = "audio_label";
const char kVideoLabel[] = "video_label";
const char kStreamId[] = "stream_id";

static void ArRtcHttpReqResponse(void* ptr, ArHttpCode eCode, ArHttpHeaders& httpHeahders, const std::string& strContent)
{
	ARRtcPusher* rtcPusher = (ARRtcPusher*)ptr;
	rtcPusher->ArHttpResponse((int)eCode, strContent);
}

ARPusher* V2_CALL createRtcPusher()
{
	rtc::scoped_refptr<ARRtcPusher> conductor(
		new rtc::RefCountedObject<ARRtcPusher>());
	return conductor.release();
}

class DummyPusherSetSessionDescriptionObserver
	: public webrtc::SetSessionDescriptionObserver {
public:
	static DummyPusherSetSessionDescriptionObserver* Create() {
		return new rtc::RefCountedObject<DummyPusherSetSessionDescriptionObserver>();
	}
	virtual void OnSuccess() { RTC_LOG(INFO) << __FUNCTION__; }
	virtual void OnFailure(webrtc::RTCError error) {
		RTC_LOG(INFO) << __FUNCTION__ << " " << ToString(error.type()) << ": "
			<< error.message();
	}
};


ARRtcPusher::ARRtcPusher(void)
	: ARPusher()
	, main_thread_(NULL)
	, peer_connection_factory_(NULL)
	, video_source_(NULL)
	, b_http_requset_(false)
	, b_audio_enabled_(true)
	, b_video_enabled_(true)
{
	main_thread_ = rtc::Thread::Current();
}
ARRtcPusher::~ARRtcPusher(void)
{

}

//* For ARPusher
int ARRtcPusher::startTask(const char* strUrl)
{
	if (!main_thread_->IsCurrent()) {
		return main_thread_->Invoke<int>(RTC_FROM_HERE, rtc::Bind(&ARRtcPusher::startTask, this, strUrl));
	}

	if (peer_connection_ == NULL) {
		str_webrtc_url_ = strUrl;
		str_session_id_ = "124rhio23j";

		webrtc::PeerConnectionInterface::RTCConfiguration config;
		config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
		config.enable_dtls_srtp = true;
#if 0
		webrtc::PeerConnectionInterface::IceServer server;
		server.uri = GetPeerConnectionString();
		config.servers.push_back(server);
#endif

		peer_connection_ = peer_connection_factory_->CreatePeerConnection(
			config, nullptr, nullptr, this);
		{
			webrtc::MutexLock l(&cs_tracks_);
			audio_track_ = peer_connection_factory_->CreateAudioTrack(
					kAudioLabel, peer_connection_factory_->CreateAudioSource(
						cricket::AudioOptions()));
			audio_track_->set_enabled(b_audio_enabled_);

			auto result_or_error = peer_connection_->AddTrack(audio_track_, { kStreamId });
			if (!result_or_error.ok()) {
				RTC_LOG(LS_ERROR) << "Failed to add audio track to PeerConnection: "
					<< result_or_error.error().message();
			}

			if (video_source_) {
				webrtc::MutexLock l(&cs_tracks_);
				video_track_ = peer_connection_factory_->CreateVideoTrack(kVideoLabel, video_source_);
				video_track_->set_enabled(b_video_enabled_);

				result_or_error = peer_connection_->AddTrack(video_track_, { kStreamId });
				if (!result_or_error.ok()) {
					RTC_LOG(LS_ERROR) << "Failed to add video track to PeerConnection: "
						<< result_or_error.error().message();
				}
			}
			else {
				RTC_LOG(LS_ERROR) << "OpenVideoCaptureDevice failed";
			}
		}

		webrtc::PeerConnectionInterface::RTCOfferAnswerOptions opts;
		opts.offer_to_receive_audio = false;
		opts.offer_to_receive_video = false;
		peer_connection_->CreateOffer(this, opts);
	}

	return 0;
}
int ARRtcPusher::stopTask()
{
	if (!main_thread_->IsCurrent()) {
		return main_thread_->Invoke<int>(RTC_FROM_HERE, rtc::Bind(&ARRtcPusher::stopTask, this));
	}
	if (b_http_requset_) {
		b_http_requset_ = false;
		ArHttpClientUnRequest(this);
	}

	{
		webrtc::MutexLock l(&cs_tracks_);
		audio_track_ = nullptr;
		video_track_ = nullptr;
	}

	if (peer_connection_ != NULL) {
		peer_connection_->Close();
		peer_connection_ = NULL;
	}
	return 0;
}

int ARRtcPusher::runOnce()
{
	return 0;
}
int ARRtcPusher::setAudioEnable(bool bAudioEnable)
{
	b_audio_enabled_ = bAudioEnable;
	webrtc::MutexLock l(&cs_tracks_);
	if (audio_track_ != NULL) {
		audio_track_->set_enabled(b_audio_enabled_);
	}
	return 0;
}
int ARRtcPusher::setVideoEnable(bool bVideoEnable)
{
	b_video_enabled_ = bVideoEnable;
	webrtc::MutexLock l(&cs_tracks_);
	if (video_track_ != NULL) {
		video_track_->set_enabled(b_video_enabled_);
	}
	return 0;
}
int ARRtcPusher::setRepeat(bool bEnable)
{
	return 0;
}
int ARRtcPusher::setRetryCountDelay(int nCount, int nDelay)
{
	return 0;
}
void ARRtcPusher::SetRtcFactory(void* ptr)
{
	peer_connection_factory_ = (webrtc::PeerConnectionFactoryInterface*)ptr;
}
void ARRtcPusher::setRtcVideoSource(void* ptr)
{
	video_source_ = (webrtc::VideoTrackSourceInterface*)ptr;
}

void ARRtcPusher::ArHttpResponse(int nCode, const std::string& strContent)
{
	rapidjson::Document		jsonReqDoc;
	JsonStr strCopy(strContent.c_str(), static_cast<int>(strContent.length()));
	if (!jsonReqDoc.ParseInsitu<0>(strCopy.Ptr).HasParseError()) {
		int code = GetJsonInt(jsonReqDoc, "errcode", F_AT);
		if (code == 0) {
			std::string strSdp = GetJsonStr(jsonReqDoc, "remotesdp", F_AT);
			if (jsonReqDoc.HasMember("remotesdp") && jsonReqDoc["remotesdp"].IsObject()) {
				/*rapidjson::StringBuffer jsonSdpStr;
				rapidjson::Writer<rapidjson::StringBuffer> jsonSdpWriter(jsonSdpStr);
				jsonReqDoc["remotesdp"].Accept(jsonSdpWriter);*/
				if (peer_connection_ != NULL) {
					std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
						webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, jsonReqDoc["remotesdp"]["sdp"].GetString());
					peer_connection_->SetRemoteDescription(
						DummyPusherSetSessionDescriptionObserver::Create(),
						session_description.release());
				}
			}

		}
	}
}

//
// PeerConnectionObserver implementation.
//
void ARRtcPusher::OnAddTrack(
	rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
	const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
	streams)
{

}
void ARRtcPusher::OnRemoveTrack(
	rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{

}

void ARRtcPusher::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
{

}

// CreateSessionDescriptionObserver implementation.
void ARRtcPusher::OnSuccess(webrtc::SessionDescriptionInterface* desc)
{
	peer_connection_->SetLocalDescription(
		DummyPusherSetSessionDescriptionObserver::Create(), desc);

	std::string sdp;
	desc->ToString(&sdp);

	
	if (!b_http_requset_) {
		b_http_requset_ = true;

		rapidjson::Document		jsonDoc;
		rapidjson::StringBuffer jsonStr;
		rapidjson::Writer<rapidjson::StringBuffer> jsonWriter(jsonStr);
		jsonDoc.SetObject();
		jsonDoc.AddMember("streamurl", str_webrtc_url_.c_str(), jsonDoc.GetAllocator());
		jsonDoc.AddMember("sessionid", str_session_id_.c_str(), jsonDoc.GetAllocator());
		rapidjson::Value objSdp(rapidjson::kObjectType);
		objSdp.AddMember("type", "offer", jsonDoc.GetAllocator());
		objSdp.AddMember("sdp", sdp.c_str(), jsonDoc.GetAllocator());
		jsonDoc.AddMember("localsdp", objSdp, jsonDoc.GetAllocator());

		jsonDoc.Accept(jsonWriter);

		ArHttpHeaders httpHdrs;
		ArHttpClientDoRequest(this, ArRtcHttpReqResponse, AHV_POST, 3000, TECENT_WEBRTC_PUSH_GB, httpHdrs, jsonStr.GetString());
	}
}
void ARRtcPusher::OnFailure(webrtc::RTCError error)
{

}
