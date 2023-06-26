#ifndef ARLIVE_VIDEO_CAPTURE_INTERFACE_H
#define ARLIVE_VIDEO_CAPTURE_INTERFACE_H

#include <string>
#include <memory>
#include <functional>
#include "api/media_stream_interface.h"

namespace rtc {
template <typename VideoFrameT>
class VideoSinkInterface;
} // namespace rtc

namespace webrtc {
class VideoFrame;
} // namespace webrtc

namespace arlive {

class PlatformContext;
class Threads;

enum class VideoState {
	Inactive,
	Paused,
	Active,
};


class VideoCaptureInterface {
protected:
	VideoCaptureInterface() = default;

public:
	static std::unique_ptr<VideoCaptureInterface> Create(
                std::shared_ptr<Threads> threads,
		std::shared_ptr<PlatformContext> platformContext = nullptr);

	virtual ~VideoCaptureInterface();
	virtual void setVideoSource(rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> _videoSource, std::string deviceId = std::string(),
								bool isScreenCapture = false) =0;
	virtual void setCaptureOptions(int width,int height,int fps) = 0;
	virtual void switchToDevice(std::string deviceId, bool isScreenCapture) = 0;
	virtual void setBeautyEffect(bool enable) = 0;
	virtual void setWhitenessLevel(float level) = 0;
	virtual void setBeautyLevel(float level) =0;
	virtual void setToneLevel(float level) =0;
	virtual void setState(VideoState state) = 0;

	virtual std::shared_ptr<PlatformContext> getPlatformContext() {
		return nullptr;
	}
};

} // namespace arlive

#endif
