#include "PlatformImpl.h"
#ifdef WEBRTC_WIN
#include "WinVideoTrackSource.h"
#elif defined(WEBRTC_ANDROID)
#include "jni/liveEngine/AndroidDeviceManager.h"
#endif
#include "VcmCapturer.h"
#include "webrtc/rtc_base/ref_counted_object.h"

rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> createPlatformVideoSouce() {
#ifdef WEBRTC_WIN
	return rtc::make_ref_counted<WinVideoTrackSource>();
#elif defined(WEBRTC_ANDROID)
    return AndroidDeviceManager::Inst().createVideoSource();
#endif

	return NULL;
}


void* createPlatformVideoCapture(const rtc::scoped_refptr<webrtc::VideoTrackSourceInterface>vidSource, size_t width, size_t height, size_t fps, size_t capture_device_index)
{
#if (defined(WEBRTC_WIN) || defined(WEBRTC_MAC))
	VcmCapturer* vCap = VcmCapturer::Create(width, height, fps, capture_device_index);
	vCap->SetVideoSource(vidSource);
	return vCap;
#elif (defined(WEBRTC_ANDROID))
    return AndroidDeviceManager::Inst().createVideoCapture(width, height, fps, capture_device_index);
#endif
}
bool startPlatformVideoCapture(void*ptrCap)
{
	if (ptrCap == NULL)
		return false;
#if (defined(WEBRTC_WIN) || defined(WEBRTC_MAC))
	VcmCapturer* vCap = (VcmCapturer*)ptrCap;
	return vCap->Start();
#elif (defined(WEBRTC_ANDROID))
	return AndroidDeviceManager::Inst().startCapture();
#endif

}
bool stopPlatformVideoCapture(void*ptrCap)
{
	if (ptrCap == NULL)
		return false;
#if (defined(WEBRTC_WIN) || defined(WEBRTC_MAC))
	VcmCapturer* vCap = (VcmCapturer*)ptrCap;
	return vCap->Stop();
#elif (defined(WEBRTC_ANDROID))
	return AndroidDeviceManager::Inst().stopCapture();
#endif
}
void destoryPlatformVideoCapture(void*ptrCap)
{
	if (ptrCap == NULL)
		return;
#if (defined(WEBRTC_WIN) || defined(WEBRTC_MAC))
	VcmCapturer* vCap = (VcmCapturer*)ptrCap;
	vCap->Stop();
	vCap->SetVideoSource(nullptr);
	delete vCap;
	vCap = NULL;
#elif (defined(WEBRTC_ANDROID))
	AndroidDeviceManager::Inst().destoryCapture();
#endif

}


