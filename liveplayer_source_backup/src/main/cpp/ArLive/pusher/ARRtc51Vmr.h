#ifndef __AR_RTC_51_VMR_H__
#define __AR_RTC_51_VMR_H__
#include "ARPusher.h"
#include "api/media_stream_interface.h"
#include "api/peer_connection_interface.h"
#include "rtc_base/thread.h"

// 给api.51vmr.cn专门做的webrtc推拉流适配
class ARRtc51Vmr : public ARPusher, public webrtc::PeerConnectionObserver, public webrtc::CreateSessionDescriptionObserver, public rtc::VideoSinkInterface<webrtc::VideoFrame>, public webrtc::AudioTrackSinkInterface
{
public:
	ARRtc51Vmr(void);
	virtual ~ARRtc51Vmr(void);

	//* For ARPusher
	virtual int startTask(const char* strUrl);
	virtual int stopTask();

	virtual int runOnce();
	virtual int setAudioEnable(bool bAudioEnable);
	virtual int setVideoEnable(bool bVideoEnable);
	virtual int setRepeat(bool bEnable);
	virtual int setRetryCountDelay(int nCount, int nDelay);
	virtual int setAudioData(const char* pData, int nLen, uint32_t ts) { return 0; };
	virtual int setVideoData(const char* pData, int nLen, bool bKeyFrame, uint32_t ts) { return 0; };
	virtual int setSeiData(const char* pData, int nLen, uint32_t ts) { return 0; };

	virtual void SetRtcFactory(void* ptr);
	virtual void setRtcVideoSource(void* ptr);

	void ArHttpNewSessionResponse(int nCode, const std::string& strContent);

	void ArHttpResponse(int nCode, const std::string& strContent);

public:
	//
	// PeerConnectionObserver implementation.
	//
	void OnSignalingChange(
		webrtc::PeerConnectionInterface::SignalingState new_state) override {}
	void OnAddTrack(
		rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
		const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
		streams) override;
	void OnRemoveTrack(
		rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
	void OnDataChannel(
		rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {}
	void OnRenegotiationNeeded() override {}
	void OnIceConnectionChange(
		webrtc::PeerConnectionInterface::IceConnectionState new_state) override {}
	void OnIceGatheringChange(
		webrtc::PeerConnectionInterface::IceGatheringState new_state) override;
	void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
	void OnIceConnectionReceivingChange(bool receiving) override {}

	// CreateSessionDescriptionObserver implementation.
	void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
	void OnFailure(webrtc::RTCError error) override;

	// VideoSinkInterface implementation
	void OnFrame(const webrtc::VideoFrame& frame) override;
	//webrtc::AudioTrackSinkInterface implementation
	void OnData(const void* audio_data,
		int bits_per_sample,
		int sample_rate,
		size_t number_of_channels,
		size_t number_of_frames) override;

private:
	rtc::Thread* main_thread_;
	webrtc::PeerConnectionFactoryInterface* peer_connection_factory_;
	webrtc::VideoTrackSourceInterface* video_source_;
	rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;


	bool	b_http_requset_;
	bool	b_audio_enabled_;
	bool	b_video_enabled_;

	std::string str_webrtc_url_;
	std::string str_session_id_;

	std::string str_51vmr_uuid_;
	std::string str_51vmr_token_;

	webrtc::SessionDescriptionInterface* desc_{nullptr};

private:
	webrtc::Mutex	cs_tracks_;
	rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> audio_cap_track_;
	rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> video_cap_track_;

	rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> audio_play_track_;
	rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> video_play_track_;
};

#endif	// __AR_RTC_51_VMR_H__