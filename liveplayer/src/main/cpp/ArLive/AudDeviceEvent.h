#ifndef __AUD_DEVICE_EVENT_H__
#define __AUD_DEVICE_EVENT_H__
#include <stdint.h>

class AudDevCaptureEvent
{
public:
	AudDevCaptureEvent(void) {};
	virtual ~AudDevCaptureEvent(void) {};

	virtual void RecordedDataIsAvailable(const void* audioSamples, const size_t nSamples,
		const size_t nBytesPerSample, const size_t nChannels, const uint32_t samplesPerSec, const uint32_t totalDelayMS) {};
};

class AudDevSpeakerEvent
{
public:
	AudDevSpeakerEvent(void) {}
	virtual ~AudDevSpeakerEvent(void) {};


	virtual int MixAudioData(bool mix, void* audioSamples, uint32_t samplesPerSec, int nChannels) { return 0; };
};

#endif	// __AUD_DEVICE_EVENT_H__
