#ifndef ARLIVE_PLATFORM_INTERFACE_H
#define ARLIVE_PLATFORM_INTERFACE_H

#include "rtc_base/thread.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "api/video_codecs/video_decoder_factory.h"
#include "api/media_stream_interface.h"
#include "rtc_base/network_monitor_factory.h"
#include "modules/audio_device/include/audio_device.h"
#include "rtc_base/ref_counted_object.h"
#include <string>

namespace arlive {

enum class VideoState;

class VideoCapturerInterface;
class PlatformContext;

struct PlatformCaptureInfo {
    bool shouldBeAdaptedToReceiverAspectRate = false;
    int rotation = 0;
};



class PlatformInterface {
public:
	static PlatformInterface *SharedInstance();
	virtual ~PlatformInterface() = default;


	virtual std::unique_ptr<webrtc::VideoEncoderFactory> makeVideoEncoderFactory(std::shared_ptr<PlatformContext> platformContext) = 0;
	virtual std::unique_ptr<webrtc::VideoDecoderFactory> makeVideoDecoderFactory(std::shared_ptr<PlatformContext> platformContext) = 0;
	virtual bool supportsEncoding(const std::string &codecName, std::shared_ptr<PlatformContext> platformContext) = 0;
	virtual rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> makeVideoSource(rtc::Thread *signalingThread, rtc::Thread *workerThread, bool screencapture) = 0;
    virtual void adaptVideoSource(rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> videoSource, int width, int height, int fps) = 0;
	virtual std::unique_ptr<VideoCapturerInterface> makeVideoCapturer(rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> source, std::string deviceId, int width,int height,int fps,std::function<void(VideoState)> stateUpdated, std::function<void(PlatformCaptureInfo)> captureInfoUpdated, std::shared_ptr<PlatformContext> platformContext) = 0;

};

std::unique_ptr<PlatformInterface> CreatePlatformInterface();

inline PlatformInterface *PlatformInterface::SharedInstance() {
	static const auto result = CreatePlatformInterface();
	return result.get();
}

} // namespace arlive

#endif
