#include "SyncBuffer.h"
#include "webrtc/base/timeutils.h"

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


SyncBuffer::SyncBuffer(SyncBufferEvent&callback)
	: callback_(callback)
	, play_status_(PS_Init)
	, play_mode_(PM_Fluent)
	, decode_sys_time_(0)
	, decode_data_time_(0)
	, play_sys_time_(0)
	, play_data_time_(0)
	, aud_data_resamp_(NULL)
	, aud_data_mix_(NULL)
{
	aud_data_resamp_ = new char[kMaxDataSizeSamples];
	memset(aud_data_resamp_, 0, kMaxDataSizeSamples);
	aud_data_mix_ = new char[kMaxDataSizeSamples];
	memset(aud_data_mix_, 0, kMaxDataSizeSamples);
}
SyncBuffer::~SyncBuffer(void)
{
	DoClear();

	delete[] aud_data_resamp_;
	delete[] aud_data_mix_;
}

void SyncBuffer::DoClear()
{
	{
		rtc::CritScope cs(&cs_audio_recv_);
		std::list<RecvPacket*>::iterator iter = lst_audio_recv_.begin();
		while (iter != lst_audio_recv_.end()) {
			RecvPacket* pkt = *iter;
			lst_audio_recv_.erase(iter++);
			delete pkt;
		}
	}
	{
		rtc::CritScope cs(&cs_video_recv_);
		std::list<RecvPacket*>::iterator iter = lst_video_recv_.begin();
		while (iter != lst_video_recv_.end()) {
			RecvPacket* pkt = *iter;
			lst_video_recv_.erase(iter++);
			delete pkt;
		}
	}

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

void SyncBuffer::DoTick()
{
	if (play_status_ == PS_Init || play_status_ == PS_Caching) {
		int vidCacheTime = 0;
		int audCacheTime = 0;
		uint32_t vidFirstDtsTime = 0;
		uint32_t audFirstDtsTime = 0;
		uint32_t vidFirstPtsTime = 0;
		uint32_t audFirstPtsTime = 0;
		{
			rtc::CritScope cs(&cs_video_recv_);
			if (lst_video_recv_.size() > 1) {
				vidCacheTime = lst_video_recv_.back()->_dts - lst_video_recv_.front()->_dts;
				vidFirstDtsTime = lst_video_recv_.front()->_dts;
				vidFirstPtsTime = lst_video_recv_.front()->_pts;
			}
		}
		{
			rtc::CritScope cs(&cs_audio_recv_);
			if (lst_audio_recv_.size() > 0) {
				audCacheTime = lst_audio_recv_.back()->_dts - lst_audio_recv_.front()->_dts;
				audFirstDtsTime = lst_audio_recv_.front()->_dts;
				audFirstPtsTime = lst_audio_recv_.front()->_pts;
			}
		}
		int dataCacheTime = audCacheTime > vidCacheTime ? audCacheTime : vidCacheTime;

		if (play_setting_.bAuto) {
			if (dataCacheTime >= play_setting_.nMinCacheTime * 1000) {
				decode_sys_time_ = rtc::Time32();
				play_sys_time_ = rtc::Time32();
				if (audFirstDtsTime != 0) {
					decode_data_time_ = audFirstDtsTime;
				}
				else {
					decode_data_time_ = vidFirstDtsTime;
				}
				if (audFirstPtsTime != 0) {
					play_data_time_ = audFirstPtsTime;
				}
				else {
					play_data_time_ = vidFirstPtsTime;
				}
				play_status_ = PS_Playing;
			}
		}
		else {
			if (dataCacheTime >= play_setting_.nCacheTime * 1000) {
				decode_sys_time_ = rtc::Time32();
				play_sys_time_ = rtc::Time32();
				if (audFirstDtsTime != 0) {
					decode_data_time_ = audFirstDtsTime;
				}
				else {
					decode_data_time_ = vidFirstDtsTime;
				}
				if (audFirstPtsTime != 0) {
					play_data_time_ = audFirstPtsTime;
				}
				else {
					play_data_time_ = vidFirstPtsTime;
				}
				play_status_ = PS_Playing;
			}
		}
	}
	else {
		int vidListSize = 0;
		int audListSize = 0;
		uint32_t decodeTime = decode_data_time_ + (rtc::Time32() - decode_sys_time_);
		std::list< RecvPacket*> vidList;
		std::list< RecvPacket*> audList;
		{
			rtc::CritScope cs(&cs_video_recv_);
			if (lst_video_recv_.size() > 0) {
				vidListSize = lst_video_recv_.size();
				while (lst_video_recv_.size() > 0) {
					if (lst_video_recv_.front()->_dts <= decodeTime) {
						vidList.push_back(lst_video_recv_.front());
						lst_video_recv_.pop_front();
					}
					else {
						break;
					}
				}
			}
		}
		{
			rtc::CritScope cs(&cs_audio_recv_);
			if (lst_audio_recv_.size() > 0) {
				audListSize = lst_audio_recv_.size();
				while (lst_audio_recv_.size() > 0) {
					if (lst_audio_recv_.front()->_dts <= decodeTime) {
						audList.push_back(lst_audio_recv_.front());
						lst_audio_recv_.pop_front();
					}
					else {
						break;
					}
				}
			}
		}

		while (vidList.size() > 0) {
			RecvPacket* vidPkt = vidList.front();
			vidList.pop_front();
			callback_.OnSyncBufferDecodeData(vidPkt->_type, vidPkt->_data, vidPkt->_data_len, vidPkt->_pts);
			delete vidPkt;
			vidPkt = NULL;
		}
		while (audList.size() > 0) {
			RecvPacket* audPkt = audList.front();
			audList.pop_front();
			callback_.OnSyncBufferDecodeData(audPkt->_type, audPkt->_data, audPkt->_data_len, audPkt->_pts);
			delete audPkt;
			audPkt = NULL;
		}

		if (vidListSize == 0 && audListSize == 0) {
			play_status_ = PS_Caching;
		}
	}
}

int SyncBuffer::DoRender(bool mix, void* audioSamples, uint32_t samplesPerSec, int nChannels)
{
	int ret = 0;
	PcmData* audPkt = NULL;
	VideoData* vidPkt = NULL;
	uint32_t playTime = play_data_time_ + (rtc::Time32() - play_sys_time_);
	{//*
		rtc::CritScope cs(&cs_audio_play_);
		if (lst_audio_play_.size() > 0) {
			if (lst_audio_play_.front()->pts_ <= playTime) {
				audPkt = lst_audio_play_.front();
				lst_audio_play_.pop_front();
			}
		}
	}
	{
		rtc::CritScope cs(&cs_video_play_);
		if (lst_video_play_.size() > 0) {
			if (lst_video_play_.front()->pts_ <= playTime) {
				vidPkt = lst_video_play_.front();
				lst_video_play_.pop_front();
			}
		}
	}

	if (audPkt != NULL) {
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

		delete audPkt;
		audPkt = NULL;
	}
	else {
		//LOG(LS_INFO) << "* No audio data time: " << rtc::Time32();
	}

	if (vidPkt != NULL) {
		LOG(LS_INFO) << "DoRender video pts: " << vidPkt->pts_ << " plytime: "<< playTime;
		callback_.OnSyncBufferVideoRender(vidPkt->video_frame_, vidPkt->pts_);
		delete vidPkt;
		vidPkt = NULL;
	}

	return ret;
}

void SyncBuffer::SetPlaySetting(bool bAuto, int nCacheTime, int nMinCacheTime, int nMaxCacheTime, int nVideoBlockThreshold)
{
	play_setting_.bAuto = bAuto;
	play_setting_.nCacheTime = nCacheTime;
	play_setting_.nMinCacheTime = nMinCacheTime;
	play_setting_.nMaxCacheTime = nMaxCacheTime;
	play_setting_.nVideoBlockThreshold = nVideoBlockThreshold;
}
void SyncBuffer::RecvVideoData(int nType, const char*pData, int nLen, uint32_t dts, uint32_t pts)
{
	//LOG(LS_INFO) << "RecvVideoData dts: " << dts << " pts: " << pts;

	RecvPacket* pkt = new RecvPacket(true);
	pkt->SetData((char*)pData, nLen, pts);
	pkt->_type = nType;
	pkt->_dts = dts;

	rtc::CritScope cs(&cs_video_recv_);
	lst_video_recv_.push_back(pkt);
}
void SyncBuffer::RecvAudioData(int nType, const char*pData, int nLen, uint32_t dts, uint32_t pts)
{
	//LOG(LS_INFO) << "RecvAudioData dts: " << dts << " pts: " << pts;
	RecvPacket* pkt = new RecvPacket(true);
	pkt->SetData((char*)pData, nLen, pts);
	pkt->_type = nType;
	pkt->_dts = dts;

	rtc::CritScope cs(&cs_audio_recv_);
	lst_audio_recv_.push_back(pkt);
}

void SyncBuffer::PlayVideoData(rtc::scoped_refptr<webrtc::VideoFrameBuffer> videoFrame, uint32_t pts)
{
	LOG(LS_INFO) << "PlayVideoData pts: " << pts;
	VideoData* pkt = new VideoData();
	pkt->video_frame_ = videoFrame;
	pkt->pts_ = pts;

	rtc::CritScope cs(&cs_video_play_);
	if (lst_video_play_.size() > 0 && false) {
		if (lst_video_play_.front()->pts_ >= pts) {
			lst_video_play_.push_front(pkt);
		}
		else if (lst_video_play_.back()->pts_ <= pts) {
			lst_video_play_.push_back(pkt);
		}
		else {
			std::list<VideoData*>::iterator iter = lst_video_play_.begin();
			while (iter != lst_video_play_.end()) {
				if ((*iter)->pts_ >= pts) {
					lst_video_play_.insert(iter, pkt);
					break;
				}
				iter++;
			}
		}
	}
	else {
		lst_video_play_.push_back(pkt);
	}
}
void SyncBuffer::PlayAudioData(const char*pData, int nLen, int sampleHz, int nChannel, uint32_t pts)
{
	//LOG(LS_INFO) << "PlayAudioData pts: " << pts  << " time: " <<rtc::Time32();
	PcmData* pkt = new PcmData((char*)pData, nLen, sampleHz, nChannel);
	pkt->pts_ =  pts;

	rtc::CritScope cs(&cs_audio_play_);
	lst_audio_play_.push_back(pkt);
}