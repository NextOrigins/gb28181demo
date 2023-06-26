#ifndef __PLAY_BUFFER_H__
#define __PLAY_BUFFER_H__
#include <list>
#include "../webrtc/rtc_base/deprecated/recursive_critical_section.h"
#include "../webrtc/modules/audio_coding/acm2/acm_resampler.h"
#include "../webrtc/api/video/video_frame.h"

struct PcmData {
	PcmData(char* pdata, int len, int sample_hz, int channels) :len_(len), sample_hz_(sample_hz), channels_(channels), pts_(0) {
		pdata_ = new char[len];
		memcpy(pdata_, pdata, len);
	}
	virtual ~PcmData(void) {
		delete[] pdata_;
	}
	char* pdata_;
	int len_;
	int sample_hz_;
	int channels_;
	int64_t pts_;
};

struct VideoData {
	VideoData(): pdata_(NULL), len_(0) { };
	virtual ~VideoData(void) {
		if (pdata_ != NULL) {
			delete[] pdata_;
			pdata_ = NULL;
		}
		
	};

	void SetData(char* ptr, int len) {
		if (pdata_ != NULL) {
			delete[] pdata_;
			pdata_ = NULL;
		}
		pdata_ = ptr; len_ = len;
	}

	rtc::scoped_refptr<webrtc::VideoFrameBuffer> video_frame_;
	char* pdata_;
	int len_;
	int64_t pts_;
};


class PlayBuffer
{
public:
	PlayBuffer(void);
	virtual ~PlayBuffer(void);

	int DoRender(bool mix, void* audioSamples, uint32_t samplesPerSec, int nChannels, bool bAudioPaused, bool bVideoPaused);
	void DoClear();

	bool NeedMoreAudioPlyData();
	bool NeedMoreVideoPlyData();
	void PlayVideoData(VideoData *videoData);
	void PlayAudioData(PcmData*pcmData);


public:
	virtual void OnBufferVideoRender(VideoData *videoData, int64_t pts) {};
	virtual void OnFirstVideoDecoded() {};
	virtual void OnFirstAudioDecoded() {};

private:
	bool	b_video_decoded_;
	bool	b_audio_decoded_;

	//* 显示缓存
	rtc::RecursiveCriticalSection	cs_audio_play_;
	std::list<PcmData*>		lst_audio_play_;
	rtc::RecursiveCriticalSection	cs_video_play_;
	std::list<VideoData*>	lst_video_play_;

private:
	webrtc::acm2::ACMResampler resampler_;
	char* aud_data_resamp_;             // The actual 16bit audio data.
	char* aud_data_mix_;
};

#endif	// __PLAY_BUFFER_H__