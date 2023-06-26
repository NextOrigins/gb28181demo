#include "PlayBuffer.h"
#include "../webrtc/rtc_base/logging.h"
#include "../webrtc/rtc_base/time_utils.h"

static const size_t kMaxDataSizeSamples = 3840;
#define MAX_INT 32767
#define MIN_INT -32767
#define CHECK_MAX_VALUE(value) ((value > 32767) ? 32767 : value)
#define CHECK_MIN_VALUE(value) ((value < -32767) ? -32767 : value)
static int MixAudio(int iChannelNum, short* sourceData1, short* sourceData2, float fBackgroundGain, float fWordsGain, short *outputData)
{
	double f = 1.0;
	if (iChannelNum <= 0) {
		return -1;
	}
	if (iChannelNum > 2) {
		return -2;
	}

	if (iChannelNum == 2) {
		float fLeftValue1 = 0;
		float fRightValue1 = 0;
		float fLeftValue2 = 0;
		float fRightValue2 = 0;
		float fLeftValue = 0;
		float fRightValue = 0;
		int output = 0;
		int iIndex = 0;

		fLeftValue1 = (float)(sourceData1[0]);
		fRightValue1 = (float)(sourceData1[1]);
		fLeftValue2 = (float)(sourceData2[0]);
		fRightValue2 = (float)(sourceData2[1]);
		fLeftValue1 = fLeftValue1 * fBackgroundGain;
		fRightValue1 = fRightValue1 * fBackgroundGain;
		fLeftValue2 = fLeftValue2 * fWordsGain;
		fRightValue2 = fRightValue2 * fWordsGain;
		fLeftValue1 = CHECK_MAX_VALUE(fLeftValue1);
		fLeftValue1 = CHECK_MIN_VALUE(fLeftValue1);
		fRightValue1 = CHECK_MAX_VALUE(fRightValue1);
		fRightValue1 = CHECK_MIN_VALUE(fRightValue1);
		fLeftValue2 = CHECK_MAX_VALUE(fLeftValue2);
		fLeftValue2 = CHECK_MIN_VALUE(fLeftValue2);
		fRightValue2 = CHECK_MAX_VALUE(fRightValue2);
		fRightValue2 = CHECK_MIN_VALUE(fRightValue2);
		fLeftValue = fLeftValue1 + fLeftValue2;
		fRightValue = fRightValue1 + fRightValue2;

		for (iIndex = 0; iIndex < 2; iIndex++) {

			if (iIndex == 0) {
				output = (int)(fLeftValue*f);
			}
			else {
				output = (int)(fRightValue*f);
			}
			if (output > MAX_INT)
			{
				f = (double)MAX_INT / (double)(output);
				output = MAX_INT;
			}
			if (output < MIN_INT)
			{
				f = (double)MIN_INT / (double)(output);
				output = MIN_INT;
			}
			if (f < 1)
			{
				f += ((double)1 - f) / (double)32;
			}
			outputData[iIndex] = (short)output;
		}
	}
	else {
		float fValue1 = 0;
		float fValue2 = 0;
		float fValue = 0;

		fValue1 = (float)(*(short*)(sourceData1));
		fValue2 = (float)(*(short*)(sourceData2));
		fValue1 = fValue1 * fBackgroundGain;
		fValue2 = fValue2 * fWordsGain;
		fValue = fValue1 + fValue2;

		fValue = CHECK_MAX_VALUE(fValue);
		fValue = CHECK_MIN_VALUE(fValue);
		*outputData = (short)fValue;
	}
	return 1;
}


PlayBuffer::PlayBuffer(void)
	: aud_data_resamp_(NULL)
	, aud_data_mix_(NULL)
	, b_video_decoded_(false)
	, b_audio_decoded_(false)
{
	aud_data_resamp_ = new char[kMaxDataSizeSamples];
	memset(aud_data_resamp_, 0, kMaxDataSizeSamples);
	aud_data_mix_ = new char[kMaxDataSizeSamples];
	memset(aud_data_mix_, 0, kMaxDataSizeSamples);
}
PlayBuffer::~PlayBuffer(void)
{
	delete[] aud_data_resamp_;
	delete[] aud_data_mix_;

	DoClear();
}

