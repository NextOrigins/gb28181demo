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
#ifndef __ARP_RTC_PUSHER_H__
#define __ARP_RTC_PUSHER_H__
#include "ARPusher.h"
#include "api/media_stream_interface.h"
#include "api/peer_connection_interface.h"
#include "rtc_base/thread.h"

class ARRtcPusher : public ARPusher, public webrtc::PeerConnectionObserver, public webrtc::CreateSessionDescriptionObserver
{
public:
	ARRtcPusher(void);
	virtual ~ARRtcPusher(void);

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
		webrtc::PeerConnectionInterface::IceGatheringState new_state) override {}
	void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
	void OnIceConnectionReceivingChange(bool receiving) override {}

	// CreateSessionDescriptionObserver implementation.
	void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
	void OnFailure(webrtc::RTCError error) override;


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

private:
	webrtc::Mutex	cs_tracks_;
	rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> audio_track_;
	rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> video_track_;
};

#endif	// __ARP_RTC_PUSHER_H__