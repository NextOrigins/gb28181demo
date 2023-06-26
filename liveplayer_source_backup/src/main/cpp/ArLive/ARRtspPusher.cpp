#include "ARRtspPusher.h"
#include "rtc_base/internal/default_socket_server.h"

ARPusher* V2_CALL createRtspPusher()
{
	return new ARRtspPusher();
}
ARRtspPusher::ARRtspPusher()
	: b_pushed_(false)
	, b_connected_(false)
{
	main_thread_ = rtc::Thread::Current();
}
ARRtspPusher::~ARRtspPusher(void) 
{

}

//* For ARPusher
int ARRtspPusher::startTask(const char* strUrl)
{
	RTC_CHECK(strUrl != NULL && strlen(strUrl) > 0);
	str_rtsp_url_ = strUrl;
	if (!b_pushed_) {
		event_loop_.reset(new xop::EventLoop());
		rtsp_pusher_ = xop::RtspPusher::Create(event_loop_.get());
		xop::MediaSession* session = xop::MediaSession::CreateNew();
		session->AddSource(xop::channel_0, xop::H264Source::CreateNew());
		session->AddSource(xop::channel_1, xop::AACSource::CreateNew(44100, 2, false));
		rtsp_pusher_->AddSession(session);

		if (rtsp_pusher_->OpenUrl(str_rtsp_url_, 3000) < 0) {
			std::cout << "Open " << str_rtsp_url_ << " failed." << std::endl;
			rtsp_pusher_ = nullptr;
			event_loop_ = nullptr;
			return 0;
		}

		b_pushed_ = true;

		if (callback_ != NULL) {
			callback_->onPushStatusUpdate(AR::ArLivePushStatus::ArLivePushStatusConnecting, NULL, NULL);
		}

	}
	return 0;
}
int ARRtspPusher::stopTask()
{
	if (b_pushed_) {
		b_pushed_ = false;
		b_connected_ = false;

		rtsp_pusher_ = nullptr;
		event_loop_ = nullptr;
	}
	return 0;
}

int ARRtspPusher::runOnce()
{
	if (rtsp_pusher_ != NULL) {
		if (rtsp_pusher_->IsConnected()) {
			if (!b_connected_) {
				b_connected_ = true;

				if (callback_ != NULL) {
					callback_->onPushStatusUpdate(AR::ArLivePushStatus::ArLivePushStatusConnectSuccess, NULL, NULL);
				}
			}
		}
		else {
			if (b_connected_) {
				b_connected_ = false;

				if (callback_ != NULL) {
					callback_->onPushStatusUpdate(AR::ArLivePushStatus::ArLivePushStatusConnectSuccess, NULL, NULL);
				}
			}
		}
	}
	return 0;
}
int ARRtspPusher::setRepeat(bool bEnable)
{
	return 0;
}
int ARRtspPusher::setRetryCountDelay(int nCount, int nDelay)
{
	return 0;
}
int ARRtspPusher::setAudioData(const char* pData, int nLen, uint32_t ts)
{
	if (rtsp_pusher_ != NULL && rtsp_pusher_->IsConnected()) {
		//��ȡһ֡ AAC, ���
		xop::AVFrame audioFrame = { 0 };
		audioFrame.size = nLen;  // ��Ƶ֡��С 
		audioFrame.timestamp = xop::AACSource::GetTimestamp(44100); // ʱ���
		audioFrame.buffer.reset(new uint8_t[audioFrame.size]);
		memcpy(audioFrame.buffer.get(),  pData, nLen);
		rtsp_pusher_->PushFrame(xop::channel_1, audioFrame); //������������, �ӿ��̰߳�ȫ
	}
	return 0;
}
int ARRtspPusher::setVideoData(const char* pData, int nLen, bool bKeyFrame, uint32_t ts)
{
	if (rtsp_pusher_ != NULL && rtsp_pusher_->IsConnected()) {
		//��ȡһ֡ H264, ���
		xop::AVFrame videoFrame = { 0 };
		videoFrame.size = nLen;  // ��Ƶ֡��С 
		videoFrame.timestamp = xop::H264Source::GetTimestamp(); // ʱ���, ����ʹ�ñ������ṩ��ʱ���
		videoFrame.buffer.reset(new uint8_t[videoFrame.size]);
		if (bKeyFrame) {
			videoFrame.type = 1;
		}
		memcpy(videoFrame.buffer.get(), pData, nLen);
		rtsp_pusher_->PushFrame(xop::channel_0, videoFrame); //������������, �ӿ��̰߳�ȫ
	}
	return 0;
}
int ARRtspPusher::setSeiData(const char* pData, int nLen, uint32_t ts)
{
	return 0;
}

