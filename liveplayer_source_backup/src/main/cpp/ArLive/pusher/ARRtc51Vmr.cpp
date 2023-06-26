#include "ARRtc51Vmr.h"
#include "rtc_base/bind.h"
#include "pc/session_description.h"
#include "rapidjson/json_value.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"	
#include "rapidjson/stringbuffer.h"
#include "arhttp/ArHttpClient.h"

#define X51VMR_NEW_SESSION "https://api.51vmr.cn:8143/api/services/5000670/new_session"
#define X51VMR_WEBRTC_PUSH_GB  "https://api.51vmr.cn:8143/api/services/5000670/participants/%s/calls"
const char kAudioLabel[] = "audio_label";
const char kVideoLabel[] = "video_label";
const char kStreamId[] = "stream_id";

static void ArRtcHttpNewSessionResponse(void* ptr, ArHttpCode eCode, ArHttpHeaders& httpHeahders, const std::string& strContent)
{
	ARRtc51Vmr* rtcPusher = (ARRtc51Vmr*)ptr;
	rtcPusher->ArHttpNewSessionResponse((int)eCode, strContent);
}

static void ArRtcHttpReqResponse(void* ptr, ArHttpCode eCode, ArHttpHeaders& httpHeahders, const std::string& strContent)
{
	ARRtc51Vmr* rtcPusher = (ARRtc51Vmr*)ptr;
	rtcPusher->ArHttpResponse((int)eCode, strContent);
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

ARPusher* V2_CALL createRtcPusher()
{
	rtc::scoped_refptr<ARRtc51Vmr> conductor(
		new rtc::RefCountedObject<ARRtc51Vmr>());
	return conductor.release();
}
ARRtc51Vmr::ARRtc51Vmr(void)
	: main_thread_(NULL)
	, peer_connection_factory_(NULL)
	, video_source_(NULL)
	, b_http_requset_(false)
	, b_audio_enabled_(true)
	, b_video_enabled_(true)
{
	main_thread_ = rtc::Thread::Current();
}
ARRtc51Vmr::~ARRtc51Vmr(void)
{

}

//* For ARPusher
int ARRtc51Vmr::startTask(const char* strUrl)
{
	if (!main_thread_->IsCurrent()) {
		return main_thread_->Invoke<int>(RTC_FROM_HERE, rtc::Bind(&ARRtc51Vmr::startTask, this, strUrl));
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
			audio_cap_track_ = peer_connection_factory_->CreateAudioTrack(
				kAudioLabel, peer_connection_factory_->CreateAudioSource(
					cricket::AudioOptions()));
			audio_cap_track_->set_enabled(b_audio_enabled_);

			auto result_or_error = peer_connection_->AddTrack(audio_cap_track_, { kStreamId });
			if (!result_or_error.ok()) {
				RTC_LOG(LS_ERROR) << "Failed to add audio track to PeerConnection: "
					<< result_or_error.error().message();
			}

			if (video_source_) {
				webrtc::MutexLock l(&cs_tracks_);
				video_cap_track_ = peer_connection_factory_->CreateVideoTrack(kVideoLabel, video_source_);
				video_cap_track_->set_enabled(b_video_enabled_);

				result_or_error = peer_connection_->AddTrack(video_cap_track_, { kStreamId });
				if (!result_or_error.ok()) {
					RTC_LOG(LS_ERROR) << "Failed to add video track to PeerConnection: "
						<< result_or_error.error().message();
				}
			}
			else {
				RTC_LOG(LS_ERROR) << "OpenVideoCaptureDevice failed";
			}
		}

#if 0
		webrtc::PeerConnectionInterface::RTCOfferAnswerOptions opts;
		opts.offer_to_receive_audio = true;
		opts.offer_to_receive_video = true;
		peer_connection_->CreateOffer(this, opts);
#else
		ArHttpHeaders httpHdrs;
		ArHttpClientUnRequest(this);
		ArHttpClientDoRequest(this, ArRtcHttpNewSessionResponse, AHV_POST, 5000, X51VMR_NEW_SESSION, httpHdrs, "{\"display_name\":\"Eric\",\"hideme\":\"\",\"account\":null,\"support_participant_list\":\"yes\",\"checkdup\":\"df975a6c-4888-4d68-a42c-5e7d87c34451\"}");
#endif
	}

	return 0;
}
int ARRtc51Vmr::stopTask()
{
	if (!main_thread_->IsCurrent()) {
		return main_thread_->Invoke<int>(RTC_FROM_HERE, rtc::Bind(&ARRtc51Vmr::stopTask, this));
	}
	if (b_http_requset_) {
		b_http_requset_ = false;
		ArHttpClientUnRequest(this);
	}

	{
		webrtc::MutexLock l(&cs_tracks_);
		audio_cap_track_ = nullptr;
		video_cap_track_ = nullptr;

		audio_play_track_ = nullptr;
		video_play_track_ = nullptr;
	}

	if (peer_connection_ != NULL) {
		peer_connection_->Close();
		peer_connection_ = NULL;
	}
	return 0;
}

