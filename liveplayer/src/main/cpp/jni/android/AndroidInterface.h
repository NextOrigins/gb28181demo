//
// Created by liu on 2021/9/18.
//

#ifndef LIVEPLAYER_ANDROIDINTERFACE_H
#define LIVEPLAYER_ANDROIDINTERFACE_H

#include "sdk/android/native_api/video/video_source.h"
#include "PlatformInterface.h"
#include "VideoCapturerInterface.h"

namespace arlive{
    class AndroidInterface : public PlatformInterface {
    public:
        std::unique_ptr<webrtc::VideoEncoderFactory> makeVideoEncoderFactory(std::shared_ptr<PlatformContext> platformContext) override;
        std::unique_ptr<webrtc::VideoDecoderFactory> makeVideoDecoderFactory(std::shared_ptr<PlatformContext> platformContext) override;
        bool supportsEncoding(const std::string &codecName, std::shared_ptr<PlatformContext> platformContext) override;
        rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> makeVideoSource(rtc::Thread *signalingThread, rtc::Thread *workerThread, bool screencapture) override;
        void adaptVideoSource(rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> videoSource, int width, int height, int fps) override;
        std::unique_ptr<VideoCapturerInterface> makeVideoCapturer(rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> source, std::string deviceId,int width,int height,int fps, std::function<void(VideoState)> stateUpdated, std::function<void(PlatformCaptureInfo)> captureInfoUpdated, std::shared_ptr<PlatformContext> platformContext) override;

    private:
        rtc::scoped_refptr<webrtc::JavaVideoTrackSourceInterface> _source[2];
        std::unique_ptr<webrtc::VideoEncoderFactory> hardwareVideoEncoderFactory;
        std::unique_ptr<webrtc::VideoEncoderFactory> softwareVideoEncoderFactory;

    };
}



#endif //LIVEPLAYER_ANDROIDINTERFACE_H
