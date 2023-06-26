#include "VideoCapturerInterfaceImpl.h"

#include <memory>

#include "VideoCameraCapturer.h"

namespace arlive {

VideoCapturerInterfaceImpl::VideoCapturerInterfaceImpl(rtc::scoped_refptr<webrtc::JavaVideoTrackSourceInterface> source, std::string deviceId,int width,int height,int fps, std::function<void(VideoState)> stateUpdated, std::shared_ptr<PlatformContext> platformContext) {
	_capturer = std::make_unique<VideoCameraCapturer>(source, deviceId,width,height,fps, stateUpdated, platformContext);
}



void VideoCapturerInterfaceImpl::setState(VideoState state) {
	_capturer->setState(state);
}

void VideoCapturerInterfaceImpl::setBeautyEffect(bool enable) {
	_capturer->setBeautyEffect(enable);
}

void VideoCapturerInterfaceImpl::setWhitenessLevel(float level) {
	_capturer->setWhitenessLevel(level);
}

void VideoCapturerInterfaceImpl::setToneLevel(float level) {
	_capturer->setToneLevel(level);
}

void VideoCapturerInterfaceImpl::setBeautyLevel(float level) {
	_capturer->setBeautyLevel(level);
}


} // namespace ARLIVE
