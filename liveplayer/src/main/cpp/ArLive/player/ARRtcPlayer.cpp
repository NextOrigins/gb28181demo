#include "ARRtcPlayer.h"
#include "rtc_base/bind.h"
#include "common_audio/signal_processing/include/signal_processing_library.h"
#include "rapidjson/json_value.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"	
#include "rapidjson/stringbuffer.h"
#include "ArHttpClient.h"

//#define TECENT_WEBRTC_PULL_GB "https://overseas-webrtc.liveplay.myqcloud.com/webrtc/v1/pullstream"
#define TECENT_WEBRTC_PULL_GB  "http://192.168.1.130:9098/anyrtc/v1/pullstream"

static void ArRtcHttpReqResponse(void* ptr, ArHttpCode eCode, ArHttpHeaders& httpHeahders, const std::string& strContent)
{
	ARRtcPlayer* rtcPlayer = (ARRtcPlayer*)ptr;
	rtcPlayer->ArHttpResponse((int)eCode, strContent);
}

ARPlayer* ARP_CALL createRtcPlayer(ARPlayerEvent&callback)
{
	rtc::scoped_refptr<ARRtcPlayer> conductor(
		new rtc::RefCountedObject<ARRtcPlayer>(callback));
	return conductor.release();
}

class DummyPlayerSetSessionDescriptionObserver
	: public webrtc::SetSessionDescriptionObserver {
public:
	static DummyPlayerSetSessionDescriptionObserver* Create() {
		return new rtc::RefCountedObject<DummyPlayerSetSessionDescriptionObserver>();
	}
	virtual void OnSuccess() { RTC_LOG(INFO) << __FUNCTION__; }
	virtual void OnFailure(webrtc::RTCError error) {
		RTC_LOG(INFO) << __FUNCTION__ << " " << ToString(error.type()) << ": "
			<< error.message();
	}
};

ARRtcPlayer::ARRtcPlayer(ARPlayerEvent& callback)
	: ARPlayer(callback)
	, main_thread_(NULL)
	, peer_connection_factory_(NULL)
	, b_http_requset_(false)
	, b_audio_enabled_(true)
	, b_video_enabled_(true)
	, e_rtc_vendor_(RV_Normal)
	, p_aud_sonic_(NULL)
{
	main_thread_ = rtc::Thread::Current();
}
ARRtcPlayer::~ARRtcPlayer(void)
{
}

//* For ARPlayer
int ARRtcPlayer::StartTask(const char*strUrl)
{
	if (!main_thread_->IsCurrent()) {
		return main_thread_->Invoke<int>(RTC_FROM_HERE, rtc::Bind(&ARRtcPlayer::StartTask, this, strUrl));
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

		webrtc::PeerConnectionInterface::RTCOfferAnswerOptions opts;
		opts.offer_to_receive_audio = true;
		opts.offer_to_receive_video = true;
		peer_connection_->CreateOffer(this, opts);
	}

	
	return 0;
}
void ARRtcPlayer::StopTask()
{
	if (!main_thread_->IsCurrent()) {
		return main_thread_->Invoke<void>(RTC_FROM_HERE, rtc::Bind(&ARRtcPlayer::StopTask, this));
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

	if (p_aud_sonic_ != NULL) {
		sonicDestroyStream(p_aud_sonic_);
		p_aud_sonic_ = NULL;
	}
}

void ARRtcPlayer::RunOnce()
{
}
void ARRtcPlayer::SetAudioEnable(bool bAudioEnable)
{
	b_audio_enabled_ = bAudioEnable;
	webrtc::MutexLock l(&cs_tracks_);
	if (audio_track_ != NULL) {
		audio_track_->set_enabled(b_audio_enabled_);
	}
}
void ARRtcPlayer::SetVideoEnable(bool bVideoEnable)
{
	b_video_enabled_ = bVideoEnable;
	webrtc::MutexLock l(&cs_tracks_);
	if (video_track_ != NULL) {
		video_track_->set_enabled(b_video_enabled_);
	}
}
// true: repeat
void ARRtcPlayer::SetRepeat(bool bEnable)
{
}
void ARRtcPlayer::SetUseTcp(bool bUseTcp)
{
}
void ARRtcPlayer::SetNoBuffer(bool bNoBuffer)
{
}
// -1: loop forever >=0 repeat count
void ARRtcPlayer::SetRepeatCount(int loopCount)
{
}
void ARRtcPlayer::SeekTo(int nSeconds)
{
}
void ARRtcPlayer::SetSpeed(float fSpeed)
{

}
void ARRtcPlayer::SetVolume(float fVolume)
{
	user_set_.f_aud_volume_ = fVolume;
}
void ARRtcPlayer::EnableVolumeEvaluation(int32_t intervalMs)
{
	user_set_.n_volume_evaluation_interval_ms_ = intervalMs;
	user_set_.n_volume_evaluation_used_ms_ = 0;
}
int ARRtcPlayer::GetTotalDuration()
{
	return 0;
}
void ARRtcPlayer::SetRtcFactory(void* ptr)
{
	peer_connection_factory_ = (webrtc::PeerConnectionFactoryInterface*)ptr;
}


void ARRtcPlayer::Config(bool bAuto, int nCacheTime, int nMinCacheTime, int nMaxCacheTime, int nVideoBlockThreshold)
{
}

void ARRtcPlayer::ArHttpResponse(int nCode, const std::string& strContent)
{
	bool reqOK = false;
	rapidjson::Document		jsonReqDoc;
	JsonStr strCopy(strContent.c_str(), strContent.length());
	if (!jsonReqDoc.ParseInsitu<0>(strCopy.Ptr).HasParseError()) {
		if (e_rtc_vendor_ == RV_Normal) {
			int nCode = GetJsonInt(jsonReqDoc, "code", F_AT);
			if (nCode == 0) {
				reqOK = true;
				std::string strSdp = GetJsonStr(jsonReqDoc, "sdp", F_AT);
				if (peer_connection_ != NULL) {
					std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
						webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, strSdp.c_str());
					peer_connection_->SetRemoteDescription(
						DummyPlayerSetSessionDescriptionObserver::Create(),
						session_description.release());
				}
			}
		}
		else {
			int nCode = GetJsonInt(jsonReqDoc, "errcode", F_AT);
			if (nCode == 0) {
				reqOK = true;
				std::string strSdp = GetJsonStr(jsonReqDoc, "remotesdp", F_AT);
				if (jsonReqDoc.HasMember("remotesdp") && jsonReqDoc["remotesdp"].IsObject()) {
					/*rapidjson::StringBuffer jsonSdpStr;
					rapidjson::Writer<rapidjson::StringBuffer> jsonSdpWriter(jsonSdpStr);
					jsonReqDoc["remotesdp"].Accept(jsonSdpWriter);*/
					if (peer_connection_ != NULL) {
						std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
							webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, jsonReqDoc["remotesdp"]["sdp"].GetString());
						peer_connection_->SetRemoteDescription(
							DummyPlayerSetSessionDescriptionObserver::Create(),
							session_description.release());
					}
				}

			}
		}
		
	}
}

