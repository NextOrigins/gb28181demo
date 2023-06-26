#ifndef __PLATFORM_IMPL_H__
#define __PLATFORM_IMPL_H__
#include "webrtc/media/base/adapted_video_track_source.h"
rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> createPlatformVideoSouce();



void* createPlatformVideoCapture(const rtc::scoped_refptr<webrtc::VideoTrackSourceInterface>vidSource, size_t width, size_t height, size_t fps, size_t capture_device_index);
bool startPlatformVideoCapture(void*ptrCap);
bool stopPlatformVideoCapture(void*ptrCap);
void destoryPlatformVideoCapture(void*ptrCap);

#endif
