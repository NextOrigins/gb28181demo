#ifndef ARLIVE_VIDEO_CAPTURE_INTERFACE_IMPL_H
#define ARLIVE_VIDEO_CAPTURE_INTERFACE_IMPL_H

#include "VideoCaptureInterface.h"
#include <memory>
#include "ThreadLocalObject.h"
#include "api/media_stream_interface.h"
#include "PlatformInterface.h"

namespace arlive {

class VideoCapturerInterface;
class Threads;

class VideoCaptureInterfaceObject {
public:
	VideoCaptureInterfaceObject(std::shared_ptr<PlatformContext> platformContext, Threads &threads);
	~VideoCaptureInterfaceObject();
	void setVideoSource(rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> _videoSource,std::string deviceId, bool isScreenCapture);
	void setBeautyEffect(bool enable);
	void setWhitenessLevel(float level);
	void setBeautyLevel(float level);
	void setToneLevel(float level);
	
	void setCaptureOptions(int width,int height,int fps);
	void switchToDevice(std::string deviceId, bool isScreenCapture);
	void setState(VideoState state);
	void setStateUpdated(std::function<void(VideoState)> stateUpdated);
	webrtc::VideoTrackSourceInterface *source();

private:

    rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> _videoSource;
    int captureWidth;
    int captureHeight;
    int captureFps;
	std::shared_ptr<rtc::VideoSinkInterface<webrtc::VideoFrame>> _currentUncroppedSink;
	std::shared_ptr<PlatformContext> _platformContext;
	std::unique_ptr<VideoCapturerInterface> _videoCapturer;
	std::function<void(VideoState)> _stateUpdated;
    std::function<void(bool)> _onIsActiveUpdated;
	VideoState _state = VideoState::Inactive;
};

class VideoCaptureInterfaceImpl : public VideoCaptureInterface {
public:
	VideoCaptureInterfaceImpl(std::shared_ptr<PlatformContext> platformContext, std::shared_ptr<Threads> threads);
	virtual ~VideoCaptureInterfaceImpl();
	void setVideoSource(rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> _videoSource,std::string deviceId, bool isScreenCapture) override;
	void setCaptureOptions(int width,int height,int fps);
	void switchToDevice(std::string deviceId, bool isScreenCapture) override;
	void setBeautyEffect(bool enable) override;
	void setWhitenessLevel(float level) override;
	void setBeautyLevel(float level) override;
	void setToneLevel(float level) override;

	void setState(VideoState state) override;
    std::shared_ptr<PlatformContext> getPlatformContext() override;

	ThreadLocalObject<VideoCaptureInterfaceObject> *object();

private:
	ThreadLocalObject<VideoCaptureInterfaceObject> _impl;

    std::shared_ptr<PlatformContext> _platformContext;

};

} // namespace ARLIVE

#endif
