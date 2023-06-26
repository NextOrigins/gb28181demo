#ifndef LIVEPLAYER_VIDEO_CAPTURER_INTERFACE_IMPL_H
#define LIVEPLAYER_VIDEO_CAPTURER_INTERFACE_IMPL_H

#include "VideoCapturerInterface.h"
#include "VideoCameraCapturer.h"
#include "api/media_stream_interface.h"

namespace arlive {

class VideoCapturerInterfaceImpl final : public VideoCapturerInterface {
public:
	VideoCapturerInterfaceImpl(rtc::scoped_refptr<webrtc::JavaVideoTrackSourceInterface> source, std::string deviceId,int width,int height,int fps, std::function<void(VideoState)> stateUpdated, std::shared_ptr<PlatformContext> platformContext);
	void setState(VideoState state) override;
	void setBeautyEffect(bool enable) override;
	void setWhitenessLevel(float level) override;
	void setBeautyLevel(float level) override;
	void setToneLevel(float level) override;


private:
	std::unique_ptr<VideoCameraCapturer> _capturer;

};

} // namespace ARLIVE

#endif
