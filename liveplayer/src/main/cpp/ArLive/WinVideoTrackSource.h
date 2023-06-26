#ifndef __WIN_VIDEO_TRACK_SOURCE_H__
#define __WIN_VIDEO_TRACK_SOURCE_H__
#include "webrtc/media/base/adapted_video_track_source.h"
#include "webrtc/rtc_base/thread.h"

class WinVideoTrackSource : public rtc::AdaptedVideoTrackSource
{
public:
	WinVideoTrackSource(void);
	virtual ~WinVideoTrackSource(void);

	void FrameCaptured(const webrtc::VideoFrame& frame);

	SourceState state() const override {
		return SourceState::kLive;
	}

	bool remote() const override { return false; }

	bool is_screencast() const override {
		return false;
	}

	absl::optional<bool> needs_denoising() const override {
		return false;
	}

private:
	// Encoded sinks not implemented for JavaVideoTrackSourceImpl.
	bool SupportsEncodedOutput() const override { return false; }
	void GenerateKeyFrame() override {}
	void AddEncodedSink(
		rtc::VideoSinkInterface<webrtc::RecordableEncodedFrame>* sink) override {}
	void RemoveEncodedSink(
		rtc::VideoSinkInterface<webrtc::RecordableEncodedFrame>* sink) override {}

private:
};

#endif	// __WIN_VIDEO_TRACK_SOURCE_H__

