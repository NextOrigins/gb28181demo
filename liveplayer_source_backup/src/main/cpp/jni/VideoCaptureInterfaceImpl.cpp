#include "VideoCaptureInterfaceImpl.h"

#include "VideoCapturerInterface.h"
#include "PlatformInterface.h"
#include "StaticThreads.h"

namespace arlive {

VideoCaptureInterfaceObject::VideoCaptureInterfaceObject( std::shared_ptr<PlatformContext> platformContext, Threads &threads)
{
	_platformContext = platformContext;

}

VideoCaptureInterfaceObject::~VideoCaptureInterfaceObject() {

}

webrtc::VideoTrackSourceInterface *VideoCaptureInterfaceObject::source() {
	return _videoSource;
}




void VideoCaptureInterfaceObject::switchToDevice(std::string deviceId, bool isScreenCapture) {
	if (_videoSource) {
        //this should outlive the capturer
        _videoCapturer = nullptr;
		_videoCapturer = PlatformInterface::SharedInstance()->makeVideoCapturer(_videoSource, deviceId,captureWidth,captureHeight,captureFps, [this](VideoState state) {
			if (this->_stateUpdated) {
				this->_stateUpdated(state);
			}
            if (this->_onIsActiveUpdated) {
                switch (state) {
                    case VideoState::Active: {
                        this->_onIsActiveUpdated(true);
                        break;
                    }
                    default: {
                        this->_onIsActiveUpdated(false);
                        break;
                    }
                }
            }
        }, [this](PlatformCaptureInfo info) {
        }, _platformContext);
	}
	if (_videoCapturer) {
		_videoCapturer->setState(_state);
	}
}

void VideoCaptureInterfaceObject::setVideoSource(
        rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> videoSource,std::string deviceId, bool isScreenCapture) {
    _videoSource = videoSource;
	if (_videoSource) {
		//this should outlive the capturer
		_videoCapturer = nullptr;
		_videoCapturer = PlatformInterface::SharedInstance()->makeVideoCapturer(_videoSource, deviceId,captureWidth,captureHeight,captureFps, [this](VideoState state) {
			if (this->_stateUpdated) {
				this->_stateUpdated(state);
			}
			if (this->_onIsActiveUpdated) {
				switch (state) {
					case VideoState::Active: {
						this->_onIsActiveUpdated(true);
						break;
					}
					default: {
						this->_onIsActiveUpdated(false);
						break;
					}
				}
			}
		}, [this](PlatformCaptureInfo info) {
		}, _platformContext);
	}
}

void VideoCaptureInterfaceObject::setCaptureOptions(int width, int height, int fps) {
    captureWidth = width;
    captureHeight= height;
    captureFps = fps;
}

void VideoCaptureInterfaceObject::setBeautyEffect(bool enable) {
	if (_videoCapturer) {
		_videoCapturer->setBeautyEffect(enable);
	}
}

void VideoCaptureInterfaceObject::setBeautyLevel(float level) {
	if (_videoCapturer) {
		_videoCapturer->setBeautyLevel(level);
	}
}

void VideoCaptureInterfaceObject::setWhitenessLevel(float level) {
	if (_videoCapturer) {
		_videoCapturer->setWhitenessLevel(level);
	}
}

void VideoCaptureInterfaceObject::setToneLevel(float level) {
	if (_videoCapturer) {
		_videoCapturer->setToneLevel(level);
	}
}


void VideoCaptureInterfaceObject::setState(VideoState state) {
	if (_state != state) {
		_state = state;
		if (_videoCapturer) {
			_videoCapturer->setState(state);
		}
	}
}






VideoCaptureInterfaceImpl::VideoCaptureInterfaceImpl(std::shared_ptr<PlatformContext> platformContext, std::shared_ptr<Threads> threads) :
_platformContext(platformContext),
_impl(threads->getMediaThread(), [ platformContext, threads]() {
	return new VideoCaptureInterfaceObject( platformContext, *threads);
}) {
}

VideoCaptureInterfaceImpl::~VideoCaptureInterfaceImpl() = default;

void VideoCaptureInterfaceImpl::switchToDevice(std::string deviceId, bool isScreenCapture) {
	_impl.perform(RTC_FROM_HERE, [deviceId, isScreenCapture](VideoCaptureInterfaceObject *impl) {
		impl->switchToDevice(deviceId, isScreenCapture);
	});
}

void VideoCaptureInterfaceImpl::setBeautyEffect(bool enable) {
	_impl.perform(RTC_FROM_HERE, [enable](VideoCaptureInterfaceObject *impl) {
		impl->setBeautyEffect(enable);
	});
}

void VideoCaptureInterfaceImpl::setBeautyLevel(float level) {
	_impl.perform(RTC_FROM_HERE, [level](VideoCaptureInterfaceObject *impl) {
		impl->setBeautyLevel(level);
	});
}

void VideoCaptureInterfaceImpl::setWhitenessLevel(float level) {
	_impl.perform(RTC_FROM_HERE, [level](VideoCaptureInterfaceObject *impl) {
		impl->setWhitenessLevel(level);
	});
}

void VideoCaptureInterfaceImpl::setToneLevel(float level) {
	_impl.perform(RTC_FROM_HERE, [level](VideoCaptureInterfaceObject *impl) {
		impl->setToneLevel(level);
	});
}

void VideoCaptureInterfaceImpl::setVideoSource(
        rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> videoSource,std::string deviceId, bool isScreenCapture) {
    _impl.perform(RTC_FROM_HERE, [videoSource,deviceId,isScreenCapture](VideoCaptureInterfaceObject *impl) {
        impl->setVideoSource(videoSource,deviceId,isScreenCapture);
    });
}

void VideoCaptureInterfaceImpl::setCaptureOptions(int width, int height, int fps) {
    _impl.perform(RTC_FROM_HERE, [width,height,fps](VideoCaptureInterfaceObject *impl) {
        impl->setCaptureOptions(width,height,fps);
    });
}



void VideoCaptureInterfaceImpl::setState(VideoState state) {
	_impl.perform(RTC_FROM_HERE, [state](VideoCaptureInterfaceObject *impl) {
		impl->setState(state);
	});
}


std::shared_ptr<PlatformContext> VideoCaptureInterfaceImpl::getPlatformContext() {
	return _platformContext;
}

ThreadLocalObject<VideoCaptureInterfaceObject> *VideoCaptureInterfaceImpl::object() {
	return &_impl;
}

} // namespace ARLIVE