//
// PeerConnectionObserver implementation.
//
void ARRtcPlayer::OnAddTrack(
	rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
	const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
	streams)
{
	if (receiver->track()->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
		auto* video_track = static_cast<webrtc::VideoTrackInterface*>(receiver->track().get());
		video_track->AddOrUpdateSink(this, rtc::VideoSinkWants());

		webrtc::MutexLock l(&cs_tracks_);
		video_track_ = receiver->track();
		video_track_->set_enabled(b_video_enabled_);
		
	}
	else if (receiver->track()->kind() == webrtc::MediaStreamTrackInterface::kAudioKind) {
		auto* audio_track = static_cast<webrtc::AudioTrackInterface*>(receiver->track().get());
		audio_track->AddSink(this);

		webrtc::MutexLock l(&cs_tracks_);
		audio_track_ = receiver->track();
		audio_track_->set_enabled(b_audio_enabled_);
	}
}
void ARRtcPlayer::OnRemoveTrack(
	rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
	if (receiver->track()->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
		webrtc::MutexLock l(&cs_tracks_);
		video_track_ = nullptr;
	}
	else if (receiver->track()->kind() == webrtc::MediaStreamTrackInterface::kAudioKind) {
		webrtc::MutexLock l(&cs_tracks_);
		audio_track_ = nullptr;
	}
}
void ARRtcPlayer::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
{

}

