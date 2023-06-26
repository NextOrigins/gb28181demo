#ifndef __RTC_TICK_H__
#define __RTC_TICK_H__
#include <map>
#include "../webrtc/rtc_base/deprecated/recursive_critical_section.h"

class RtcTick
{
public:
	RtcTick(void) : unAttach(false) {};
	virtual ~RtcTick(void) {};

	virtual void OnTick() = 0;
	virtual void OnTickUnAttach() = 0;

	bool unAttach;
};

class MThreadTick
{
public:
	virtual ~MThreadTick();

	void RegisteRtcTick(void* ptr, RtcTick* rtcTick);
	void UnRegisteRtcTick(void* ptr);
	void UnAttachRtcTick(void* ptr);


	void DoProcess();

protected:
	MThreadTick();

private:
	typedef std::map<void*, RtcTick*> MapRtcTick;
	rtc::RecursiveCriticalSection cs_rtc_tick_;
	MapRtcTick		map_rtc_tick_;
};

#endif