int ARRtc51Vmr::runOnce()
{
	return 0;
}
int ARRtc51Vmr::setAudioEnable(bool bAudioEnable)
{
	b_audio_enabled_ = bAudioEnable;
	webrtc::MutexLock l(&cs_tracks_);
	if (audio_cap_track_ != NULL) {
		audio_cap_track_->set_enabled(b_audio_enabled_);
	}
	return 0;
}
int ARRtc51Vmr::setVideoEnable(bool bVideoEnable)
{
	b_video_enabled_ = bVideoEnable;
	webrtc::MutexLock l(&cs_tracks_);
	if (video_cap_track_ != NULL) {
		video_cap_track_->set_enabled(b_video_enabled_);
	}
	return 0;
}
int ARRtc51Vmr::setRepeat(bool bEnable)
{
	return 0;
}
int ARRtc51Vmr::setRetryCountDelay(int nCount, int nDelay)
{
	return 0;
}
void ARRtc51Vmr::SetRtcFactory(void* ptr)
{
	peer_connection_factory_ = (webrtc::PeerConnectionFactoryInterface*)ptr;
}
void ARRtc51Vmr::setRtcVideoSource(void* ptr)
{
	video_source_ = (webrtc::VideoTrackSourceInterface*)ptr;
}

void ARRtc51Vmr::ArHttpNewSessionResponse(int nCode, const std::string& strContent)
{
	rapidjson::Document		jsonReqDoc;
	JsonStr strCopy(strContent.c_str(), strContent.length());
	if (!jsonReqDoc.ParseInsitu<0>(strCopy.Ptr).HasParseError()) {
		const char* strRlt = GetJsonStr(jsonReqDoc, "status", F_AT);
		if (strcmp("success", strRlt) == 0) {
			if (jsonReqDoc.HasMember("result") && jsonReqDoc["result"].IsObject()) {
				str_51vmr_uuid_ = jsonReqDoc["result"]["participant_uuid"].GetString();
				str_51vmr_token_ = jsonReqDoc["result"]["token"].GetString();
				if (str_51vmr_uuid_.length() > 0) {
					webrtc::PeerConnectionInterface::RTCOfferAnswerOptions opts;
					opts.offer_to_receive_audio = true;
					opts.offer_to_receive_video = true;
					opts.num_simulcast_layers = 1;
					peer_connection_->CreateOffer(this, opts);
				}
			}
		}
	}
}

