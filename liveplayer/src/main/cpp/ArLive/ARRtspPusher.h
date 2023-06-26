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
#ifndef __ARP_RTSP_PUSHER_H__
#define __ARP_RTSP_PUSHER_H__
#include "ARPusher.h"
#include "xop/RtspPusher.h"
#include "rtc_base/thread.h"

class ARRtspPusher : public ARPusher
{
public:
	ARRtspPusher();
	virtual ~ARRtspPusher(void);

	//* For ARPusher
	virtual int startTask(const char* strUrl);
	virtual int stopTask();

	virtual int runOnce();
	virtual int setAudioEnable(bool bAudioEnable) { return 0; };
	virtual int setVideoEnable(bool bVideoEnable) { return 0; };
	virtual int setRepeat(bool bEnable);
	virtual int setRetryCountDelay(int nCount, int nDelay);
	virtual int setAudioData(const char* pData, int nLen, uint32_t ts);
	virtual int setVideoData(const char* pData, int nLen, bool bKeyFrame, uint32_t ts);
	virtual int setSeiData(const char* pData, int nLen, uint32_t ts);

private:
	rtc::Thread* main_thread_;
	bool b_pushed_;
	bool b_connected_;

	std::string str_rtsp_url_;

private:
	std::shared_ptr<xop::EventLoop> event_loop_;
	std::shared_ptr<xop::RtspPusher> rtsp_pusher_;
};

#endif	// __ARP_RTSP_PUSHER_H__