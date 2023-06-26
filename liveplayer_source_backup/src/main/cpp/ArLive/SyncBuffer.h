#ifndef __SYNC_BUFFER_H__
#define __SYNC_BUFFER_H__
#include <list>
#include "webrtc/base/criticalsection.h"
#include "webrtc/modules/audio_coding/acm2/acm_resampler.h"
#include "webrtc/media/engine/webrtcvideoframe.h"

static const size_t kMaxDataSizeSamples = 3840;

struct PlyPacket
{
	PlyPacket(bool isvideo) :_data(NULL), _data_len(0),
		_b_video(isvideo),  _pts(0) {}

	virtual ~PlyPacket(void) {
		if (_data)
			delete[] _data;
	}
	void SetData(const char*pdata, int len, uint32_t pts) {
		_pts = pts;
		if (len > 0 && pdata != NULL) {
			if (_data)
				delete[] _data;
			if (_b_video)
				_data = new char[len + 8];
			else
				_data = new char[len];
			memcpy(_data, pdata, len);
			_data_len = len;
		}
	}
	char*_data;
	int _data_len;
	bool _b_video;
	uint32_t _pts;
};

struct RecvPacket : public PlyPacket
{
	RecvPacket(bool isvideo) :PlyPacket(isvideo), _type(0), _dts(0) {}

	virtual ~RecvPacket(void) {

	}
	
	int _type;
	uint32_t _dts;
};

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
	uint32_t pts_;
};

struct VideoData {
	VideoData(void) {};
	virtual ~VideoData(void) { video_frame_ = NULL; };

	rtc::scoped_refptr<webrtc::VideoFrameBuffer> video_frame_;
	uint32_t pts_;
};

enum PlayMode
{
	PM_Fluent = 0,	// ���������л��壬�ʺ�ֱ��
	PM_RealTime,	// ʵʱ��û�л��壬�ʺ϶���Ƶ
};

enum PlayStatus
{
	PS_Init = 0,	// ��ʼ��״̬
	PS_Caching,		// ������
	PS_Playing,		// ������
};

struct PlaySetting
{
	bool bAuto;	//�����Ƿ��Զ���������ʱ�䡣
	int nCacheTime;	//���ò���������ʱ�䡣
	int nMinCacheTime;	//������С�Ļ���ʱ�䡣
	int nMaxCacheTime;	//�������Ļ���ʱ�䡣
	int nVideoBlockThreshold;	//���ò�������Ƶ���ٱ�����ֵ��
};

class SyncBufferEvent
{
public:
	SyncBufferEvent(void) {};
	virtual ~SyncBufferEvent(void) {};

	virtual void OnSyncBufferDecodeData(int nType, const char*pData, int nLen, uint32_t pts) {};
	virtual void OnSyncBufferVideoRender(rtc::scoped_refptr<webrtc::VideoFrameBuffer>videoFrame, uint32_t pts) {};
};

class SyncBuffer
{
public: 
	SyncBuffer(SyncBufferEvent&callback);
	virtual ~SyncBuffer(void);

	void DoClear();
	void DoTick();
	int DoRender(bool mix, void* audioSamples, uint32_t samplesPerSec, int nChannels);
	

	void SetPlaySetting(bool bAuto, int nCacheTime, int nMinCacheTime, int nMaxCacheTime, int nVideoBlockThreshold);
	void RecvVideoData(int nType, const char*pData, int nLen, uint32_t dts, uint32_t pts);
	void RecvAudioData(int nType, const char*pData, int nLen, uint32_t dts, uint32_t pts);

	void PlayVideoData(rtc::scoped_refptr<webrtc::VideoFrameBuffer> videoFrame, uint32_t pts);
	void PlayAudioData(const char*pData, int nLen, int sampleHz, int nChannel, uint32_t pts);

private:
	SyncBufferEvent &callback_;
	PlayStatus	play_status_;
	PlayMode	play_mode_;
	uint32_t	decode_sys_time_;		// �����ϵͳ��ʼʱ��
	uint32_t	decode_data_time_;		// ��������ݿ�ʼʱ��(DTS)
	uint32_t	play_sys_time_;			// ���ŵ�ϵͳ��ʼʱ��
	uint32_t	play_data_time_;		// ���ŵ����ݿ�ʼʱ��(PTS)

	PlaySetting	play_setting_;
private:
	//* ���ݻ���
	rtc::CriticalSection	cs_audio_recv_;
	std::list<RecvPacket*>	lst_audio_recv_;
	rtc::CriticalSection	cs_video_recv_;
	std::list<RecvPacket*>	lst_video_recv_;

private:
	//* ��ʾ����
	rtc::CriticalSection	cs_audio_play_;
	std::list<PcmData*>		lst_audio_play_;
	rtc::CriticalSection	cs_video_play_;
	std::list<VideoData*>	lst_video_play_;

private:
	webrtc::acm2::ACMResampler resampler_;
	char* aud_data_resamp_;             // The actual 16bit audio data.
	char* aud_data_mix_;
};

#endif	// __SYNC_BUFFER_H__