// CreateSessionDescriptionObserver implementation.
void ARRtcPlayer::OnSuccess(webrtc::SessionDescriptionInterface* desc)
{
	peer_connection_->SetLocalDescription(
		DummyPlayerSetSessionDescriptionObserver::Create(), desc);

	std::string sdp;
	desc->ToString(&sdp);

	if (!b_http_requset_) {
		b_http_requset_ = true;

		{
			//rapidjson::Document		jsonOfferDoc;
			//jsonOfferDoc.ParseInsitu<0>((char*)sdp.c_str());

			rapidjson::Document		jsonDoc;
			rapidjson::StringBuffer jsonStr;
			rapidjson::Writer<rapidjson::StringBuffer> jsonWriter(jsonStr);
			jsonDoc.SetObject();
			std::string strApiUrl;
			if (e_rtc_vendor_ == RV_Normal) {
				/*
				* api: "http://117.71.98.4:40026/rtc/v1/play/"
				clientip: null
				sdp: "v=0\r\no=- 9026116847439648398 2 IN IP4 127.0.0.1\r\ns=-\r\nt=0 0\r\nxxx"
				streamurl: "webrtc://117.71.98.4:40026/live/489235729379479"
				tid: "357570a"
				*/
				{
					char strUrl[512];
					const char* ptr = str_webrtc_url_.c_str() + strlen("webrtc://");
					const char* findT = strstr(ptr, "/");
					int nLen = strlen(ptr);
					if (findT != NULL) {
						nLen = findT - ptr;
					}
					sprintf(strUrl, "http://%.*s/rtc/v1/play/", nLen, ptr);
					strApiUrl = strUrl;
				}
				jsonDoc.AddMember("streamurl", str_webrtc_url_.c_str(), jsonDoc.GetAllocator());
				jsonDoc.AddMember("tid", str_session_id_.c_str(), jsonDoc.GetAllocator());
				jsonDoc.AddMember("api", strApiUrl.c_str(), jsonDoc.GetAllocator());
				jsonDoc.AddMember("clientip", "127.0.0.1", jsonDoc.GetAllocator());
				jsonDoc.AddMember("sdp", sdp.c_str(), jsonDoc.GetAllocator());
			}
			else {
				strApiUrl = TECENT_WEBRTC_PULL_GB;
				jsonDoc.AddMember("streamurl", str_webrtc_url_.c_str(), jsonDoc.GetAllocator());
				jsonDoc.AddMember("sessionid", str_session_id_.c_str(), jsonDoc.GetAllocator());
				jsonDoc.AddMember("clientinfo", "chrome_94", jsonDoc.GetAllocator());
				rapidjson::Value objSdp(rapidjson::kObjectType);
				objSdp.AddMember("type", "offer", jsonDoc.GetAllocator());
				objSdp.AddMember("sdp", sdp.c_str(), jsonDoc.GetAllocator());
				jsonDoc.AddMember("localsdp", objSdp, jsonDoc.GetAllocator());
			}

			jsonDoc.Accept(jsonWriter);

			ArHttpHeaders httpHdrs;
			ArHttpClientDoRequest(this, ArRtcHttpReqResponse, AHV_POST, 3000, strApiUrl, httpHdrs, jsonStr.GetString());
		}
	}
}
void ARRtcPlayer::OnFailure(webrtc::RTCError error)
{

}

// VideoSinkInterface implementation
void ARRtcPlayer::OnFrame(const webrtc::VideoFrame& frame)
{
	uint8_t* pData[3];
	int linesize[3];
	pData[0] = (uint8_t*)frame.video_frame_buffer()->GetI420()->DataY();
	pData[1] = (uint8_t*)frame.video_frame_buffer()->GetI420()->DataU();
	pData[2] = (uint8_t*)frame.video_frame_buffer()->GetI420()->DataV();
	linesize[0] = frame.video_frame_buffer()->GetI420()->StrideY();
	linesize[1] = frame.video_frame_buffer()->GetI420()->StrideU();
	linesize[2] = frame.video_frame_buffer()->GetI420()->StrideV();
	callback_.OnArPlyVideo(this, 0, frame.width(), frame.height(), pData, linesize, rtc::TimeUTCMillis());
}

//webrtc::AudioTrackSinkInterface implementation
void ARRtcPlayer::OnData(const void* audio_data,
	int bits_per_sample,
	int sample_rate,
	size_t number_of_channels,
	size_t number_of_frames)
{
	if (user_set_.f_aud_volume_ != 1.0 ) {
		if (p_aud_sonic_ == NULL) {
			p_aud_sonic_ = sonicCreateStream(sample_rate, number_of_channels);
		}
	}
	else {
		if (p_aud_sonic_ != NULL) {
			sonicDestroyStream(p_aud_sonic_);
			p_aud_sonic_ = NULL;
		}
	}
	if (user_set_.n_volume_evaluation_interval_ms_ > 0) {
		user_set_.n_volume_evaluation_used_ms_ += 10;
		if (user_set_.n_volume_evaluation_used_ms_ >= user_set_.n_volume_evaluation_interval_ms_) {
			user_set_.n_volume_evaluation_used_ms_ = 0;
			int max_abs = WebRtcSpl_MaxAbsValueW16((int16_t*)(audio_data), number_of_frames * number_of_channels);
			max_abs = (max_abs * 100) / 32767;
			callback_.OnArPlyVolumeUpdate(this, max_abs);
		}
	}
	if (p_aud_sonic_ != NULL) {
		sonicSetVolume(p_aud_sonic_, user_set_.f_aud_volume_);
		int audFrame10ms = number_of_frames;
		sonicWriteShortToStream(p_aud_sonic_, (short*)audio_data, audFrame10ms);
		int ava = sonicSamplesAvailable(p_aud_sonic_);
		//RTC_LOG(LS_INFO) << "sonicWriteShortToStream: " << audFrame10ms << " ava: " << ava;
		while (sonicSamplesAvailable(p_aud_sonic_) >= audFrame10ms) {
			int rd = sonicReadShortFromStream(p_aud_sonic_, (short*)audio_data, audFrame10ms);
			//RTC_LOG(LS_INFO) << "sonicReadShortFromStream: " << rd;
			if (rd > 0) {
				callback_.OnArPlyAudio(this, (char*)audio_data, sample_rate, number_of_channels, rtc::TimeUTCMillis());
			}
			else {
				break;
			}
		}
	}
	else {
		callback_.OnArPlyAudio(this, (char*)audio_data, sample_rate, number_of_channels, rtc::TimeUTCMillis());
	}
}