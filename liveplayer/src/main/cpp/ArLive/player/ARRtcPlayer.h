/*
*  Copyright (c) 2021 The AnyRTC project authors. All Rights Reserved.
*
*  Please visit https://www.anyrtc.io for detail.
*
* The GNU General Public License is a free, copyleft license for
* software and other kinds of works.
*
* The licenses for most software and other practical works are designed
* to take away your freedom to share and change the works.  By contrast,
* the GNU General Public License is intended to guarantee your freedom to
* share and change all versions of a program--to make sure it remains free
* software for all its users.  We, the Free Software Foundation, use the
* GNU General Public License for most of our software; it applies also to
* any other work released this way by its authors.  You can apply it to
* your programs, too.
* See the GNU LICENSE file for more info.
*/
#ifndef __AR_RTC_PLAYER_H__
#define __AR_RTC_PLAYER_H__
#include "ARPlayer.h"
#include "sonic.h"
#include "api/media_stream_interface.h"
#include "api/peer_connection_interface.h"

enum RtcVendor
{
	RV_Normal = 0,	// srs
	RV_AnyRTC,
	RV_Tecent,
};

class ARRtcPlayer : public ARPlayer, public webrtc::PeerConnectionObserver, public webrtc::CreateSessionDescriptionObserver, public rtc::VideoSinkInterface<webrtc::VideoFrame>, public webrtc::AudioTrackSinkInterface
{
public:
	ARRtcPlayer(ARPlayerEvent&callback);
	virtual ~ARRtcPlayer(void);

	//* For ARPlayer
	virtual int StartTask(const char*strUrl);
	virtual void StopTask();

	virtual void RunOnce();
	virtual void SetAudioEnable(bool bAudioEnable);
	virtual void SetVideoEnable(bool bVideoEnable);
	// true: repeat
	virtual void SetRepeat(bool bEnable);
	virtual void SetUseTcp(bool bUseTcp);
	virtual void SetNoBuffer(bool bNoBuffer);
	// -1: loop forever >=0 repeat count
	virtual void SetRepeatCount(int loopCount);
	virtual void SeekTo(int nSeconds);
	virtual void SetSpeed(float fSpeed);
	virtual void SetVolume(float fVolume);
	virtual void EnableVolumeEvaluation(int32_t intervalMs);
	virtual int GetTotalDuration();
	virtual int GetCurAudDuration() { return 0; };
	virtual int GetCurVidDuration() { return 0; };
	virtual void SetRtcFactory(void* ptr);

	virtual void Config(bool bAuto, int nCacheTime, int nMinCacheTime, int nMaxCacheTime, int nVideoBlockThreshold);

	void ArHttpResponse(int nCode,  const std::string& strContent);

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
		webrtc::PeerConnectionInterface::IceGatheringState new_state) override {}
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
	webrtc::PeerConnectionFactoryInterface *peer_connection_factory_;
	rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;

	bool	b_http_requset_;
	bool	b_audio_enabled_;
	bool	b_video_enabled_;
	RtcVendor	e_rtc_vendor_;

	std::string str_webrtc_url_;
	std::string str_session_id_;

private:
	webrtc::Mutex	cs_tracks_;
	rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> audio_track_;
	rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> video_track_;

	sonicStream		p_aud_sonic_;
};

#endif	// __AR_RTC_PLAYER_H__