int PlayBuffer::DoRender(bool mix, void* audioSamples, uint32_t samplesPerSec, int nChannels, bool bAudioPaused, bool bVideoPaused)
{
	int ret = 0;
	PcmData* audPkt = NULL;
	VideoData* vidPkt = NULL;
	{//*
		rtc::CritScope cs(&cs_audio_play_);
		//RTC_LOG(LS_INFO) << "Audio list size: " << lst_audio_play_.size();
		if (lst_audio_play_.size() > 0) {
			audPkt = lst_audio_play_.front();
			lst_audio_play_.pop_front();
		}

	}
	{
		rtc::CritScope cs(&cs_video_play_);
		if (lst_video_play_.size() > 0) {
			vidPkt = lst_video_play_.front();
			lst_video_play_.pop_front();
		}
	}

	if (audPkt != NULL) {
		if (!bAudioPaused) {
			ret = 1;
			int a_frame_size = samplesPerSec * nChannels * sizeof(int16_t) / 100;

			if (samplesPerSec != audPkt->sample_hz_ || audPkt->channels_ != nChannels) {
				resampler_.Resample10Msec((int16_t*)audPkt->pdata_, audPkt->sample_hz_ * audPkt->channels_,
					samplesPerSec * nChannels, 1, kMaxDataSizeSamples, (int16_t*)aud_data_resamp_);
			}
			else {
				memcpy(aud_data_resamp_, audPkt->pdata_, a_frame_size);
			}
			if (!mix) {
				memcpy(audioSamples, aud_data_resamp_, a_frame_size);
			}
			else {
				float voice_gain = 1.0;
				float musice_gain = 1.0;
				short* pMusicUnit = (short*)aud_data_resamp_;
				short* pMicUnit = (short*)audioSamples;
				short* pOutputPcm = (short*)aud_data_mix_;
				for (int iIndex = 0; iIndex < audPkt->len_; iIndex = iIndex + nChannels) {
					MixAudio(nChannels, &pMusicUnit[iIndex], &pMicUnit[iIndex], musice_gain, voice_gain, &pOutputPcm[iIndex]);
				}
				memcpy(audioSamples, pOutputPcm, a_frame_size);
			}
		}
		delete audPkt;
		audPkt = NULL;
	}
	else {
		//RTC_LOG(LS_INFO) << "* No audio data time: " << rtc::Time32();
	}

	if (vidPkt != NULL) {
		//RTC_LOG(LS_INFO) << "DoRender video pts: " << vidPkt->pts_ << " plytime: " << play_pts_time_;
		if (!bVideoPaused) {
			OnBufferVideoRender(vidPkt, vidPkt->pts_);
		}
		delete vidPkt;
		vidPkt = NULL;
	}

	return ret;
}

void PlayBuffer::DoClear()
{
	{
		rtc::CritScope cs(&cs_audio_play_);
		std::list<PcmData*>::iterator iter = lst_audio_play_.begin();
		while (iter != lst_audio_play_.end()) {
			PcmData* pkt = *iter;
			lst_audio_play_.erase(iter++);
			delete pkt;
		}
	}
	{
		rtc::CritScope cs(&cs_video_play_);
		std::list<VideoData*>::iterator iter = lst_video_play_.begin();
		while (iter != lst_video_play_.end()) {
			VideoData* pkt = *iter;
			lst_video_play_.erase(iter++);
			delete pkt;
		}
	}
}

bool PlayBuffer::NeedMoreAudioPlyData()
{
	rtc::CritScope cs(&cs_audio_play_);
	if (lst_audio_play_.size() > 3) {
		return false;
	}
	return true;
}
bool PlayBuffer::NeedMoreVideoPlyData()
{
	rtc::CritScope cs(&cs_video_play_);
	if (lst_video_play_.size() > 1)
	{
		return false;
	}
	return true;
}
void PlayBuffer::PlayVideoData(VideoData *videoData)
{
	//RTC_LOG(LS_INFO) << "PlayVideoData pts: " << videoData->pts_;
	if (!b_video_decoded_) {
		b_video_decoded_ = true;
		OnFirstVideoDecoded();
	}
	rtc::CritScope cs(&cs_video_play_);
	if (lst_video_play_.size() > 0 && false) {
		if (lst_video_play_.front()->pts_ >= videoData->pts_) {
			lst_video_play_.push_front(videoData);
		}
		else if (lst_video_play_.back()->pts_ <= videoData->pts_) {
			lst_video_play_.push_back(videoData);
		}
		else {
			std::list<VideoData*>::iterator iter = lst_video_play_.begin();
			while (iter != lst_video_play_.end()) {
				if ((*iter)->pts_ >= videoData->pts_) {
					lst_video_play_.insert(iter, videoData);
					break;
				}
				iter++;
			}
		}
	}
	else {
		lst_video_play_.push_back(videoData);
	}
}
void PlayBuffer::PlayAudioData(PcmData*pcmData)
{
	//RTC_LOG(LS_INFO) << "PlayAudioData pts: " << pcmData->pts_;
	if (!b_audio_decoded_) {
		b_audio_decoded_ = true;
		OnFirstAudioDecoded();
	}
	rtc::CritScope cs(&cs_audio_play_);
	lst_audio_play_.push_back(pcmData);

	//RTC_LOG(LS_INFO) << "PlayAudioData list size: " << lst_audio_play_.size();
}

