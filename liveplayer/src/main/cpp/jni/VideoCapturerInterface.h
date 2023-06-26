#ifndef ARLIVE_VIDEO_CAPTURER_INTERFACE_H
#define ARLIVE_VIDEO_CAPTURER_INTERFACE_H

#include "VideoCaptureInterface.h"

#include <memory>
#include <functional>

namespace rtc {
template <typename VideoFrameT>
class VideoSinkInterface;
} // namespace rtc

namespace webrtc {
class VideoFrame;
} // namespace webrtc

namespace arlive {

class VideoCapturerInterface {
public:
	virtual ~VideoCapturerInterface() = default;
	virtual void setState(VideoState state) = 0;
	virtual void setBeautyEffect(bool enable) = 0;
	virtual void setWhitenessLevel(float level) = 0;
	virtual void setBeautyLevel(float level) =0;
	virtual void setToneLevel(float level) =0;
};

} // namespace arlive

#endif