void ARRtc51Vmr::ArHttpResponse(int nCode, const std::string& strContent)
{
	bool reqOK = false;
	rapidjson::Document		jsonReqDoc;
	JsonStr strCopy(strContent.c_str(), strContent.length());
	if (!jsonReqDoc.ParseInsitu<0>(strCopy.Ptr).HasParseError()) {
		const char*strRlt = GetJsonStr(jsonReqDoc, "status", F_AT);
		if (strcmp("success", strRlt) == 0) {
			reqOK = true;
			RTC_LOG(INFO) << "Recv Answer: " << strContent;
			if (jsonReqDoc.HasMember("result") && jsonReqDoc["result"].IsObject()) {
				/*rapidjson::StringBuffer jsonSdpStr;
				rapidjson::Writer<rapidjson::StringBuffer> jsonSdpWriter(jsonSdpStr);
				jsonReqDoc["remotesdp"].Accept(jsonSdpWriter);*/
				if (peer_connection_ != NULL) {
					const char* strSdp = jsonReqDoc["result"]["sdp"].GetString();
					std::string copySdp = strSdp;
					copySdp = copySdp.replace(copySdp.find("BUNDLE audio video"), strlen("BUNDLE audio video"), "BUNDLE 0 1");
					copySdp = copySdp.replace(copySdp.find("mid:audio"), strlen("mid:audio"), "mid:0");
					copySdp = copySdp.replace(copySdp.find("mid:video"), strlen("mid:video"), "mid:1");
					std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
						webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, copySdp);
					if(0)
					{
						cricket::SessionDescription* sessionDesc = session_description->description();
						if (sessionDesc != NULL) {
							{
								const cricket::ContentGroups& groups = sessionDesc->groups();
								cricket::ContentGroups::const_iterator itgr = groups.begin();
								while (itgr != groups.end()) {
									const cricket::ContentGroup&group = *itgr;
									cricket::ContentGroup* pGrp = (cricket::ContentGroup*)&group;
									if (pGrp->HasContentName("audio")) {
										pGrp->RemoveContentName("audio");
										pGrp->AddContentName("0");
									}
									if (pGrp->HasContentName("video")) {
										pGrp->RemoveContentName("video");
										pGrp->AddContentName("1");
									}
									itgr++;
								}
							}
							{
								const cricket::ContentInfos& contentInfo = sessionDesc->contents();
								cricket::ContentInfos::const_iterator itcr = contentInfo.begin();
								while (itcr != contentInfo.end()) {
									const cricket::ContentInfo& content = *itcr;
									cricket::ContentInfo* pCont = (cricket::ContentInfo*)&content;
									if (pCont->mid().compare("audio") == 0) {
										pCont->set_mid("0");
									}
									else if (pCont->mid().compare("video") == 0) {
										pCont->set_mid("1");
									}
									itcr++;
								}
							}
							if(0)
							{
								const cricket::TransportInfos&  transportInfos = sessionDesc->transport_infos();
								cricket::TransportInfos::const_iterator ittr = transportInfos.begin();
								while (ittr != transportInfos.end()) {
									const cricket::TransportInfo& transportInfo = *ittr;
									cricket::TransportInfo* pTrans = (cricket::TransportInfo*)&transportInfo;
									if (pTrans->content_name.compare("audio") == 0) {
										pTrans->content_name = "0";
									}
									if (pTrans->content_name.compare("video") == 0) {
										pTrans->content_name = "1";
									}
									ittr++;
								}
							}
						}	
#if 0
						{
							std::vector<cricket::Candidate> candidates;
							session_description->RemoveCandidates(candidates);
							for (int i = 0; i < candidates.size(); i++) {
								const cricket::Candidate& iceCandi = candidates[i];
								std::unique_ptr<webrtc::IceCandidateInterface> copyCadi = webrtc::CreateIceCandidate("0", 0, iceCandi);
								session_description->AddCandidate(copyCadi.release());
							}
						}
						for (int i = 0; i < session_description->number_of_mediasections(); i++) {
							const webrtc::IceCandidateCollection*  candi = session_description->candidates(i);
							for (int j = 0; j < candi->count(); j++) {
								const webrtc::IceCandidateInterface* iceCandi = candi->at(j);
								if (iceCandi != NULL) {
									if (iceCandi->sdp_mid().compare("audio") == 0) {
										std::string strMid = "0";
										std::unique_ptr<webrtc::IceCandidateInterface> copyCadi = webrtc::CreateIceCandidate(strMid, iceCandi->sdp_mline_index(), iceCandi->candidate());
										session_description->RemoveCandidates(iceCandi);
									}
									else if (iceCandi->sdp_mid().compare("video") == 0) {
										std::string strMid = "1";
										std::unique_ptr<webrtc::IceCandidateInterface> copyCadi = webrtc::CreateIceCandidate(strMid, iceCandi->sdp_mline_index(), iceCandi->candidate());
									}
								}
							}
						}
#endif
					}

					//std::string sdp;
					//session_description->ToString(&sdp);
#if 0
					sdp = sdp.replace(sdp.find("m=audio"), 7, "m=0");
					sdp = sdp.replace(sdp.find("m=video"), 7, "m=1");
					session_description =
						webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, sdp.c_str());
#endif
					//RTC_LOG(INFO) << "Recv Answer sdp: " << sdp;
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
void ARRtc51Vmr::OnAddTrack(
	rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
	const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
	streams)
{
	if (receiver->track()->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
		auto* video_track = static_cast<webrtc::VideoTrackInterface*>(receiver->track().get());
		video_track->AddOrUpdateSink(this, rtc::VideoSinkWants());

		webrtc::MutexLock l(&cs_tracks_);
		video_play_track_ = receiver->track();
		video_play_track_->set_enabled(b_video_enabled_);

	}
	else if (receiver->track()->kind() == webrtc::MediaStreamTrackInterface::kAudioKind) {
		auto* audio_track = static_cast<webrtc::AudioTrackInterface*>(receiver->track().get());
		audio_track->AddSink(this);

		webrtc::MutexLock l(&cs_tracks_);
		audio_play_track_ = receiver->track();
		audio_play_track_->set_enabled(b_audio_enabled_);
	}
}
void ARRtc51Vmr::OnRemoveTrack(
	rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
	if (receiver->track()->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
		webrtc::MutexLock l(&cs_tracks_);
		video_play_track_ = nullptr;
	}
	else if (receiver->track()->kind() == webrtc::MediaStreamTrackInterface::kAudioKind) {
		webrtc::MutexLock l(&cs_tracks_);
		audio_play_track_ = nullptr;
	}
}

void ARRtc51Vmr::OnIceGatheringChange(
	webrtc::PeerConnectionInterface::IceGatheringState new_state)
{
	if (new_state == webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringComplete && desc_ != NULL) {
		std::string sdp;
		desc_->ToString(&sdp);

		if (!b_http_requset_) {
			b_http_requset_ = true;

			rapidjson::Document		jsonDoc;
			rapidjson::StringBuffer jsonStr;
			rapidjson::Writer<rapidjson::StringBuffer> jsonWriter(jsonStr);
			jsonDoc.SetObject();
			jsonDoc.AddMember("call_type", "WEBRTC", jsonDoc.GetAllocator());
			jsonDoc.AddMember("sdp", sdp.c_str(), jsonDoc.GetAllocator());
			jsonDoc.AddMember("clayout", "1:4", jsonDoc.GetAllocator());
			jsonDoc.AddMember("multistream", true, jsonDoc.GetAllocator());
			jsonDoc.AddMember("simulcast", true, jsonDoc.GetAllocator());
			jsonDoc.Accept(jsonWriter);

			RTC_LOG(INFO) << "Send Offer: " << jsonStr.GetString();

			ArHttpHeaders httpHdrs;
			char strReqUrl[1024];
			sprintf(strReqUrl, X51VMR_WEBRTC_PUSH_GB, str_51vmr_uuid_.c_str());
			ArHttpClientUnRequest(this);
			httpHdrs["token"] = str_51vmr_token_;
			ArHttpClientDoRequest(this, ArRtcHttpReqResponse, AHV_POST, 5000, strReqUrl, httpHdrs, jsonStr.GetString());
		}
	}
}
void ARRtc51Vmr::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
{
	if (desc_ != NULL) {
		desc_->AddCandidate(candidate);
	}
}

// CreateSessionDescriptionObserver implementation.
void ARRtc51Vmr::OnSuccess(webrtc::SessionDescriptionInterface* desc)
{
#if 0
	cricket::SessionDescription*  sessionDesc = desc->description();
	if (sessionDesc != NULL) {
		const cricket::ContentInfos& contentInfo = sessionDesc->contents();
		cricket::ContentInfos::const_iterator itcr = contentInfo.begin();
		while (itcr != contentInfo.end()) {
			const cricket::ContentInfo &content = *itcr;
			cricket::ContentInfo* pCont = (cricket::ContentInfo*)&content;
			if (pCont->mid().compare("0") == 0) {
				pCont->set_mid("audio");
			}
			else if (pCont->mid().compare("1") == 0) {
				pCont->set_mid("video");
			}
			itcr++;
		}
	}
#endif
	peer_connection_->SetLocalDescription(
		DummyPusherSetSessionDescriptionObserver::Create(), desc);

	desc_ = desc;
	
}
void ARRtc51Vmr::OnFailure(webrtc::RTCError error)
{

}

// VideoSinkInterface implementation
void ARRtc51Vmr::OnFrame(const webrtc::VideoFrame& frame)
{
}

//webrtc::AudioTrackSinkInterface implementation
void ARRtc51Vmr::OnData(const void* audio_data,
	int bits_per_sample,
	int sample_rate,
	size_t number_of_channels,
	size_t number_of_frames)
{
}