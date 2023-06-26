#ifndef __RTMP_IO_H__
#define __RTMP_IO_H__
#include <map>
#include "../../webrtc/rtc_base/deprecated/recursive_critical_section.h"
#include "../../webrtc/rtc_base/thread.h"

class RtmpIOTick
{
public:
	RtmpIOTick(void) {};
	virtual ~RtmpIOTick(void) {};

	virtual void OnRtmpIOTick() = 0;
	virtual void OnRtmpIODetach() = 0;
};

class RtmpIO : public rtc::Thread
{
protected:
	RtmpIO(void);
public:
	virtual ~RtmpIO(void);

	static RtmpIO&Inst();

	void Attach(RtmpIOTick*ptr);
	void Detach(RtmpIOTick*ptr);

	virtual void Run();

private:
	bool	b_running_;

private:
	typedef std::map<void*, RtmpIOTick*> MapRtmp;

	rtc::RecursiveCriticalSection cs_rtmp_;
	MapRtmp	map_rtmp_;
};

#endif	// __RTMP_IO_H__
