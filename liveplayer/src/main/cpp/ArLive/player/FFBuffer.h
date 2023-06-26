/*
*  Copyright (c) 2021 The AnyRTC project authors. All Rights Reserved.
*
*  Please visit https://www.anyrtc.io for detail.
*
* The GNU General Public License is a free, copyleft license for
* software and other kinds of works.
*
* The licenses for most software and other practical works are designed
* to take away your freedom to share and change the works.  By contrast,
* the GNU General Public License is intended to guarantee your freedom to
* share and change all versions of a program--to make sure it remains free
* software for all its users.  We, the Free Software Foundation, use the
* GNU General Public License for most of our software; it applies also to
* any other work released this way by its authors.  You can apply it to
* your programs, too.
* See the GNU LICENSE file for more info.
*/
#ifndef __FF_BUFFER_H__
#define __FF_BUFFER_H__
#include <list>
#ifdef WEBRTC_80
#include "rtc_base/deprecated/recursive_critical_section.h"
#else
#include "webrtc/rtc_base/critical_section.h"
#endif
extern "C" {
#include "libavformat/avformat.h"
}

struct RecvPacket
{
	RecvPacket(void): pkt_(NULL), dts_(0), pts_(0), duration_(0) {
	};
	virtual ~RecvPacket(void) {
		if (pkt_ != NULL) {
			av_packet_unref(pkt_);
			delete pkt_;
			pkt_ = NULL;
		}
	};

	AVPacket* pkt_;
	int64_t dts_;
	int64_t pts_;
	int64_t duration_;
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
	bool bAuto;					//�����Ƿ��Զ���������ʱ�䡣
	int nCacheTime;				//���ò���������ʱ�䡣
	int nMinCacheTime;			//������С�Ļ���ʱ�䡣
	int nMaxCacheTime;			//�������Ļ���ʱ�䡣
	int nVideoBlockThreshold;	//���ò�������Ƶ���ٱ�����ֵ��
};

class FFBuffer
{
public:
	FFBuffer();
	virtual ~FFBuffer(void);

	void DoClear();
	void DoTick();
	bool DoDecodeAudio();
	bool DoDecodeVideo();
	
	void SetPlaySetting(bool bAuto, int nCacheTime, int nMinCacheTime, int nMaxCacheTime, int nVideoBlockThreshold);
	void ResetTime();
	bool NeedMoreData();
	bool IsPlaying();
	bool BufferIsEmpty();
	int AudioCacheTime();
	int VideoCacheTime();
	void RecvVideoData(AVPacket* pkt, int64_t dts, int64_t pts, int64_t duration);
	void RecvAudioData(AVPacket* pkt, int64_t dts, int64_t pts, int64_t duration);

public:
	virtual void OnBufferDecodeAudioData(AVPacket* pkt) {};
	virtual void OnBufferDecodeVideoData(AVPacket* pkt) {};
	virtual void OnBufferStatusChanged(PlayStatus playStatus) {};
	virtual bool OnBufferGetPuased() { return false; }
	virtual float OnBufferGetSpeed() { return 1.0; };
	
private:
	PlayStatus	play_status_;			// ��ǰ״̬
	PlayMode	play_mode_;				// ���ŵ�ģʽ
	bool		b_reset_time_;
	int32_t		n_cacheing_time_;		// ������ʱ�䣬����������ʱ�䣬����Ҫ��������ж�ȡ
	int32_t     n_cache_to_play_time_;	// ���岥��ʱ�䣬��ʼ�򿨶ٺ󣬱���ﵽ���ʱ��������²���
	int32_t		n_cache_to_play_max_;	// ���Ļ��岥��ʱ��
	uint32_t	last_sys_time_;			// �ϴε��õ�ϵͳʱ��
	uint32_t	n_sys_played_time_;		// �Ѿ����ŵ�ʱ��
	uint32_t	n_played_time_;			// �Ѿ����ŵ�ʱ��
	int64_t		decode_data_time_;		// ��������ݿ�ʼʱ��(DTS)
	int64_t		last_aud_dts_time_;		// ��¼��һ�ν������Ƶʱ���
	int64_t		last_vid_dts_time_;		// ��¼��һ�ν������Ƶʱ���

	PlaySetting	play_setting_;

private:
	//* ���ݻ���
#ifdef WEBRTC_80
	rtc::RecursiveCriticalSection	cs_audio_recv_;
#else
	rtc::CriticalSection cs_audio_recv_;
#endif
	std::list<RecvPacket*>	lst_audio_recv_;
#ifdef WEBRTC_80
	rtc::RecursiveCriticalSection	cs_video_recv_;
#else
	rtc::CriticalSection cs_video_recv_;
#endif
	std::list<RecvPacket*>	lst_video_recv_;

private:
	//* ׼����������
#ifdef WEBRTC_80
	rtc::RecursiveCriticalSection	cs_audio_decode_;
#else
	rtc::CriticalSection cs_audio_decode_;
#endif
	std::list<RecvPacket*>	lst_audio_decode_;
#ifdef WEBRTC_80
	rtc::RecursiveCriticalSection	cs_video_decode_;
#else
	rtc::CriticalSection cs_video_decode_;
#endif
	std::list<RecvPacket*>	lst_video_decode_;
};

#endif	// __FF_BUFFER_H__